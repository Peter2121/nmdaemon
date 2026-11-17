# Subscription event workflow

This document explains how a `SUBSCRIBE`d client actually receives events: who
decides an event happened, who builds it, and who sends it. For the wire
format (the exact JSON shapes, validation rules, and which commands publish
what today) see the "Subscriptions" section of `commands.md`. This document
is about the internals behind that wire format.

## The actors

- **The subscriber**: a client that previously sent `SUBSCRIBE` and is
  listening on the socket path it gave.
- **The triggering client**: whoever sends the command that causes the
  event (`IF_ADD`, `IF_ENABLE`, etc.). This can be the same process as the
  subscriber, or a completely unrelated one — nmdaemon doesn't care who
  triggers an event, only who's registered to hear about it.
- **The worker**: the single, long-lived `NmWorkerBase`-derived object for
  that scope (e.g. one `NmWorkerSys` instance handles every `SYSTEM`-scope
  command, for every client connection, for the whole life of the nmdaemon
  process). It owns the subscriber list for its scope and is the only thing
  that ever emits events for that scope.
- **async++'s thread pool**: where the actual socket I/O to subscribers
  happens, since the change described below. Not a persistent "event bus" —
  just a pool of worker threads async++ already maintains for the project
  (see `nmworkerwpa.cpp` for its other use in this codebase).

## Walkthrough, using `IF_ADD` as the example

**1. Registration, once, in advance.**

The subscriber sends, over nmdaemon's normal command socket, the same as
any other command:

    { "SCOPE": "SYSTEM", "CMD": "SUBSCRIBE", "SOCKET": "/var/tmp/nmdcli11451" }

`NmWorkerSys::execCmdSubscribe()` (inherited from `NmWorkerBase`) validates
`SOCKET` and appends `{socketPath, dataFilter}` to that worker's own
`subscriptions` vector — an in-memory list, protected by its own mutex,
living inside the worker object itself. It is *not* written anywhere, not
shared with any other scope's worker, and it persists for as long as
nmdaemon keeps running, independent of which client connection sent the
`SUBSCRIBE` or whether that connection is still open.

**2. Someone triggers a change.**

Some client — again, could be the subscriber, could be anyone — sends:

    { "SCOPE": "SYSTEM", "CMD": "IF_ADD", "DATA": { "INTERFACE NAME": "epair8", "INTERFACE TYPE": "EPAIR" } }

`NmDaemon`'s dispatcher thread for *that* connection routes it to
`NmWorkerSys::execCmd()` → `execCmdIfAdd()`, which does the real work (the
`SIOCIFCREATE2` ioctl and the rest of the creation logic).

**3. The same function decides whether to emit, and builds the event.**

If — and only if — the operation actually succeeded and the interface was
genuinely created (not "already existed"), `execCmdIfAdd()` — still the
exact same function call, still running on the dispatcher thread handling
the `IF_ADD` request — calls `publishEvent(NmCmd::IF_ADD, {...})` on itself.
There is no separate "event detector": the command handler that performed
the change is also the one that decides an event happened, right at the
point in its own code where it already knows the outcome — and it's also
the one that tells `publishEvent` which command this is, via its first
argument, so the delivered event can carry that as `EVENT` (e.g.
`"IF_ADD"`) without `publishEvent` itself needing to know anything about
who's calling it.

**4. `publishEvent` hands the actual sending off to async++, and returns immediately.**

This is the part that changed. `publishEvent()`:

1. Locks `subscriptions_mutex`, scans the worker's subscriber list, filters
   to the ones whose `DATA` filter matches this event, copies those into a
   small local list, and unlocks. This part is synchronous — it's pure
   in-memory work, microseconds at most.
2. If no subscriber matches, returns immediately — nothing more happens.
3. Otherwise, builds the `{"EVENT":..., "SCOPE":..., "DATA":...}` payload, then hands
   `{the copied subscriber list, the payload}` to `async::spawn(...)` and
   **discards the returned task handle** (fire-and-forget) — it does not
   wait for it, keep it, or check on it later.
4. `publishEvent()` returns at this point. `execCmdIfAdd()` then returns its
   own result, which the dispatcher turns into the ordinary
   `{"RESULT":"SUCCESS",...}` response sent back to whoever sent `IF_ADD`.

The connect/write/close to each matching subscriber's socket — the part
that can actually take a noticeable amount of wall-clock time if a
subscriber is slow to accept or its process is scheduled out — now happens
inside that `async::spawn`ed task, on one of async++'s own pool threads, at
some point shortly after step 4, independently of the dispatcher thread and
independently of whether the triggering client has already gotten its
response.

## What this means in practice

**The worker thread is never blocked by a subscriber.** Before this change,
`execCmdIfAdd()` (and the dispatcher thread running it) would stay blocked
inside `publishEvent()` until every matching subscriber's socket had been
connected to, written to, and closed, one at a time. A slow or unresponsive
subscriber delayed the response to the *triggering* client, who has nothing
to do with that subscriber. Now `publishEvent()` only ever blocks for the
cheap in-memory filtering step; the slow part runs elsewhere.

**The ordering guarantee is gone, deliberately.** Previously, a client that
got `{"RESULT":"SUCCESS"}` back from `IF_ADD` could rely on every
currently-registered subscriber having already been notified (or at least
attempted) by that point — the two were strictly ordered. That's no longer
true: the command's response and the subscriber notifications now race each
other, and either can arrive first. This is the direct tradeoff for not
blocking, not an accident — if an observer needs strict ordering between "I
got a success response" and "subscribers have been told", this design no
longer provides it.

**A failed delivery still doesn't affect the triggering command.** This was
already true before (a dead subscriber socket is logged and dropped,
never surfaced in the triggering command's own `RESULT`), and stays true
now: `async::spawn`'s lambda wraps the delivery loop in its own `try`/`catch`
purely so a failure is visible in nmdaemon's logs, not because it could
otherwise crash anything. (An uncaught exception inside a fire-and-forget
async++ task that nobody ever calls `.get()`/`.wait()` on is simply stored
and dropped by the library — unlike a raw `std::thread`, it does not call
`std::terminate()` and take the whole process down.)

**Each event is still independent and one-shot.** nmdaemon does not keep a
persistent connection to any subscriber. Every single event, for every
matching subscriber, is its own fresh connect → write → close, exactly as
before — only now that sequence runs on an async++ pool thread instead of
the worker thread that detected the change.
