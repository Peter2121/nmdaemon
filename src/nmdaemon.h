#ifndef NMDAEMON_H
#define NMDAEMON_H

#include <string>
#include <queue>
#include <thread>
#include <mutex>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ucred.h>

//#define LOGURU_WITH_STREAMS 1
#include "loguru/loguru.hpp"

#include "sockpp/unix_acceptor.h"
#include "sockpp/version.h"

#include "nmcommanddata.h"
#include "nmworkerbase.h"
#include "nmconfig.h"

class NmDaemon
{
protected:
    const unsigned long NM_MAXBUF = 8192;
    static std::mutex work_access;
    std::atomic<bool> running_sock_receiver;
    std::atomic<bool> running_dispatcher;
    std::atomic<bool> stop_dispatcher;
    std::atomic<bool> stop_receiver;
    std::mutex req_access;
    std::mutex sock_access_write;
    std::queue<NmCommandData*> requests;
    std::vector<NmWorkerBase*> workers;
    void logPeerCredentials(const sockpp::unix_socket&);

public:
    NmDaemon(std::vector<NmWorkerBase*>);
    void sock_receiver(sockpp::unix_socket);
    void dispatcher(sockpp::unix_socket);
    void shutdown();
};

#endif // NMDAEMON_H
