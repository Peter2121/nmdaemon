#ifndef NMWORKERBASE_H
#define NMWORKERBASE_H

#include <atomic>
#include <functional>

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

class NmWorkerBase
{
protected:
    // Serializes all operations run on a single worker instance, so that an
    // async job's background thread (see execCmdAsync) never runs concurrently
    // with another command sharing the same worker's internal state.
    std::atomic<bool> worker_access{false};

    // Generic async request/response helper, reusable by any worker/command:
    // validates SOCKET+REQUEST on pcmd, test-connects to the client's callback
    // socket, acks synchronously, then runs syncExec on a detached thread
    // (holding worker_access) and delivers {RESULT,REQUEST} to the client
    // socket once it completes.
    json execCmdAsync(NmCommandData* pcmd, std::function<json(NmCommandData*)> syncExec);

public:
    virtual ~NmWorkerBase() {}
    virtual NmScope getScope() = 0;
    virtual json execCmd(NmCommandData*) = 0;
    virtual bool isValidCmd(NmCommandData*) = 0;
};

#endif // NMWORKERBASE_H
