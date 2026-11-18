#ifndef NMWORKERBASE_H
#define NMWORKERBASE_H

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>
#include <string>

#include "sockpp/socket.h"
#include "nmcommanddata.h"

// Non-blocking, thread-transferable "busy" guard for a single worker
// instance. Backed by an atomic flag rather than a mutex: an async job
// acquires this in the dispatcher thread but releases it later from its own
// background thread once the job completes, and std::mutex::unlock() from a
// thread other than the one that locked it is undefined behavior (FreeBSD's
// libthr enforces this with a hard trap, which is what SIGILL'd here).
class WorkerAccessGuard
{
public:
    explicit WorkerAccessGuard(std::atomic<bool>& flag);
    WorkerAccessGuard(const WorkerAccessGuard&) = delete;
    WorkerAccessGuard& operator=(const WorkerAccessGuard&) = delete;
    WorkerAccessGuard(WorkerAccessGuard&& other) noexcept;
    WorkerAccessGuard& operator=(WorkerAccessGuard&& other) noexcept;
    ~WorkerAccessGuard();

    bool owns() const { return flag_ != nullptr; }
    void release();

private:
    std::atomic<bool>* flag_ = nullptr;
};

// Generic PF_ROUTE listener, reusable by any worker that wants to react to
// kernel routing-socket events on its own independent socket (one per
// worker that wants this, not a shared listener - see the "thread-per-
// scope vs. shared monitor" discussion: the kernel broadcasts each message
// independently to every open PF_ROUTE socket anyway, so N listeners cost
// nothing worth avoiding at this scale). Deliberately ignorant of
// NmCmd/json/scope - it just hands raw bytes to the handler and knows
// nothing about what they mean; all of that stays in the per-worker
// handler the caller supplies. RAII: opens the socket and starts the
// background thread in the constructor, signals it to stop via a self-pipe
// and joins it in the destructor (not a racy cross-thread close() against
// a blocked read() - same shutdown discipline as WorkerAccessGuard's own
// cross-thread-unlock lesson above). Construct it as an ordinary member
// with an in-class initializer and its lifetime takes care of itself.
class PfRouteMonitor
{
public:
    using MessageHandler = std::function<void(const char* buf, size_t n)>;

    PfRouteMonitor(std::string label, MessageHandler handler);
    PfRouteMonitor(const PfRouteMonitor&) = delete;
    PfRouteMonitor& operator=(const PfRouteMonitor&) = delete;
    ~PfRouteMonitor();

private:
    void loop();

    std::string label_;
    MessageHandler handler_;
    sockpp::socket sock_;
    int stopPipe_[2] = { -1, -1 };
    std::thread thread_;
};

class NmWorkerBase
{
protected:
    // Serializes all operations run on a single worker instance, so that an
    // async job's background task (see execCmdAsync) never runs concurrently
    // with another command sharing the same worker's internal state.
    std::atomic<bool> worker_access{false};

    // Generic async request/response helper, reusable by any worker/command:
    // validates SOCKET+REQUEST on pcmd, test-connects to the client's callback
    // socket, acks synchronously, then runs syncExec on async++'s thread pool
    // (holding worker_access, released once syncExec returns) and delivers
    // {RESULT,REQUEST} to the client socket once it completes - same
    // fire-and-forget delivery as publishEvent().
    json execCmdAsync(NmCommandData* pcmd, std::function<json(NmCommandData*)> syncExec);

    // One registered SUBSCRIBE: where to deliver this worker's events, and
    // the optional second-level DATA filter (empty = match any event).
    struct Subscription
    {
        std::string socketPath;
        json dataFilter;
    };
    // Protects subscriptions: execCmdSubscribe (dispatcher thread, one call
    // per SUBSCRIBE request) and publishEvent (may be called from a
    // background thread once something actually calls it - e.g. a future
    // async job noticing an external change) can run concurrently.
    std::mutex subscriptions_mutex;
    std::vector<Subscription> subscriptions;

    // Generic SUBSCRIBE handler, reusable by any worker: validates SOCKET on
    // pcmd (same client-socket hardening as execCmdAsync) and registers it,
    // along with an optional DATA filter, against this worker's own scope
    // (getScope()). publishEvent() is the primitive a command calls on
    // success to notify matching subscribers, after that command actually
    // changed something; it delivers to each matching subscriber in turn
    // (one-shot, same delivery pattern as an async notification, but off
    // the calling thread - see publishEvent's own definition) with
    // { EVENT: <cmd>, SCOPE: getScope(), DATA: data }, where a subscriber's
    // own DATA filter (if any) must be a subset of `data` to match.
    json execCmdSubscribe(NmCommandData* pcmd);
    // Generic UNSUBSCRIBE handler, mirroring execCmdSubscribe: removes the
    // subscription whose SOCKET and (optional) DATA filter exactly match
    // the request - the same {socketPath, dataFilter} pair execCmdSubscribe
    // itself dedups on, so "subscribe, then unsubscribe with the identical
    // request" is the exact inverse. Deliberately skips the client-socket
    // directory/ownership/test-connect checks execCmdSubscribe applies:
    // this needs to work even for a socket that's gone stale (the client
    // already exited), since that's often exactly why it's being called.
    // Returns SUCCESS if a matching subscription was removed, NOTFOUND if
    // none matched (not an error - the client's desired end state, "not
    // subscribed", was already true).
    json execCmdUnsubscribe(NmCommandData* pcmd);
    void publishEvent(NmCmd cmd, const json& data = json::object());
    // Cheap existence check for a caller that can skip real work (e.g.
    // decoding a kernel event) entirely when nobody could possibly be
    // listening - a true result doesn't guarantee any one subscriber's own
    // DATA filter will actually match, only that publishEvent() isn't
    // guaranteed to be a no-op.
    bool hasSubscribers();

public:
    virtual ~NmWorkerBase() {}
    virtual NmScope getScope() = 0;
    virtual json execCmd(NmCommandData*) = 0;
    virtual bool isValidCmd(NmCommandData*) = 0;
};

#endif // NMWORKERBASE_H
