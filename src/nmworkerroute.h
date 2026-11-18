#ifndef NMWORKERROUTE_H
#define NMWORKERROUTE_H

#include <unistd.h>
#include <sys/socket.h>
#include <net/route.h>
#include <sys/sysctl.h>
//#define LOGURU_WITH_STREAMS 1
#include "loguru/loguru.hpp"
#include "sockpp/socket.h"
#include "sockpp/version.h"
#include "nmworkerbase.h"
#include "interface.h"
#include "addressgroup.h"
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

// Same leading layout as static_route/static_route6, with a trailing RTA_IFP
// sockaddr_dl used only when a specific outgoing interface ('-ifp') is forced;
// when it is not, the message is sent truncated right before this member
// (see offsetof(..., ifp) in setStaticRoute).
typedef struct
{
      struct rt_msghdr head;
      struct sockaddr_in dest;
      struct sockaddr_in gateway;
      struct sockaddr_in netmask;
      struct sockaddr_dl ifp;
} static_route_ifp;

typedef struct
{
      struct rt_msghdr head;
      struct sockaddr_in6 dest;
      struct sockaddr_in6 gateway;
      struct sockaddr_in6 netmask;
      struct sockaddr_dl ifp;
} static_route6_ifp;

class NmWorkerRoute : public NmWorkerBase
{
protected:
    static constexpr NmCommand Cmds[] =
    {
        { NmScope::ROUTE, NmCmd::RT_GET },
        { NmScope::ROUTE, NmCmd::RT_DEF_GET },
        { NmScope::ROUTE, NmCmd::RT_DEF6_GET },
        { NmScope::ROUTE, NmCmd::RT_SET },
        { NmScope::ROUTE, NmCmd::RT_DEF_SET },
        { NmScope::ROUTE, NmCmd::RT_DEL },
        { NmScope::ROUTE, NmCmd::RT_DEF_DEL },
        { NmScope::ROUTE, NmCmd::RT_LIST },
        { NmScope::ROUTE, NmCmd::RT_LIST6 },
        { NmScope::ROUTE, NmCmd::SUBSCRIBE },
        { NmScope::ROUTE, NmCmd::UNSUBSCRIBE }
    };
    bool setStaticRoute(std::shared_ptr<AddressGroup>, const std::string& ifname = std::string());
    bool delStaticRoute(std::shared_ptr<AddressGroup>);
    std::unique_ptr<Interface> getStaticRoute(std::shared_ptr<AddressGroup>);
    // Core of RT_LIST/RT_LIST6 (dumps the kernel routing table for the given
    // address family), factored out so it can be called with a plain family
    // instead of an NmCommandData.
    json getRouteListJson(int family);
//    bool getStaticRouteN(std::shared_ptr<addr>);
    void setPsaStruct(sockaddr_in *, const std::shared_ptr<AddressBase>);
    void setPsaStruct6(sockaddr_in6 *, const std::shared_ptr<AddressBase>);
    void setPsaStructIfp(struct sockaddr_dl *, const std::string& ifname);

    // Decodes one RTM_ADD/DELETE/CHANGE message - built directly from its
    // own trailing DST/GATEWAY/NETMASK sockaddrs (same parsing getRouteListJson()
    // and getStaticRoute() already do for the same message shape), not a
    // follow-up kernel re-query - into the EVENT/DATA this worker's
    // publishEvent() should fire. Returns false (nothing published) for a
    // message type this worker doesn't care about, a truncated message, or
    // one with no usable destination address.
    bool decodeRouteEvent(const char* buf, size_t n, NmCmd& outCmd, json& outData);
    // Long-lived PF_ROUTE listener for ROUTE-scope kernel events, same
    // model as NmWorkerIf's/NmWorkerSys's/NmWorkerIeee80211's own.
    PfRouteMonitor routeMonitor;
public:
    NmWorkerRoute();
    ~NmWorkerRoute();
    NmScope getScope();
    json execCmd(NmCommandData*);
    bool isValidCmd(NmCommandData*);
    json execCmdRouteSet(NmCommandData*);
    json execCmdRouteGet(NmCommandData*);
    json execCmdDefRouteSet(NmCommandData*);
    json execCmdDefRouteGet(NmCommandData*);
    json execCmdDefRouteGet6(NmCommandData*);
    json execCmdRouteList(NmCommandData*);
    json execCmdRouteDel(NmCommandData*);
    json execCmdDefRouteDel(NmCommandData*);
//    json execCmdRouteList6(nmcommand_data*);
};

#endif // NMWORKERROUTE_H
