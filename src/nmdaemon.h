#ifndef NMDAEMON_H
#define NMDAEMON_H

#include <string>
#include <queue>
#include <thread>

#define LOGURU_WITH_STREAMS 1
#include "loguru/loguru.hpp"

#include "sockpp/unix_acceptor.h"
#include "sockpp/version.h"

#include "nmcommand_data.h"
#include "nmworker.h"

using namespace std;

class nmdaemon
{
protected:
    const unsigned long NM_MAXBUF = 8192;
    bool running_sock_receiver;
    bool running_dispatcher;
    bool stop_dispatcher;
    bool stop_receiver;
    mutex req_access;
    mutex sock_access_write;
    mutex work_access;
    queue<nmcommand_data*> requests;
    vector<nmworker*> workers;

public:
    nmdaemon(vector<nmworker*>);
    void sock_receiver(sockpp::unix_socket);
    void dispatcher(sockpp::unix_socket);
    void shutdown();
};

#endif // NMDAEMON_H
