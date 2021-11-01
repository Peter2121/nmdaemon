#include <iostream>
//#include <stdio.h>
//#include <string.h>
#include <unistd.h>
//#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
//#include <stdlib.h>
#include <thread>

#define LOGURU_WITH_STREAMS 1
#include "loguru/loguru.hpp"
#include "json/json.hpp"

#include "sockpp/unix_acceptor.h"
#include "sockpp/version.h"

#include "nmdaemon.h"
#include "nmworker.h"
#include "rcconf.h"
#include "workers.h"
#if defined (WORKER_DUMMY)
#include "dummy_worker.h"
#endif
#if defined (WORKER_SYSTEM)
#include "system_worker.h"
#endif
#if defined (WORKER_INTERFACE)
#include "if_worker.h"
#endif
#if defined (WORKER_ROUTE)
#include "route_worker.h"
#endif
#if defined (WORKER_WPA)
#include "wpa_worker.h"
#endif


/*
void run_echo(sockpp::unix_socket sock)
{
    int n;
    char buf[512];

    while ((n = sock.read(buf, sizeof(buf))) > 0)
        sock.write_n(buf, n);

    LOG_S(INFO) << "Connection closed";
}
*/

bool needShutdown;

void signalCleaner(int signum)
{
    LOG_S(WARNING) << "Interrupt signal " << signum << " received, cleaning up...";
    needShutdown = true;
   // cleanup and close up stuff here
   // terminate program

    exit(signum);
}

int main(int argc, char* argv[])
{
    const string DEFAULT_SOCKET_PATH = "/var/run/nmd.socket";

//    const int OPT_ON = 1;
    const int MAXBUF = 512;
    char buf[MAXBUF];
    bool res;
    std::vector<nmdaemon*> daemons;
    std::vector<nmworker*> workers;
    std::vector<std::thread*> threads;
    std::string sock_path = DEFAULT_SOCKET_PATH;
    struct sockaddr_un sock_addr;
    memset(&sock_addr, 0, sizeof(struct sockaddr_un));
    sock_addr.sun_family = AF_UNIX;
    memset(&buf, 0, MAXBUF*sizeof(char));

    loguru::init(argc, argv);

    if ( getuid() != 0 )
    {
        LOG_S(WARNING) << "This program must be run as root!";
    }

    //*******************
    /*
    string s;
    getline(cin, s);
    rcconf* rc_conf = new rcconf("/home/peter/Programming/nmdebug/rc.conf");
    if(!rc_conf->iniLoad())
    {
        LOG_S(ERROR) << "Cannot load rc.conf";
        return -1;
    }
    json rcjson = rc_conf->getRcIpConfig();
    if(!rcjson.empty())
    {
        LOG_S(INFO) << rcjson.dump(4);
//        exit(0);
    }
    else
        exit(-1);
    */
    //*******************

    signal(SIGINT, signalCleaner);
    signal(SIGABRT, signalCleaner);
    signal(SIGFPE, signalCleaner);
    signal(SIGILL, signalCleaner);
    signal(SIGSEGV, signalCleaner);
    signal(SIGTERM, signalCleaner);

#if defined (WORKER_DUMMY)
    workers.push_back(new WORKER_DUMMY());
#endif

#if defined (WORKER_SYSTEM)
    workers.push_back(new WORKER_SYSTEM());
#endif

#if defined (WORKER_INTERFACE)
    workers.push_back(new WORKER_INTERFACE());
#endif

#if defined (WORKER_ROUTE)
    workers.push_back(new WORKER_ROUTE());
#endif

#if defined (WORKER_WPA)
    workers.push_back(new WORKER_WPA());
#endif

    sockpp::socket_initializer sockInit;
    sockpp::unix_acceptor sockAcc;

    unlink(sock_path.c_str());
    res = sockAcc.open(sockpp::unix_address(sock_path));

    if (!res)
    {
        LOG_S(ERROR) << "Error creating the acceptor on " << sock_path << " : " << sockAcc.last_error_str();
        LOG_S(ERROR) << "If no other instance of nmdaemon is running - you can try to delete " << sock_path << " and restart nmdaemon";
        return -1;
    }
    LOG_S(INFO) << "Acceptor bound to address: '" << sockAcc.address();

    if(chmod(sock_path.c_str(), S_IRUSR|S_IWUSR|S_ISGID|S_IRGRP|S_IWGRP)<0)
    {
        LOG_S(WARNING) << "Cannot change permissions for socket: '" << sockAcc.address();
    }

    needShutdown = false;
    while (true)
    {
        auto sock = sockAcc.accept();
        LOG_S(INFO) << "Received a connection";

        if (!sock) {
            LOG_S(ERROR) << "Error accepting incoming connection: " << sockAcc.last_error_str();
        }
        else {
            nmdaemon* daemon = new nmdaemon(workers);
            daemons.push_back(daemon);
            std::thread thr(&nmdaemon::sock_receiver, daemon, move(sock));
            threads.push_back(&thr);
            thr.detach();
        }
        if(needShutdown)
        {
            LOG_S(WARNING) << "Escaping from main thread";
            break;
        }
    }
    for(auto daemon : daemons)
    {
        daemon->shutdown();
    }
    for(auto pthr : threads)
    {
        if(pthr->joinable())
            pthr->join();
    }
    if(unlink(sock_path.c_str())!=0)
    {
        strerror_r(errno, buf, MAXBUF);
        LOG_S(ERROR) << "Cannot destroy socket " << sock_path << " : " << buf;
    }
    if(remove(sock_path.c_str())!=0)
    {
        strerror_r(errno, buf, MAXBUF);
        LOG_S(ERROR) << "Cannot remove socket file " << sock_path << " : " << buf;
    }
    return 0;
}
