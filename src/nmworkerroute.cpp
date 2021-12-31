#include "nmworkerroute.h"

/*
 * Round up 'a' to next multiple of 'size', which must be a power of 2
 * (c) Unix Network Programming by W. Richard Stevens.
*/
#define ROUNDUP(a, size) (((a) & ((size)-1)) ? (1 + ((a) | ((size)-1))) : (a))
/*
 * Step to next socket address structure;
 * if sa_len is 0, assume it is sizeof(u_long)
 * (c) Unix Network Programming by W. Richard Stevens.
*/
#define NEXT_SA(ap) ap = (struct sockaddr *) ((caddr_t) ap + (ap->sa_len ? ROUNDUP(ap->sa_len, sizeof (u_long)) : sizeof(u_long)))

NmWorkerRoute::NmWorkerRoute()
{ }

NmWorkerRoute::~NmWorkerRoute()
{ }

NmScope NmWorkerRoute::getScope()
{
    return NmScope::ROUTE;
}

json NmWorkerRoute::execCmd(NmCommandData* pcmd)
{
    switch (pcmd->getCommand().cmd)
    {
        case NmCmd::RT_GET :
            return execCmdRouteGet(pcmd);
        case NmCmd::RT_DEF_GET :
            return execCmdDefRouteGet(pcmd);
        case NmCmd::RT_SET :
            return execCmdRouteSet(pcmd);
        case NmCmd::RT_DEF_SET :
            return execCmdDefRouteSet(pcmd);
        case NmCmd::RT_LIST :
        case NmCmd::RT_LIST6 :
            return execCmdRouteList(pcmd);
        case NmCmd::RT_DEL :
            return execCmdRouteDel(pcmd);
        case NmCmd::RT_DEF6_GET :
            return execCmdDefRouteGet6(pcmd);
        case NmCmd::RT_DEF_DEL :
            return execCmdDefRouteDel(pcmd);
        default :
            return { { JSON_PARAM_RESULT, JSON_PARAM_ERR }, {JSON_PARAM_ERR, JSON_DATA_ERR_INVALID_COMMAND} };
    }
}

bool NmWorkerRoute::isValidCmd(NmCommandData* pcmd)
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
/*
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
*/
void NmWorkerRoute::setPsaStruct(sockaddr_in *psa, const std::shared_ptr<AddressBase> spstrt)
{
    psa->sin_len = sizeof(struct sockaddr_in);
    psa->sin_family = spstrt->getFamily();
    memcpy(psa, spstrt->getSockAddr(), sizeof(struct sockaddr_in));
}

void NmWorkerRoute::setPsaStruct6(sockaddr_in6 *psa6, const std::shared_ptr<AddressBase> spstrt)
{
    psa6->sin6_len = sizeof(struct sockaddr_in6);
    psa6->sin6_family = spstrt->getFamily();
    memcpy(psa6, spstrt->getSockAddr(), sizeof(struct sockaddr_in6));
}

bool NmWorkerRoute::setStaticRoute(std::shared_ptr<AddressGroup> stroute)
{
    std::unique_ptr<static_route> up_new_route=nullptr;
    std::unique_ptr<static_route6> up_new_route6=nullptr;
    void* pnr=nullptr;
    size_t pnr_size=0;
    struct rt_msghdr *prtm_hdr=nullptr;
    short fam = stroute->getAddrAB()->getFamily();
    time_t curtime = time(NULL);
    struct tm *info = localtime(&curtime);
    int seq = 3600*info->tm_hour + 60*info->tm_min + info->tm_sec;

    if(fam == AF_INET)
    {
        up_new_route = std::make_unique<static_route>();
        memset(up_new_route.get(), 0x00, sizeof(static_route));
        prtm_hdr = &(up_new_route->head);
        prtm_hdr->rtm_msglen = sizeof(static_route);
    }
    else if(fam == AF_INET6)
    {
        up_new_route6 = std::make_unique<static_route6>();
        memset(up_new_route6.get(), 0x00, sizeof(static_route6));
        prtm_hdr = &(up_new_route6->head);
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
        setPsaStruct(&(up_new_route->dest), stroute->getAddr());
        setPsaStruct(&(up_new_route->gateway), stroute->getData());
        setPsaStruct(&(up_new_route->netmask), stroute->getMask());
        pnr=up_new_route.get();
        pnr_size = sizeof(static_route);
    }
    else if(fam == AF_INET6)
    {
        setPsaStruct6(&(up_new_route6->dest), stroute->getAddr());
        setPsaStruct6(&(up_new_route6->gateway), stroute->getData());
        setPsaStruct6(&(up_new_route6->netmask), stroute->getMask());
        pnr=up_new_route6.get();
        pnr_size = sizeof(static_route6);
    }

    sockpp::socket sock = sockpp::socket::create(AF_ROUTE, SOCK_RAW);

    while(true)
    {
        if(write(sock.handle(), pnr, pnr_size) < 0)
        {
               if( (errno == EEXIST) && (prtm_hdr->rtm_type == RTM_ADD) )
               {
                        prtm_hdr->rtm_type = RTM_CHANGE;
                        continue;
               }
               LOG_S(ERROR) << "setStaticRoute cannot write to socket";
               sock.close();
               return false;
        }
        sock.close();
        return true;
    }
}

bool NmWorkerRoute::delStaticRoute(std::shared_ptr<AddressGroup> stroute)
{
    std::unique_ptr<static_route> up_old_route=nullptr;
    std::unique_ptr<static_route6> up_old_route6=nullptr;
    void* pnr=nullptr;
    size_t pnr_size=0;
    struct rt_msghdr *prtm_hdr=nullptr;
    short fam = stroute->getAddrAB()->getFamily();
    time_t curtime = time(NULL);
    struct tm *info = localtime(&curtime);
    int seq = 3600*info->tm_hour + 60*info->tm_min + info->tm_sec;

    if(fam == AF_INET)
    {
        up_old_route = std::make_unique<static_route>();
        memset(up_old_route.get(), 0x00, sizeof(static_route));
        prtm_hdr = &(up_old_route->head);
        prtm_hdr->rtm_msglen = sizeof(static_route);
    }
    else if(fam == AF_INET6)
    {
        up_old_route6 = std::make_unique<static_route6>();
        memset(up_old_route6.get(), 0x00, sizeof(static_route6));
        prtm_hdr = &(up_old_route6->head);
        prtm_hdr->rtm_msglen = sizeof(static_route6);
    }
    else
    {
        LOG_S(ERROR) << "Error in delStaticRoute: incorrect address family received: " << fam;
        return false;
    }

    prtm_hdr->rtm_type = RTM_DELETE;
    prtm_hdr->rtm_version = RTM_VERSION;
    prtm_hdr->rtm_seq = seq;
    prtm_hdr->rtm_addrs = RTA_DST | RTA_GATEWAY | RTA_NETMASK;
    prtm_hdr->rtm_pid = getpid();

    if(fam == AF_INET)
    {
        setPsaStruct(&(up_old_route->dest), stroute->getAddr());
        setPsaStruct(&(up_old_route->gateway), stroute->getData());
        setPsaStruct(&(up_old_route->netmask), stroute->getMask());
        pnr=up_old_route.get();
        pnr_size = sizeof(static_route);
    }
    else if(fam == AF_INET6)
    {
        setPsaStruct6(&(up_old_route6->dest), stroute->getAddr());
        setPsaStruct6(&(up_old_route6->gateway), stroute->getData());
        setPsaStruct6(&(up_old_route6->netmask), stroute->getMask());
        pnr=up_old_route6.get();
        pnr_size = sizeof(static_route6);
    }

    sockpp::socket sock = sockpp::socket::create(AF_ROUTE, SOCK_RAW);

    if(write(sock.handle(), pnr, pnr_size) < 0)
    {
           LOG_S(ERROR) << "delStaticRoute cannot write to socket";
           sock.close();
           return false;
    }
    sock.close();
    return true;
}
/*
bool route_worker::getStaticRoute(std::shared_ptr<addr> stroute)
{
    std::unique_ptr<static_route> up_cur_route=nullptr;
    std::unique_ptr<static_route6> up_cur_route6=nullptr;
    struct rt_msghdr *prtm_hdr=nullptr;
    void* pnr=nullptr;
    size_t pnr_size=0;
    short fam = stroute->getAddrAB()->getFamily();
    time_t curtime = time(NULL);
    struct tm *info = localtime(&curtime);
    int seq = 3600*info->tm_hour + 60*info->tm_min + info->tm_sec;
    pid_t curr_pid = getpid();

    if(fam == AF_INET)
    {
        up_cur_route = std::make_unique<static_route>();
        memset(up_cur_route.get(), 0x00, sizeof(static_route));
        prtm_hdr = &(up_cur_route->head);
        prtm_hdr->rtm_msglen = sizeof(static_route);
    }
    else if(fam == AF_INET6)
    {
        up_cur_route6 = std::make_unique<static_route6>();
        memset(up_cur_route6.get(), 0x00, sizeof(static_route6));
        prtm_hdr = &(up_cur_route6->head);
        prtm_hdr->rtm_msglen = sizeof(static_route6);
    }
    else
        return false;

    prtm_hdr->rtm_type = RTM_GET;
    prtm_hdr->rtm_flags |= RTF_GATEWAY;
    prtm_hdr->rtm_flags |= RTF_UP;
    prtm_hdr->rtm_version = RTM_VERSION;
    prtm_hdr->rtm_seq = seq;
    prtm_hdr->rtm_pid = curr_pid;
    prtm_hdr->rtm_addrs = RTA_DST | RTA_GATEWAY | RTA_NETMASK;
//    prtm_hdr->rtm_addrs = RTA_DST;

    if(fam == AF_INET)
    {
        setPsaStruct(&(up_cur_route->dest), stroute->getAddr());
        setPsaStruct(&(up_cur_route->netmask), stroute->getMask());
        up_cur_route->gateway.sin_len = sizeof(struct sockaddr_in);
        up_cur_route->gateway.sin_family = fam;
        pnr=up_cur_route.get();
        pnr_size = sizeof(static_route);
    }
    else if(fam == AF_INET6)
    {
        setPsaStruct6(&(up_cur_route6->dest), stroute->getAddr());
        setPsaStruct6(&(up_cur_route6->netmask), stroute->getMask());
        up_cur_route6->gateway.sin6_len = sizeof(struct sockaddr_in6);
        up_cur_route6->gateway.sin6_family = fam;
        pnr=up_cur_route6.get();
        pnr_size = sizeof(static_route6);
    }

    sockpp::socket sock = sockpp::socket::create(AF_ROUTE, SOCK_RAW);
    if(write(sock.handle(), pnr, pnr_size) < 0)
    {
        LOG_S(ERROR) << "getStaticRoute cannot write to socket";
        sock.close();
        return false;
    }
    do
    {
        if(read(sock.handle(), pnr, pnr_size) == -1)
        {
            LOG_S(ERROR) << "getStaticRoute cannot read from socket";
            sock.close();
            return false;
        }
    } while( reinterpret_cast<struct rt_msghdr*>(pnr)->rtm_type != RTM_GET ||
             reinterpret_cast<struct rt_msghdr*>(pnr)->rtm_seq != seq ||
             reinterpret_cast<struct rt_msghdr*>(pnr)->rtm_pid != curr_pid );
    sock.close();

    try
    {
        switch(fam)
        {
            case AF_INET:
                stroute->setData(std::make_shared<address_ip4>(reinterpret_cast<const struct sockaddr_in*>(&(reinterpret_cast<static_route*>(pnr)->gateway))));
                break;
            case AF_INET6:
                stroute->setData(std::make_shared<address_ip6>(reinterpret_cast<const struct sockaddr_in6*>(&(reinterpret_cast<static_route6*>(pnr)->gateway))));
                break;
            case AF_LINK:
//              TODO: it does not work - we get just interface number here
                stroute->setData(std::make_shared<address_link>(reinterpret_cast<const struct sockaddr_dl*>(&(reinterpret_cast<static_route_link*>(pnr)->gateway))));
                break;
        }
        return true;
    }
    catch(std::exception& e)
    {
        LOG_S(ERROR) << "getStaticRoute cannot decode router data received from system";
        return false;
    }
}
*/
json NmWorkerRoute::execCmdRouteGet(NmCommandData* pcmd)
{
    std::shared_ptr<AddressGroup> sp_rt_addr=nullptr;
    std::unique_ptr<Interface> proute=nullptr;
    json cmd = {};
    json res_route = {};

    try {
        cmd = pcmd->getJsonData();
    } catch (std::exception& e) {
        LOG_S(ERROR) << "Exception in execCmdRouteGet - cannot get interface parameters";
        return JSON_RESULT_ERR;
    }

    sp_rt_addr = Tool::getAddrFromJson(cmd);
    if(sp_rt_addr==nullptr)
    {
        LOG_S(ERROR) << "Cannot decode JSON data";
        return JSON_RESULT_ERR;
    }

    sp_rt_addr->setType(AddressGroupType::ROUTE);
    proute=getStaticRoute(sp_rt_addr);
    if( proute == nullptr ) {
        LOG_S(ERROR) << "Error in execCmdRouteGet - cannot get route";
        return JSON_RESULT_ERR;
    }

    res_route[JSON_PARAM_RESULT] = JSON_PARAM_SUCC;
    res_route[JSON_PARAM_DATA] = proute->getIfJson();
    return res_route;
}

json NmWorkerRoute::execCmdDefRouteDel(NmCommandData*)
{
    auto spaddr = std::make_shared<AddressIp4>();
    auto spmask = std::make_shared<AddressIp4>();
    auto sp_rt_addr = std::make_shared<AddressGroup>(spaddr, spmask, nullptr, AddressGroupType::ROUTE);

    if( !getStaticRoute(sp_rt_addr) )
    {
        LOG_S(ERROR) << "Error in execCmdDefRouteDel - cannot get default route";
        return JSON_RESULT_ERR;
    }

    if( !delStaticRoute(sp_rt_addr) )
    {
        LOG_S(ERROR) << "Error in execCmdDefRouteDel - cannot delete default route";
        return JSON_RESULT_ERR;
    }

    LOG_S(INFO) << "Default route deleted";
    return JSON_RESULT_SUCCESS;
}

json NmWorkerRoute::execCmdDefRouteGet(NmCommandData*)
{
    json res_route = {};
    short family;
    std::string strGw = "";
    auto spaddr = std::make_shared<AddressIp4>();
    auto spmask = std::make_shared<AddressIp4>();
//    auto spgate = std::make_shared<address_ip4>();
//    auto sp_rt_addr = std::make_shared<addr>(spaddr, spmask, spgate, ipaddr_type::ROUTE);
    auto sp_rt_addr = std::make_shared<AddressGroup>(spaddr, spmask, nullptr, AddressGroupType::ROUTE);
    if( !getStaticRoute(sp_rt_addr) )
    {
        LOG_S(ERROR) << "Error in execCmdDefRouteGet - cannot get default route";
        return JSON_RESULT_ERR;
    }

    family = sp_rt_addr->getData()->getFamily();
    strGw = sp_rt_addr->getData()->getStrAddr();
    res_route[JSON_PARAM_RESULT] = JSON_PARAM_SUCC;
    res_route[JSON_PARAM_DATA] = { { JSON_PARAM_IPV4_GW, strGw } };
    return res_route;
}

json NmWorkerRoute::execCmdDefRouteGet6(NmCommandData*)
{
    json res_route = {};
    short family;
    std::string strGw = "";
    auto spaddr = std::make_shared<AddressIp6>();
    auto spmask = std::make_shared<AddressIp6>();
//    auto spgate = std::make_shared<address_ip6>();
//    auto sp_rt_addr = std::make_shared<addr>(spaddr, spmask, spgate, ipaddr_type::ROUTE);
    auto sp_rt_addr = std::make_shared<AddressGroup>(spaddr, spmask, nullptr, AddressGroupType::ROUTE);
    if( !getStaticRoute(sp_rt_addr) )
    {
        LOG_S(ERROR) << "Error in execCmdDefRouteGet6 - cannot get default route";
        return JSON_RESULT_ERR;
    }

    family = sp_rt_addr->getData()->getFamily();
    strGw = sp_rt_addr->getData()->getStrAddr();
    res_route[JSON_PARAM_RESULT] = JSON_PARAM_SUCC;
    res_route[JSON_PARAM_DATA] = { { JSON_PARAM_IPV6_GW, strGw } };
    return res_route;
}

json NmWorkerRoute::execCmdRouteSet(NmCommandData* pcmd)
{
    json cmd = {};

    auto rt_addr = Tool::getAddrFromJson(pcmd->getJsonData());
    if(rt_addr==nullptr)
    {
        LOG_S(ERROR) << "Cannot decode JSON data";
        return JSON_RESULT_ERR;
    }

    if(!NmWorkerRoute::setStaticRoute(rt_addr))
    {
        LOG_S(ERROR) << "Cannot set route";
        return JSON_RESULT_ERR;
    }

    LOG_S(INFO) << "Static route set";
    return JSON_RESULT_SUCCESS;
}

json NmWorkerRoute::execCmdRouteDel(NmCommandData* pcmd)
{
    json cmd = {};

    auto rt_addr = Tool::getAddrFromJson(pcmd->getJsonData());
    if(rt_addr==nullptr)
    {
        LOG_S(ERROR) << "Cannot decode JSON data";
        return JSON_RESULT_ERR;
    }

    if(!NmWorkerRoute::delStaticRoute(rt_addr))
    {
        LOG_S(ERROR) << "Cannot delete route";
        return JSON_RESULT_ERR;
    }

    LOG_S(INFO) << "Static route deleted";
    return JSON_RESULT_SUCCESS;
}

json NmWorkerRoute::execCmdDefRouteSet(NmCommandData* pcmd)
{
    json cmd = {};
    std::string strDefAddr4 = "0.0.0.0";
    std::string strDefAddr6 = "0:0:0:0:0:0:0:0";

    cmd = pcmd->getJsonData();

    if(!cmd.contains(JSON_PARAM_DATA))
    {
        LOG_S(ERROR) << "execCmdDefRouteSet - JSON does not contain data section";
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

    auto rt_addr = Tool::getAddrFromJson(cmd);
    if(rt_addr==nullptr)
    {
        LOG_S(ERROR) << "Cannot decode JSON data";
        return JSON_RESULT_ERR;
    }

    if(!NmWorkerRoute::setStaticRoute(rt_addr))
    {
        LOG_S(ERROR) << "Cannot set default route";
        return JSON_RESULT_ERR;
    }

    LOG_S(INFO) << "Default route set";
    return JSON_RESULT_SUCCESS;
}

json NmWorkerRoute::execCmdRouteList(NmCommandData* pcmd)
{
    struct rt_msghdr* rtm = nullptr;
    struct sockaddr *sa = nullptr;
    struct sockaddr *rti_info[RTAX_MAX];
    size_t sz = 0;
    short err_cnt = 0;
    short constexpr ERR_CNT_MAX = 10;
    std::vector<uint8_t> buf(0);
    std::vector<Interface*> ifaces(0);
    std::shared_ptr<AddressBase> sp_dest = nullptr;
    std::shared_ptr<AddressBase> sp_mask = nullptr;
    std::shared_ptr<AddressBase> sp_gate = nullptr;
    short constexpr MIB_SIZE = 6;
    int family = 0;
    int mib[MIB_SIZE];
    char ifname[IFNAMSIZ];
    std::string strIfName = "";
    bool isFound = false;
    std::vector<nlohmann::json> vectIfsJson;
    nlohmann::json retIfListJson = {};
    nlohmann::json res_routes = {};

    if(pcmd->getCommand().cmd == NmCmd::RT_LIST)
        family = AF_INET;
    else if(pcmd->getCommand().cmd == NmCmd::RT_LIST6)
        family = AF_INET6;
    else
        return JSON_RESULT_ERR;

    mib[0] = CTL_NET;
    mib[1] = AF_ROUTE;
    mib[2] = 0;
    mib[3] = family;
    mib[4] = NET_RT_DUMP;
    mib[5] = 0;

    //  Get necessary buffer size and resize the buffer
    if ( sysctl(mib, MIB_SIZE, nullptr, &sz, nullptr, 0) != 0 )
    {
        LOG_S(ERROR) << "Cannot get sysctl table size";
        return JSON_RESULT_ERR;
    }
    buf.resize(sz);

    //  Try to get routing table using sysctl calls until we successfully get it
    //  or until we get ERR_CNT_MAX errors
    while(true)
    {
        if ( sysctl(mib, MIB_SIZE, &buf[0], &sz, nullptr, 0) == 0)
        {
            if (buf.size() < sz)
            {
                buf.resize(sz);
                continue;
            }
            else if (buf.size() > sz)
            {
                buf.resize(sz);
            }
            rtm = reinterpret_cast<struct rt_msghdr*>(&(buf[0]));
            for( size_t offset=0; offset<buf.size(); offset+=rtm->rtm_msglen )
            {
                if(offset>0)
                {
                    rtm = reinterpret_cast<struct rt_msghdr*>(&(buf[offset]));
                }
                if (rtm->rtm_version != RTM_VERSION)
                {
                    LOG_S(ERROR) << "RTM version mismatch: expected " << RTM_VERSION << " got " << rtm->rtm_version;
                    err_cnt++;
                    if(err_cnt > ERR_CNT_MAX)
                        return JSON_RESULT_ERR;
                    continue;
                }
                if (rtm->rtm_errno != 0)
                {
                    LOG_S(ERROR) << "Got error in RTM: " << rtm->rtm_errno;
                    err_cnt++;
                    if(err_cnt > ERR_CNT_MAX)
                        return JSON_RESULT_ERR;
                    continue;
                }
//              We need only active routes with gateway defined
                if( !(rtm->rtm_flags & RTF_GATEWAY) || !(rtm->rtm_flags & RTF_UP) )
                    continue;

//              Fill rti_info array with available information
                sa = reinterpret_cast<struct sockaddr*>(rtm + 1);
                for (short i=0; i<RTAX_MAX; i++)
                {
                    if (rtm->rtm_addrs & (1 << i))
                    {
                        rti_info[i] = sa;
                        NEXT_SA(sa);
                    }
                    else
                    {
                        rti_info[i] = nullptr;
                    }
                }

//              Analyse and output the information from rti_info array
                if ( (sa=rti_info[RTAX_DST]) != nullptr )
                {
                    if (sa->sa_family == AF_INET)
                    {
                        sp_dest = std::make_shared<AddressIp4>(reinterpret_cast<const struct sockaddr_in*>(sa));
                    }
                    else if (sa->sa_family == AF_INET6)
                    {
                        sp_dest = std::make_shared<AddressIp6>(reinterpret_cast<const struct sockaddr_in6*>(sa));
                    }
                }
                else
                {
                    sp_dest = nullptr;
                }

                if ( (sa = rti_info[RTAX_GATEWAY]) != nullptr)
                {
                    if (sa->sa_family == AF_INET)
                    {
                        sp_gate = std::make_shared<AddressIp4>(reinterpret_cast<const struct sockaddr_in*>(sa));
                    }
                    else if (sa->sa_family == AF_INET6)
                    {
                        sp_gate = std::make_shared<AddressIp6>(reinterpret_cast<const struct sockaddr_in6*>(sa));
                    }
                }
                else
                {
                    sp_gate = nullptr;
                }

                if ( (sa = rti_info[RTAX_NETMASK]) != nullptr)
                {
                    if (sa->sa_family == AF_INET)
                    {
                        sp_mask = std::make_shared<AddressIp4>(reinterpret_cast<const struct sockaddr_in*>(sa));
                    }
                    else if (sa->sa_family == AF_INET6)
                    {
                        sp_mask = std::make_shared<AddressIp6>(reinterpret_cast<const struct sockaddr_in6*>(sa));
                    }
                }
                else
                {
                    sp_mask = nullptr;
                }

                if(!(rtm->rtm_flags & RTF_LLINFO) && (rtm->rtm_flags & RTF_HOST))
                {
                    if (sa->sa_family == AF_INET)
                        sp_mask = std::make_shared<AddressIp4>("255.255.255.255");
                    else if (sa->sa_family == AF_INET6)
                        sp_mask = std::make_shared<AddressIp6>("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
                }

                if(sp_dest==nullptr)
                {
                    if (sa->sa_family == AF_INET)
                        sp_mask = std::make_shared<AddressIp4>("0.0.0.0");
                    else if (sa->sa_family == AF_INET6)
                        sp_mask = std::make_shared<AddressIp6>("::");
                }

                if(if_indextoname(rtm->rtm_index, ifname) != nullptr)
                {
                    strIfName = std::string(ifname);
                }
                else
                {
                    strIfName = std::to_string(rtm->rtm_index);
                }

                isFound = false;
                for(auto iface : ifaces)
                {
                    if(iface->getName()==strIfName)
                    {
                        isFound = true;
                        iface->addAddress(std::make_shared<AddressGroup>(sp_dest, sp_mask, sp_gate, AddressGroupType::ROUTE, true, rtm->rtm_flags));
                    }
                }
                if(!isFound)
                {
                    ifaces.push_back(new Interface(strIfName));
                    ifaces[ifaces.size()-1]->addAddress(std::make_shared<AddressGroup>(sp_dest, sp_mask, sp_gate, AddressGroupType::ROUTE, true, rtm->rtm_flags));
                }
            }
        }

        if (errno == ENOMEM)
        {
            if ( sysctl(mib, MIB_SIZE, nullptr, &sz, nullptr, 0) != 0 )
            {
                LOG_S(ERROR) << "Cannot get sysctl table size to resize buffer";
                return JSON_RESULT_ERR;
            }
            buf.resize(sz);
            continue;
        }

        if (errno!=0)
        {
            LOG_S(ERROR) << "sysctl call failed: " << strerror(errno);
            return JSON_RESULT_ERR;
        }

        break;
    }

    if(ifaces.size()>0)
    {
        for (auto iface : ifaces)
        {
          vectIfsJson.push_back(iface->getIfJson());
          delete iface;
        }

        retIfListJson[JSON_PARAM_ROUTES] = vectIfsJson;
        res_routes[JSON_PARAM_RESULT] = JSON_PARAM_SUCC;
        res_routes[JSON_PARAM_DATA] = retIfListJson;
        return res_routes;
    }
    else
    {
        return JSON_RESULT_NOTFOUND;
    }
}

std::unique_ptr<Interface> NmWorkerRoute::getStaticRoute(std::shared_ptr<AddressGroup> sp_route)
{
    std::unique_ptr<static_route> up_cur_route=nullptr;
    std::unique_ptr<static_route6> up_cur_route6=nullptr;
    struct rt_msghdr *rtm=nullptr;
    short fam = sp_route->getAddrAB()->getFamily();
    time_t curtime = time(NULL);
    struct tm *info = localtime(&curtime);
    int seq = 3600*info->tm_hour + 60*info->tm_min + info->tm_sec;
    pid_t curr_pid = getpid();
    std::shared_ptr<AddressBase> sp_dest = nullptr;
    std::shared_ptr<AddressBase> sp_mask = nullptr;
    std::shared_ptr<AddressBase> sp_gate = nullptr;
    char ifname[IFNAMSIZ];
    std::string strIfName = "";
    struct sockaddr *sa = nullptr;
    struct sockaddr *rti_info[RTAX_MAX];
    struct sockaddr_in *sin = nullptr;
    struct sockaddr_in6 *sin6 = nullptr;
    constexpr short RTBUFLEN = 512;
    auto buf = std::make_unique<unsigned char[]>(RTBUFLEN);
    memset(buf.get(),0,sizeof(buf));
    rtm = reinterpret_cast<struct rt_msghdr*>(buf.get());
    if(fam == AF_INET)
        rtm->rtm_msglen = sizeof(struct rt_msghdr) + sizeof(struct sockaddr_in);
    else if(fam == AF_INET6)
        rtm->rtm_msglen = sizeof(struct rt_msghdr) + sizeof(struct sockaddr_in6);
    rtm->rtm_type = RTM_GET;
    rtm->rtm_flags |= RTF_GATEWAY;
    rtm->rtm_flags |= RTF_UP;
//********************
//    prtm_hdr->rtm_flags |= RTF_HOST;
//********************
    rtm->rtm_version = RTM_VERSION;
    rtm->rtm_seq = seq;
    rtm->rtm_pid = curr_pid;
//********************
//    prtm_hdr->rtm_addrs = RTA_DST | RTA_GATEWAY | RTA_NETMASK;
//********************
    rtm->rtm_addrs = RTA_DST;

    if(fam == AF_INET)
    {
        sin = (struct sockaddr_in *) (rtm + 1);
        sin->sin_len = sizeof(struct sockaddr_in);
        sin->sin_family = AF_INET;
        memcpy(sin, sp_route->getAddrAB()->getSockAddr(), sizeof(struct sockaddr_in));
    }
    else if(fam == AF_INET6)
    {
        sin6 = (struct sockaddr_in6 *) (rtm + 1);
        sin6->sin6_len = sizeof(struct sockaddr_in6);
        sin6->sin6_family = AF_INET6;
        memcpy(sin6, sp_route->getAddrAB()->getSockAddr(), sizeof(struct sockaddr_in6));
    }

    sockpp::socket sock = sockpp::socket::create(AF_ROUTE, SOCK_RAW);
    if(write(sock.handle(), rtm, rtm->rtm_msglen) < 0)
    {
        LOG_S(ERROR) << "getStaticRoute cannot write to socket";
        sock.close();
        return nullptr;
    }

    do {
        if(read(sock.handle(), rtm, RTBUFLEN) == -1)
        {
            LOG_S(ERROR) << "getStaticRoute cannot read from socket";
            sock.close();
            return nullptr;
        }
    } while (rtm->rtm_type != RTM_GET || rtm->rtm_seq != seq || rtm->rtm_pid != curr_pid);

    sock.close();

    if (rtm->rtm_version != RTM_VERSION)
    {
        LOG_S(ERROR) << "RTM version mismatch: expected " << RTM_VERSION << " got " << rtm->rtm_version;
        return nullptr;
    }
    if (rtm->rtm_errno != 0)
    {
        LOG_S(ERROR) << "Got error in RTM: " << rtm->rtm_errno;
        return nullptr;
    }

//      Fill rti_info array with available information
        sa = reinterpret_cast<struct sockaddr*>(rtm + 1);
        for (short i=0; i<RTAX_MAX; i++)
        {
            if (rtm->rtm_addrs & (1 << i))
            {
                rti_info[i] = sa;
                NEXT_SA(sa);
            }
            else
            {
                rti_info[i] = nullptr;
            }
        }

//      Analyse and output the information from rti_info array
        if ( (sa=rti_info[RTAX_DST]) != nullptr )
        {
            if (sa->sa_family == AF_INET)
            {
                sp_dest = std::make_shared<AddressIp4>(reinterpret_cast<const struct sockaddr_in*>(sa));
            }
            else if (sa->sa_family == AF_INET6)
            {
                sp_dest = std::make_shared<AddressIp6>(reinterpret_cast<const struct sockaddr_in6*>(sa));
            }
            else if (sa->sa_family == AF_LINK)
            {
                sp_dest = std::make_shared<AddressLink>(reinterpret_cast<const struct sockaddr_dl*>(sa));
            }
        }
        else
        {
            sp_dest = nullptr;
        }

        if ( (sa = rti_info[RTAX_GATEWAY]) != nullptr)
        {
            if (sa->sa_family == AF_INET)
            {
                sp_gate = std::make_shared<AddressIp4>(reinterpret_cast<const struct sockaddr_in*>(sa));
            }
            else if (sa->sa_family == AF_INET6)
            {
                sp_gate = std::make_shared<AddressIp6>(reinterpret_cast<const struct sockaddr_in6*>(sa));
            }
            else if (sa->sa_family == AF_LINK)
            {
                sp_gate = std::make_shared<AddressLink>(reinterpret_cast<const struct sockaddr_dl*>(sa));
            }
        }
        else
        {
            sp_gate = nullptr;
        }

        if ( (sa = rti_info[RTAX_NETMASK]) != nullptr)
        {
            if (sa->sa_family == AF_INET)
            {
                sp_mask = std::make_shared<AddressIp4>(reinterpret_cast<const struct sockaddr_in*>(sa));
            }
            else if (sa->sa_family == AF_INET6)
            {
                sp_mask = std::make_shared<AddressIp6>(reinterpret_cast<const struct sockaddr_in6*>(sa));
            }
        }
        else
        {
            sp_mask = nullptr;
        }

        if(sp_gate == nullptr)
        {
            LOG_S(ERROR) << "No gateway received from socket";
            return nullptr;
        }

        if((sa != nullptr) && !(rtm->rtm_flags & RTF_LLINFO) && (rtm->rtm_flags & RTF_HOST))
        {
            if (sa->sa_family == AF_INET)
                sp_mask = std::make_shared<AddressIp4>("255.255.255.255");
            else if (sa->sa_family == AF_INET6)
                sp_mask = std::make_shared<AddressIp6>("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
        }

        if(sp_dest==nullptr)
        {
            if (sa->sa_family == AF_INET)
                sp_mask = std::make_shared<AddressIp4>("0.0.0.0");
            else if (sa->sa_family == AF_INET6)
                sp_mask = std::make_shared<AddressIp6>("::");
        }

        if(if_indextoname(rtm->rtm_index, ifname) != nullptr)
        {
            strIfName = std::string(ifname);
        }
        else
        {
            strIfName = std::to_string(rtm->rtm_index);
        }

    sp_route->setAddr(sp_dest);
    sp_route->setMask(sp_mask);
    sp_route->setData(sp_gate);
    sp_route->setFlags(rtm->rtm_flags);

    auto up_ret = std::make_unique<Interface>(strIfName);
    up_ret->addAddress(sp_route);

    return up_ret;
}
