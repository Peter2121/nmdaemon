#ifndef ROUTE_WORKER_H
#define ROUTE_WORKER_H

#include <unistd.h>
#include <sys/socket.h>
#include <net/route.h>
#include <sys/sysctl.h>
//#define LOGURU_WITH_STREAMS 1
#include "loguru/loguru.hpp"
#include "sockpp/socket.h"
#include "sockpp/version.h"
#include "nmworker.h"
#include "interface.h"
#include "addr.h"
#include "tool.h"

typedef struct
{
      struct rt_msghdr head;
      struct sockaddr_in dest;
      struct sockaddr_in gateway;
      struct sockaddr_in netmask;
} static_route;

typedef struct
{
      struct rt_msghdr head;
      struct sockaddr_in6 dest;
      struct sockaddr_in6 gateway;
      struct sockaddr_in6 netmask;
} static_route6;

typedef struct
{
      struct rt_msghdr head;
      struct sockaddr_dl dest;
      struct sockaddr_dl gateway;
      struct sockaddr_dl netmask;
} static_route_link;

class route_worker : public nmworker
{
protected:
    static constexpr nmcommand Cmds[] =
    {
        { nmscope::ROUTE, nmcmd::RT_GET },
        { nmscope::ROUTE, nmcmd::RT_DEF_GET },
        { nmscope::ROUTE, nmcmd::RT_DEF6_GET },
        { nmscope::ROUTE, nmcmd::RT_SET },
        { nmscope::ROUTE, nmcmd::RT_DEF_SET },
        { nmscope::ROUTE, nmcmd::RT_DEL },
        { nmscope::ROUTE, nmcmd::RT_DEF_DEL },
        { nmscope::ROUTE, nmcmd::RT_LIST },
        { nmscope::ROUTE, nmcmd::RT_LIST6 }
    };
    bool setStaticRoute(std::shared_ptr<addr>);
    bool delStaticRoute(std::shared_ptr<addr>);
    std::unique_ptr<interface> getStaticRoute(std::shared_ptr<addr>);
//    bool getStaticRouteN(std::shared_ptr<addr>);
    void setPsaStruct(sockaddr_in *, const std::shared_ptr<address_base>);
    void setPsaStruct6(sockaddr_in6 *, const std::shared_ptr<address_base>);
public:
    route_worker();
    ~route_worker();
    nmscope getScope();
    json execCmd(nmcommand_data*);
    bool isValidCmd(nmcommand_data*);
    json execCmdRouteSet(nmcommand_data*);
    json execCmdRouteGet(nmcommand_data*);
    json execCmdDefRouteSet(nmcommand_data*);
    json execCmdDefRouteGet(nmcommand_data*);
    json execCmdDefRouteGet6(nmcommand_data*);
    json execCmdRouteList(nmcommand_data*);
    json execCmdRouteDel(nmcommand_data*);
    json execCmdDefRouteDel(nmcommand_data*);
//    json execCmdRouteList6(nmcommand_data*);
};

#endif // ROUTE_WORKER_H
