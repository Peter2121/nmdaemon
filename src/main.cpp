#include <iostream>
//#include <stdio.h>
//#include <string.h>
#include <unistd.h>
//#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
//#include <stdlib.h>
#include <thread>

#define LOGURU_WITH_STREAMS 1
#include "loguru/loguru.hpp"
#include "json/json.hpp"

#include "sockpp/unix_acceptor.h"
#include "sockpp/version.h"

#include "nmdaemon.h"
#include "nmworkerbase.h"

std::shared_ptr<NmConfig> sp_conf;

#include "rcconf.h"
#include "workers.h"
#if defined (WORKER_DUMMY)
#include "nmworkerdummy.h"
#endif
#if defined (WORKER_SYSTEM)
#include "system_worker.h"
#endif
#if defined (WORKER_INTERFACE)
#include "nmworkerif.h"
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

static constexpr char *PROG_NAME = (char *)"nmdaemon";
static inline const std::string DEFAULT_CONF_FILENAME = "/usr/local/etc/nmdaemon.conf";

static inline const std::string CONF_SECT_NMDAEMON = "nmdaemon";
static inline const std::string CONF_KEY_SOCKET_PATH = "socket_path";
static inline const std::string CONF_KEY_SOCKET_USER = "socket_user";
static inline const std::string CONF_KEY_SOCKET_GROUP = "socket_group";
static inline const std::string CONF_KEY_SOCKET_MOD = "socket_mod";
static inline const std::string CONF_KEY_LOG_LEVEL = "stderr_log_level";

static inline const std::string DEFAULT_SOCKET_PATH = "/var/run/nmd.socket";
static inline const std::string DEFAULT_SOCKET_USER = "root";
static inline const std::string DEFAULT_SOCKET_GROUP = "wheel";
static inline const std::string DEFAULT_SOCKET_MOD = "660";
static inline const std::string DEFAULT_LOG_LEVEL = "INFO";

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
//    const int OPT_ON = 1;
    const int MAXBUF = 512;
    char buf[MAXBUF];
    bool res;
    std::vector<NmDaemon*> daemons;
    std::vector<NmWorkerBase*> workers;
    std::vector<std::thread*> threads;
    std::string conf_value;
    std::string config_filename = DEFAULT_CONF_FILENAME;
    std::string sock_path = DEFAULT_SOCKET_PATH;
    std::string sock_user = DEFAULT_SOCKET_USER;
    std::string sock_group = DEFAULT_SOCKET_GROUP;
    std::string sock_mod = DEFAULT_SOCKET_MOD;
    std::string log_level = DEFAULT_LOG_LEVEL;

    int loginit_argc = 3;
    const char *loginit_argv[] = { PROG_NAME, (char *)"-v", (char *)"INFO" };
    uid_t socket_uid = 0;
    gid_t socket_gid = 0;
    struct passwd *ppasswd = nullptr;
    struct group *pgroup = nullptr;
    unsigned int sock_mode = 0;

    struct sockaddr_un sock_addr;
    memset(&sock_addr, 0, sizeof(struct sockaddr_un));
    sock_addr.sun_family = AF_UNIX;
    memset(&buf, 0, MAXBUF*sizeof(char));
    sp_conf = nullptr;

    if(argc>1)
    {
//      Actually we process only the first argument - the name of configuration file
        config_filename = argv[1];
    }

    sp_conf = std::make_shared<NmConfig>(config_filename);
    try
    {
        if(!sp_conf->iniLoad())
        {
            std::cout << "Cannot load configuration file: " << config_filename << std::endl;
            std::cout << "Continue with default settings..." << std::endl;
        }
        else
        {
            std::cout << "Using settings from " << config_filename << std::endl;
            conf_value = sp_conf->getConfigValue(CONF_SECT_NMDAEMON, CONF_KEY_SOCKET_PATH);
            if(!conf_value.empty())
                sock_path = conf_value;
            conf_value = sp_conf->getConfigValue(CONF_SECT_NMDAEMON, CONF_KEY_SOCKET_USER);
            if(!conf_value.empty())
                sock_user = conf_value;
            conf_value = sp_conf->getConfigValue(CONF_SECT_NMDAEMON, CONF_KEY_SOCKET_GROUP);
            if(!conf_value.empty())
                sock_group = conf_value;
            conf_value = sp_conf->getConfigValue(CONF_SECT_NMDAEMON, CONF_KEY_SOCKET_MOD);
            if(!conf_value.empty())
                sock_mod = conf_value;
            conf_value = sp_conf->getConfigValue(CONF_SECT_NMDAEMON, CONF_KEY_LOG_LEVEL);
            if(!conf_value.empty())
            {
                log_level = conf_value;
                loginit_argv[2] = log_level.c_str();
            }
        }
    }  catch (std::exception e)
    {
        std::cerr << "Exception loading configuration file: " << config_filename << std::endl;
        std::cerr << e.what() << std::endl;
        std::cout << "Exception loading configuration file: " << config_filename << std::endl;
        std::cout << e.what() << std::endl;
        return -1;  // Maybe it would be OK to continue here?
    }
    loguru::init(loginit_argc, (char **)loginit_argv);
/*
    LOG_S(ERROR) << "Test log ERROR";
    LOG_S(WARNING) << "Test log WARNING";
    LOG_S(INFO) << "Test log INFO";
*/
    if ( getuid() != 0 )
    {
        LOG_S(WARNING) << "This program must be run as root!";
        LOG_S(WARNING) << "Some functions will not run as expected!";
    }

    errno = 0;
    ppasswd = getpwnam(sock_user.c_str());
    if(ppasswd != nullptr)
    {
        socket_uid = ppasswd->pw_uid;
    }
    else
    {
        LOG_S(ERROR) << "Cannot get UID of user " << sock_user << " for socket: " << strerror(errno);
        return -1;
    }

    errno = 0;
    pgroup = getgrnam(sock_group.c_str());
    if(pgroup != nullptr)
    {
        socket_gid = pgroup->gr_gid;
    }
    else
    {
        LOG_S(ERROR) << "Cannot get GID of group " << sock_group << " for socket: " << strerror(errno);
        return -1;
    }
/*
    signal(SIGINT, signalCleaner);
    signal(SIGABRT, signalCleaner);
    signal(SIGFPE, signalCleaner);
    signal(SIGILL, signalCleaner);
    signal(SIGSEGV, signalCleaner);
    signal(SIGTERM, signalCleaner);
*/

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

    LOG_S(INFO) << "Trying to chown the socket to " << socket_uid << ":" << socket_gid;
    if(chown(sock_path.c_str(), socket_uid, socket_gid)<0)
    {
        LOG_S(ERROR) << "Error setting owner of socket: " << strerror(errno);
        sockAcc.close();
        return -1;
    }

    sock_mode = std::stoi(sock_mod, nullptr, 8);
    LOG_S(INFO) << "Trying to chmod the socket to " << sock_mode;
    if(chmod(sock_path.c_str(), sock_mode)<0)
    {
        LOG_S(WARNING) << "Cannot setting permissions for socket: " << strerror(errno);
        sockAcc.close();
        return -1;
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
            NmDaemon* daemon = new NmDaemon(workers);
            daemons.push_back(daemon);
            std::thread thr(&NmDaemon::sock_receiver, daemon, std::move(sock));
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
