#include "route_worker.h"

route_worker::route_worker()
{ }

route_worker::~route_worker()
{ }

nmscope route_worker::getScope()
{
    return nmscope::ROUTE;
}

json route_worker::execCmd(nmcommand_data* pcmd)
{
    switch (pcmd->getCommand().cmd)
    {
        case nmcmd::RT_GET :
            return execCmdRouteGet(pcmd);
        case nmcmd::RT_DEF_GET :
            return execCmdDefRouteGet(pcmd);
        case nmcmd::RT_SET :
            return execCmdRouteSet(pcmd);
        case nmcmd::RT_DEF_SET :
            return execCmdDefRouteSet(pcmd);
        case nmcmd::RT_DEF6_GET :
        case nmcmd::RT_REMOVE :
        case nmcmd::RT_DEF_REMOVE :
        case nmcmd::RT_LIST :
            return { { JSON_PARAM_RESULT, JSON_PARAM_ERR }, {JSON_PARAM_ERR, JSON_DATA_ERR_NOT_IMPLEMENTED} };
        default :
            return { { JSON_PARAM_RESULT, JSON_PARAM_ERR }, {JSON_PARAM_ERR, JSON_DATA_ERR_INVALID_COMMAND} };
    }
}

bool route_worker::isValidCmd(nmcommand_data* pcmd)
{
    if( pcmd->getCommand().scope != getScope() )
        return false;

    for(auto cm : Cmds)
    {
        if ( cm.cmd == pcmd->getCommand().cmd )
            return true;
    }

    return false;
}

void route_worker::setPsaStruct(sockaddr_in *psa, const address_base* strt)
{
    psa->sin_len = sizeof(struct sockaddr_in);
    psa->sin_family = strt->getFamily();
    memcpy(psa, strt->getSockAddr(), sizeof(struct sockaddr_in));
}

void route_worker::setPsaStruct6(sockaddr_in6 *psa6, const address_base* strt)
{
    psa6->sin6_len = sizeof(struct sockaddr_in6);
    psa6->sin6_family = strt->getFamily();
    memcpy(psa6, strt->getSockAddr(), sizeof(struct sockaddr_in6));
}

bool route_worker::setStaticRoute(addr* stroute)
{
    static_route* pnew_route=nullptr;
    static_route6* pnew_route6=nullptr;
    void* pnr=nullptr;
    size_t pnr_size=0;
    struct rt_msghdr *prtm_hdr=nullptr;
    short fam = stroute->getAddrAB()->getFamily();
    time_t curtime = time(NULL);
    struct tm *info = localtime(&curtime);
    int seq = 3600*info->tm_hour + 60*info->tm_min + info->tm_sec;

    if(fam == AF_INET)
    {
        pnew_route = new static_route;
        memset(pnew_route, 0x00, sizeof(static_route));
        prtm_hdr = &(pnew_route->head);
        prtm_hdr->rtm_msglen = sizeof(static_route);
    }
    else if(fam == AF_INET6)
    {
        pnew_route6 = new static_route6;
        memset(pnew_route6, 0x00, sizeof(static_route6));
        prtm_hdr = &(pnew_route6->head);
        prtm_hdr->rtm_msglen = sizeof(static_route6);
    }
    else
        return false;

    prtm_hdr->rtm_type = RTM_ADD;
    prtm_hdr->rtm_flags = RTF_UP | RTF_GATEWAY | RTF_STATIC;
    prtm_hdr->rtm_version = RTM_VERSION;
    prtm_hdr->rtm_seq = seq;
    prtm_hdr->rtm_addrs = RTA_DST | RTA_GATEWAY | RTA_NETMASK;
    prtm_hdr->rtm_pid = getpid();

    if(fam == AF_INET)
    {
        setPsaStruct(&(pnew_route->dest), stroute->getAddrAB());
        setPsaStruct(&(pnew_route->gateway), stroute->getDataAB());
        setPsaStruct(&(pnew_route->netmask), stroute->getMaskAB());
        pnr=pnew_route;
        pnr_size = sizeof(static_route);
    }
    else if(fam == AF_INET6)
    {
        setPsaStruct6(&(pnew_route6->dest), stroute->getAddrAB());
        setPsaStruct6(&(pnew_route6->gateway), stroute->getDataAB());
        setPsaStruct6(&(pnew_route6->netmask), stroute->getMaskAB());
        pnr=pnew_route6;
        pnr_size = sizeof(static_route6);
    }

    sockpp::socket sock = sockpp::socket::create(AF_ROUTE, SOCK_RAW);

DO_WRITE:
    if(write(sock.handle(), pnr, pnr_size) < 0)
    {
           if( (errno == EEXIST) && (prtm_hdr->rtm_type == RTM_ADD) )
           {
                    prtm_hdr->rtm_type = RTM_CHANGE;
                    goto DO_WRITE;
           }
           LOG_S(ERROR) << "setStaticRoute cannot write to socket";
           sock.close();
           return false;
    }
    sock.close();
    return true;
}

bool route_worker::getStaticRoute(addr* stroute)
{
    static_route* pcur_route=nullptr;
    static_route6* pcur_route6=nullptr;
    struct rt_msghdr *prtm_hdr=nullptr;
    void* pnr=nullptr;
    size_t pnr_size=0;
    address_base* paddr_gw=nullptr;
    short fam = stroute->getAddrAB()->getFamily();
    short fam_gw = 0;
    time_t curtime = time(NULL);
    struct tm *info = localtime(&curtime);
    int seq = 3600*info->tm_hour + 60*info->tm_min + info->tm_sec;

    if(fam == AF_INET)
    {
        pcur_route = new static_route;
        memset(pcur_route, 0x00, sizeof(static_route));
        prtm_hdr = &(pcur_route->head);
        prtm_hdr->rtm_msglen = sizeof(static_route);
    }
    else if(fam == AF_INET6)
    {
        pcur_route6 = new static_route6;
        memset(pcur_route6, 0x00, sizeof(static_route6));
        prtm_hdr = &(pcur_route6->head);
        prtm_hdr->rtm_msglen = sizeof(static_route6);
    }
    else
        return false;

    prtm_hdr->rtm_type = RTM_GET;
    prtm_hdr->rtm_flags |= RTF_GATEWAY;
    prtm_hdr->rtm_version = RTM_VERSION;
    prtm_hdr->rtm_seq = seq;
    prtm_hdr->rtm_pid = getpid();
    prtm_hdr->rtm_addrs = RTA_DST | RTA_GATEWAY | RTA_NETMASK;

    if(fam == AF_INET)
    {
        setPsaStruct(&(pcur_route->dest), stroute->getAddrAB());
        setPsaStruct(&(pcur_route->netmask), stroute->getMaskAB());
        pcur_route->gateway.sin_len = sizeof(struct sockaddr_in);
        pcur_route->gateway.sin_family = fam;
        pnr=pcur_route;
        pnr_size = sizeof(static_route);
    }
    else if(fam == AF_INET6)
    {
        setPsaStruct6(&(pcur_route6->dest), stroute->getAddrAB());
        setPsaStruct6(&(pcur_route6->netmask), stroute->getMaskAB());
        pcur_route6->gateway.sin6_len = sizeof(struct sockaddr_in6);
        pcur_route6->gateway.sin6_family = fam;
        pnr=pcur_route6;
        pnr_size = sizeof(static_route6);
    }

    sockpp::socket sock = sockpp::socket::create(AF_ROUTE, SOCK_RAW);
    if(write(sock.handle(), pnr, pnr_size) < 0)
    {
        LOG_S(ERROR) << "getStaticRoute cannot write to socket";
        sock.close();
        return false;
    }
    if(read(sock.handle(), pnr, pnr_size) == -1)
    {
        LOG_S(ERROR) << "getStaticRoute cannot read from socket";
        sock.close();
        return false;
    }
    sock.close();

    fam_gw = pcur_route->gateway.sin_family;
    try
    {
        switch(fam_gw)
        {
            case AF_INET:
                paddr_gw = new address_ip4((sockaddr_in*)&(pcur_route->gateway));
                stroute->setData(paddr_gw, true);
                break;
            case AF_INET6:
                paddr_gw = new address_ip6((sockaddr_in6*)&(pcur_route6->gateway));
                stroute->setData(paddr_gw, true);
                break;
            case AF_LINK:
                paddr_gw = new address_link((sockaddr_dl*)&(pcur_route->gateway));
                stroute->setData(paddr_gw, true);
                break;
            default:
                LOG_S(ERROR) << "getStaticRoute received unknown router family from system: " << fam_gw;
                return false;
        }
        return true;
    }
    catch(std::exception& e)
    {
        LOG_S(ERROR) << "getStaticRoute cannot decode router data received from system";
        return false;
    }
}

json route_worker::execCmdRouteGet(nmcommand_data* pcmd)
{
    addr* rt_addr = nullptr;
    json cmd = {};
    json res_route = {};
    short family;
    std::string strGw = "";

    try {
        cmd = pcmd->getJsonData();
    } catch (std::exception& e) {
        LOG_S(ERROR) << "Exception in execCmdRouteGet - cannot get interface parameters";
        return JSON_RESULT_ERR;
    }

    rt_addr = tool::getAddrFromJson(cmd);
    if(rt_addr==nullptr)
    {
        LOG_S(ERROR) << "Cannot decode JSON data";
        return JSON_RESULT_ERR;
    }

    if( !route_worker::getStaticRoute(rt_addr) ) {
        LOG_S(ERROR) << "Error in execCmdRouteGet - cannot get route";
        return JSON_RESULT_ERR;
    }

    family = rt_addr->getDataAB()->getFamily();
    strGw = rt_addr->getDataAB()->getStrAddr();

    switch(family)
    {
    case AF_INET:
        res_route[JSON_PARAM_RESULT] = JSON_PARAM_SUCC;
        res_route[JSON_PARAM_DATA] = { { JSON_PARAM_IPV4_GW, strGw } };
        break;
    case AF_INET6:
        res_route[JSON_PARAM_RESULT] = JSON_PARAM_SUCC;
        res_route[JSON_PARAM_DATA] = { { JSON_PARAM_IPV6_GW, strGw } };
        break;
    case AF_LINK:
        res_route[JSON_PARAM_RESULT] = JSON_PARAM_SUCC;
        res_route[JSON_PARAM_DATA] = { { JSON_PARAM_LINK_ADDR, strGw } };
        break;
    }
    return res_route;
}

json route_worker::execCmdDefRouteGet(nmcommand_data*)
{
    std::string strDefAddr = "0.0.0.0";
    json res_route = {};
    short family;
    std::string strGw = "";
//    std::string strDefAddr6 = "0:0:0:0:0:0:0:0";

    address_ip4* paddr = new address_ip4(strDefAddr);
    address_ip4* pmask = new address_ip4(strDefAddr);
    address_ip4* pgate = new address_ip4(strDefAddr);
    addr* rt_addr = new addr(paddr, pmask, pgate, ipaddr_type::PPP);

    if( !route_worker::getStaticRoute(rt_addr) ) {
        LOG_S(ERROR) << "Error in execCmdDefRouteGet - cannot get route";
        delete rt_addr;
        delete paddr;
        delete pmask;
        delete pgate;
        return JSON_RESULT_ERR;
    }

    family = rt_addr->getDataAB()->getFamily();
    strGw = rt_addr->getDataAB()->getStrAddr();

    switch(family)
    {
    case AF_INET:
        res_route[JSON_PARAM_RESULT] = JSON_PARAM_SUCC;
        res_route[JSON_PARAM_DATA] = { { JSON_PARAM_IPV4_GW, strGw } };
        break;
    case AF_INET6:
        res_route[JSON_PARAM_RESULT] = JSON_PARAM_SUCC;
        res_route[JSON_PARAM_DATA] = { { JSON_PARAM_IPV6_GW, strGw } };
        break;
    }

    delete rt_addr;
    delete paddr;
    delete pmask;
    delete pgate;
    return res_route;
}

json route_worker::execCmdRouteSet(nmcommand_data* pcmd)
{
    json cmd = {};
    addr* rt_addr = nullptr;

    cmd = pcmd->getJsonData();

    rt_addr = tool::getAddrFromJson(cmd);
    if(rt_addr==nullptr)
    {
        LOG_S(ERROR) << "Cannot decode JSON data";
        return JSON_RESULT_ERR;
    }

    if(!route_worker::setStaticRoute(rt_addr))
    {
        LOG_S(ERROR) << "Cannot set route";
        return JSON_RESULT_ERR;
    }

    LOG_S(INFO) << "Static route set";
    delete rt_addr;
    return JSON_RESULT_SUCCESS;
}

json route_worker::execCmdDefRouteSet(nmcommand_data* pcmd)
{
    json cmd = {};
    addr* rt_addr = nullptr;
    std::string strDefAddr4 = "0.0.0.0";
    std::string strDefAddr6 = "0:0:0:0:0:0:0:0";

    cmd = pcmd->getJsonData();

    if(!cmd.contains(JSON_PARAM_DATA))
    {
        LOG_S(ERROR) << "getAddrFromJson - JSON does not contain data section";
        return JSON_RESULT_ERR;
    }

    if(cmd[JSON_PARAM_DATA].contains(JSON_PARAM_IPV4_GW))
    {
        cmd[JSON_PARAM_DATA][JSON_PARAM_IPV4_ADDR] = strDefAddr4;
        cmd[JSON_PARAM_DATA][JSON_PARAM_IPV4_MASK] = strDefAddr4;
    }
    else if(cmd[JSON_PARAM_DATA].contains(JSON_PARAM_IPV6_GW))
    {
        cmd[JSON_PARAM_DATA][JSON_PARAM_IPV6_ADDR] = strDefAddr6;
        cmd[JSON_PARAM_DATA][JSON_PARAM_IPV6_MASK] = strDefAddr6;
    }
    else
    {
        LOG_S(ERROR) << "execCmdDefRouteSet - Invalid JSON data";
        return JSON_RESULT_ERR;
    }

    rt_addr = tool::getAddrFromJson(cmd);
    if(rt_addr==nullptr)
    {
        LOG_S(ERROR) << "Cannot decode JSON data";
        return JSON_RESULT_ERR;
    }

    if(!route_worker::setStaticRoute(rt_addr))
    {
        LOG_S(ERROR) << "Cannot set default route";
        return JSON_RESULT_ERR;
    }

    LOG_S(INFO) << "Default route set";
    delete rt_addr;
    return JSON_RESULT_SUCCESS;
}
