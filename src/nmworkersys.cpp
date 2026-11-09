#include "nmworkersys.h"
#include "nmworkerroute.h"
#include <net/if_lagg.h>

NmWorkerSys::NmWorkerSys()
{
    prcConf = nullptr;
}

NmWorkerSys::~NmWorkerSys()
{
}

NmScope NmWorkerSys::getScope()
{
    return NmScope::SYSTEM;
}

json NmWorkerSys::execCmd(NmCommandData* pcmd)
{
    switch (pcmd->getCommand().cmd)
    {
        case NmCmd::IF_LIST :
            return execCmdIfList(pcmd);
        case NmCmd::IF_ADD :
            return execCmdIfAdd(pcmd);
        case NmCmd::IF_REMOVE :
            return execCmdIfRemove(pcmd);
        case NmCmd::IF_ENABLE :
            return execCmdIfEnable(pcmd);
        case NmCmd::IF_DISABLE :
            return execCmdIfDisable(pcmd);
        case NmCmd::RCCONF_READ :
            return execCmdRcConfRead(pcmd);
        case NmCmd::RCCONF_WRITE :
            return execCmdRcConfWrite(pcmd);
        case NmCmd::JAIL_LIST :
            return execCmdJailList(pcmd);
        case NmCmd::RESOLVCONF_READ :
            return execCmdResolvConfRead(pcmd);
        case NmCmd::RESOLVCONF_WRITE :
            return execCmdResolvConfWrite(pcmd);
        default :
            return { { JSON_PARAM_RESULT, JSON_PARAM_ERR }, {JSON_PARAM_ERR, JSON_DATA_ERR_INVALID_COMMAND} };
    }
}

bool NmWorkerSys::isValidCmd(NmCommandData* pcmd)
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

// Checks every filter criterion that's actually present in `filter` against
// one interface's JSON (as produced by Tool::getIfInfoJson()), ANDing them
// together. Currently only "INTERFACE TYPE" is implemented; a future
// criterion is added the same way: look it up in `filter`, and if present,
// AND its own match result into `matches`.
bool NmWorkerSys::ifMatchesFilterCriteria(const json& ifJson, const json& filter)
{
    bool matches = true;

    if(filter.contains(JSON_PARAM_IF_TYPE))
    {
        std::string ifname = ifJson.value(JSON_PARAM_IF_NAME, std::string());
        std::string iftype = Tool::classifyIfType(ifname);
        bool typeMatches = false;
        for(const auto& allowed : filter[JSON_PARAM_IF_TYPE])
        {
            if(allowed.is_string() && allowed.get<std::string>() == iftype)
            {
                typeMatches = true;
                break;
            }
        }
        matches = matches && typeMatches;
    }

    return matches;
}

bool NmWorkerSys::applyIfListFilter(std::vector<json>& ifList, const json& filter)
{
    std::string mode;

    if(!filter.contains(JSON_PARAM_MODE) || !filter[JSON_PARAM_MODE].is_string())
    {
        LOG_S(ERROR) << "execCmdIfList: filter " << JSON_PARAM_MODE << " is missing or not a string";
        return false;
    }
    mode = filter[JSON_PARAM_MODE].get<std::string>();
    if(mode != JSON_PARAM_INCLUDE && mode != JSON_PARAM_EXCLUDE)
    {
        LOG_S(ERROR) << "execCmdIfList: filter " << JSON_PARAM_MODE << " must be " << JSON_PARAM_INCLUDE << " or " << JSON_PARAM_EXCLUDE;
        return false;
    }

    if(filter.contains(JSON_PARAM_IF_TYPE) && !filter[JSON_PARAM_IF_TYPE].is_array())
    {
        LOG_S(ERROR) << "execCmdIfList: filter " << JSON_PARAM_IF_TYPE << " must be an array";
        return false;
    }

    std::vector<json> filtered;
    for(const auto& ifJson : ifList)
    {
        bool matches = ifMatchesFilterCriteria(ifJson, filter);
        bool keep = (mode == JSON_PARAM_INCLUDE) ? matches : !matches;
        if(keep)
            filtered.push_back(ifJson);
    }

    ifList = filtered;
    return true;
}

json NmWorkerSys::execCmdIfList(NmCommandData* pcmd)
{
    nlohmann::json retIfListJson;
    nlohmann::json res_ifaces = {};
    std::vector<json> ifList = Tool::getIfInfoJson();

    json cmd = pcmd->getJsonData();
    if(cmd.contains(JSON_PARAM_DATA) && cmd[JSON_PARAM_DATA].contains(JSON_PARAM_FILTER))
    {
        if(!applyIfListFilter(ifList, cmd[JSON_PARAM_DATA][JSON_PARAM_FILTER]))
            return JSON_RESULT_ERR;
    }

    retIfListJson[JSON_PARAM_INTERFACES] = ifList;
    res_ifaces[JSON_PARAM_RESULT] = JSON_PARAM_SUCC;
    res_ifaces[JSON_PARAM_DATA] = retIfListJson;

    return res_ifaces;
}

/*********  ifconfig.c  *********

static int
getifflags(const char *ifname, int us)
{
    struct ifreq my_ifr;
    int s;

    memset(&my_ifr, 0, sizeof(my_ifr));
    (void) strlcpy(my_ifr.ifr_name, ifname, sizeof(my_ifr.ifr_name));
    if (us < 0) {
        if ((s = socket(AF_LOCAL, SOCK_DGRAM, 0)) < 0)
            err(1, "socket(family AF_LOCAL,SOCK_DGRAM");
    } else
        s = us;
    if (ioctl(s, SIOCGIFFLAGS, (caddr_t)&my_ifr) < 0) {
        Perror("ioctl (SIOCGIFFLAGS)");
        exit(1);
    }
    if (us < 0)
        close(s);
    return ((my_ifr.ifr_flags & 0xffff) | (my_ifr.ifr_flagshigh << 16));
}

static void
setifflags(const char *vname, int value, int s, const struct afswtch *afp)
{
    struct ifreq		my_ifr;
    int flags;

    flags = getifflags(name, s);
    if (value < 0) {
        value = -value;
        flags &= ~value;
    } else
        flags |= value;
    memset(&my_ifr, 0, sizeof(my_ifr));
    (void) strlcpy(my_ifr.ifr_name, name, sizeof(my_ifr.ifr_name));
    my_ifr.ifr_flags = flags & 0xffff;
    my_ifr.ifr_flagshigh = flags >> 16;
    if (ioctl(s, SIOCSIFFLAGS, (caddr_t)&my_ifr) < 0)
        Perror(vname);
}


int system_worker::getIfFlags(std::string ifname)
{
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(struct ifreq));
    strlcpy(ifr.ifr_name, ifname.c_str(), sizeof(ifr.ifr_name));
    sockpp::socket sock = sockpp::socket::create(AF_LOCAL, SOCK_DGRAM);
    if (ioctl(sock.handle(), SIOCGIFFLAGS, (caddr_t)&ifr) < 0)
    {
        LOG_S(ERROR) << "Cannot get interface " << ifname << " flags";
        sock.close();
        return 0;
    }
    sock.close();
    return ((ifr.ifr_flags & 0xffff) | (ifr.ifr_flagshigh << 16));
}

bool system_worker::setIfFlags(std::string ifname, int setflags)
{
    sockpp::socket sock = sockpp::socket::create(AF_LOCAL, SOCK_DGRAM);
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(struct ifreq));
    strlcpy(ifr.ifr_name, ifname.c_str(), sizeof(ifr.ifr_name));
    ifr.ifr_flags = setflags & 0xffff;
    ifr.ifr_flagshigh = setflags >> 16;
    if (ioctl(sock.handle(), SIOCSIFFLAGS, (caddr_t)&ifr) < 0)
    {
        LOG_S(ERROR) << "Cannot set interface " << ifname << " flags";
        sock.close();
        return false;
    }
    sock.close();
    return true;
}
*/
json NmWorkerSys::execCmdIfEnable(NmCommandData* pcmd) {
    struct ifreq ifr;
    std::string ifname = "";
    const int cmdflag=IFF_UP;
    memset(&ifr, 0, sizeof(struct ifreq));
    json cmd_json = {};
    try {
        cmd_json = pcmd->getJsonData();
        ifname = cmd_json[JSON_PARAM_DATA][JSON_PARAM_IF_NAME];
    } catch (std::exception& e) {
        LOG_S(ERROR) << "Exception in execCmdIfEnable - cannot get ifname";
        return JSON_RESULT_ERR;
    }
    int curflags = Tool::getIfFlags(ifname);
    if(curflags==0)
    {
        return JSON_RESULT_ERR;
    }
    curflags |= cmdflag;
    if(!Tool::setIfFlags(ifname, curflags))
    {
        return JSON_RESULT_ERR;
    }
    return JSON_RESULT_SUCCESS;
}

json NmWorkerSys::execCmdIfDisable(NmCommandData* pcmd) {
    struct ifreq ifr;
    std::string ifname = "";
    std::string default_route_ifname = "";
    const int cmdflag=IFF_UP;
    memset(&ifr, 0, sizeof(struct ifreq));
    json cmd_json = {};
    try {
        cmd_json = pcmd->getJsonData();
        ifname = cmd_json[JSON_PARAM_DATA][JSON_PARAM_IF_NAME];
    } catch (std::exception& e) {
        LOG_S(ERROR) << "Exception in execCmdIfDisable - cannot get ifname";
        return JSON_RESULT_ERR;
    }
    int curflags = Tool::getIfFlags(ifname);
    if(curflags==0)
    {
        return JSON_RESULT_ERR;
    }
    curflags &= ~cmdflag;
    if(!Tool::setIfFlags(ifname, curflags))
    {
        return JSON_RESULT_ERR;
    }
    //json NmWorkerRoute::execCmdDefRouteGet(NmCommandData*)
    auto route_worker = std::make_unique<NmWorkerRoute>();
    auto default_route = route_worker->execCmdDefRouteGet(nullptr);
    if(default_route != JSON_RESULT_ERR) {
        try {
            default_route_ifname = cmd_json[JSON_PARAM_DATA][JSON_PARAM_IF_NAME];
        } catch (std::exception& e) {
            LOG_S(WARNING) << "Cannot get interface name for default route";
        }
        if(default_route_ifname == ifname) {
            auto result = route_worker->execCmdDefRouteDel(nullptr);
            if(result == JSON_RESULT_ERR) {
                LOG_S(ERROR) << "Cannot delete default route for interface " << default_route_ifname;
                return JSON_RESULT_ERR;
            }
        }
    }
    return JSON_RESULT_SUCCESS;
}

json NmWorkerSys::execCmdIfAdd(NmCommandData* pcmd)
{
    static const std::map<std::string, std::string> TYPE_PREFIXES =
    {
        { JSON_PARAM_IF_TYPE_TUN, "tun" },
        { JSON_PARAM_IF_TYPE_TAP, "tap" },
        { JSON_PARAM_IF_TYPE_BRIDGE, Tool::BRIDGE_IF_PREFIX },
        { JSON_PARAM_IF_TYPE_EPAIR, "epair" },
        { JSON_PARAM_IF_TYPE_LAGG, Tool::LAGG_IF_PREFIX }
    };
    static const std::map<std::string, lagg_type> LAGG_TYPE_MAP =
    {
        { JSON_PARAM_LAGG_TYPE_ETHERNET, LAGG_TYPE_ETHERNET },
        { JSON_PARAM_LAGG_TYPE_INFINIBAND, LAGG_TYPE_INFINIBAND }
    };
    struct ifreq ifr;
    struct iflaggparam iflp;
    std::string ifname;
    std::string iftype;
    std::vector<std::string> members;
    bool have_laggproto = false;
    int laggproto = (int)LAGG_PROTO_FAILOVER;
    bool have_rrlimit = false;
    int rrlimit = 0;
    json cmd_json = {};

    memset(&ifr, 0, sizeof(struct ifreq));
    memset(&iflp, 0, sizeof(iflp));
    try {
        cmd_json = pcmd->getJsonData();
        ifname = cmd_json[JSON_PARAM_DATA][JSON_PARAM_IF_NAME];
        iftype = cmd_json[JSON_PARAM_DATA][JSON_PARAM_IF_TYPE];
    } catch (std::exception& e) {
        LOG_S(ERROR) << "Exception in execCmdIfAdd - cannot get interface name/type from JSON";
        return JSON_RESULT_ERR;
    }

    if(!Tool::isValidIfName(ifname))
    {
        LOG_S(ERROR) << "execCmdIfAdd: invalid interface name " << ifname;
        return JSON_RESULT_ERR;
    }

    auto it_prefix = TYPE_PREFIXES.find(iftype);
    if(it_prefix == TYPE_PREFIXES.end())
    {
        LOG_S(ERROR) << "execCmdIfAdd: unsupported interface type " << iftype;
        return { { JSON_PARAM_RESULT, JSON_PARAM_ERR }, {JSON_PARAM_ERR, JSON_DATA_ERR_NOT_IMPLEMENTED} };
    }
    if(!ifname.starts_with(it_prefix->second))
    {
        LOG_S(ERROR) << "execCmdIfAdd: interface name " << ifname << " does not match type " << iftype;
        return JSON_RESULT_ERR;
    }

    if(if_nametoindex(ifname.c_str()) != 0)
    {
        LOG_S(ERROR) << "execCmdIfAdd: interface " << ifname << " already exists";
        return JSON_RESULT_EXISTS;
    }

    if((iftype == JSON_PARAM_IF_TYPE_BRIDGE || iftype == JSON_PARAM_IF_TYPE_LAGG) && cmd_json[JSON_PARAM_DATA].contains(JSON_PARAM_MEMBERS))
    {
        try {
            members = cmd_json[JSON_PARAM_DATA][JSON_PARAM_MEMBERS].get<std::vector<std::string>>();
        } catch (std::exception& e) {
            LOG_S(ERROR) << "execCmdIfAdd: invalid " << JSON_PARAM_MEMBERS << " list for " << ifname;
            return JSON_RESULT_ERR;
        }
        for(const auto& member : members)
        {
            if(!Tool::isValidIfName(member))
            {
                LOG_S(ERROR) << "execCmdIfAdd: invalid member interface name " << member;
                return JSON_RESULT_ERR;
            }
            if(if_nametoindex(member.c_str()) == 0)
            {
                LOG_S(ERROR) << "execCmdIfAdd: member interface " << member << " does not exist";
                return JSON_RESULT_NOTFOUND;
            }
        }
    }

    if(iftype == JSON_PARAM_IF_TYPE_LAGG)
    {
        if(cmd_json[JSON_PARAM_DATA].contains(JSON_PARAM_LAGG_TYPE))
        {
            std::string lagg_type_str;
            try {
                lagg_type_str = cmd_json[JSON_PARAM_DATA][JSON_PARAM_LAGG_TYPE];
            } catch (std::exception& e) {
                LOG_S(ERROR) << "execCmdIfAdd: invalid " << JSON_PARAM_LAGG_TYPE << " value";
                return JSON_RESULT_ERR;
            }
            auto it_type = LAGG_TYPE_MAP.find(lagg_type_str);
            if(it_type == LAGG_TYPE_MAP.end())
            {
                LOG_S(ERROR) << "execCmdIfAdd: unsupported " << JSON_PARAM_LAGG_TYPE << " value " << lagg_type_str;
                return JSON_RESULT_ERR;
            }
            iflp.lagg_type = (uint8_t)it_type->second;
            ifr.ifr_data = (caddr_t)&iflp;
        }

        if(cmd_json[JSON_PARAM_DATA].contains(JSON_PARAM_LAGG_PROTO))
        {
            std::string lagg_proto_str;
            try {
                lagg_proto_str = cmd_json[JSON_PARAM_DATA][JSON_PARAM_LAGG_PROTO];
            } catch (std::exception& e) {
                LOG_S(ERROR) << "execCmdIfAdd: invalid " << JSON_PARAM_LAGG_PROTO << " value";
                return JSON_RESULT_ERR;
            }
            if(!Tool::laggProtoFromName(lagg_proto_str, laggproto))
            {
                LOG_S(ERROR) << "execCmdIfAdd: unsupported " << JSON_PARAM_LAGG_PROTO << " value " << lagg_proto_str;
                return JSON_RESULT_ERR;
            }
            have_laggproto = true;
        }

        if(cmd_json[JSON_PARAM_DATA].contains(JSON_PARAM_LAGG_ROUNDROBIN_LIMIT))
        {
            if(!have_laggproto || laggproto != (int)LAGG_PROTO_ROUNDROBIN)
            {
                LOG_S(ERROR) << "execCmdIfAdd: " << JSON_PARAM_LAGG_ROUNDROBIN_LIMIT << " is only valid with "
                             << JSON_PARAM_LAGG_PROTO << "=" << JSON_PARAM_LAGG_PROTO_ROUNDROBIN;
                return JSON_RESULT_ERR;
            }
            try {
                rrlimit = cmd_json[JSON_PARAM_DATA][JSON_PARAM_LAGG_ROUNDROBIN_LIMIT];
            } catch (std::exception& e) {
                LOG_S(ERROR) << "execCmdIfAdd: invalid " << JSON_PARAM_LAGG_ROUNDROBIN_LIMIT << " value";
                return JSON_RESULT_ERR;
            }
            have_rrlimit = true;
        }
    }

    strlcpy(ifr.ifr_name, ifname.c_str(), sizeof(ifr.ifr_name));
    sockpp::socket sock = sockpp::socket::create(AF_LOCAL, SOCK_DGRAM);
    if (ioctl(sock.handle(), SIOCIFCREATE2, (caddr_t)&ifr) < 0)
    {
        int create_errno = errno;
        // EINVAL from SIOCIFCREATE2 means no registered cloner matched the
        // requested name, which typically happens when the backing kernel
        // module (e.g. if_epair) has not been loaded yet. Unlike ifconfig(8),
        // we talk to the ioctl directly, so load the module ourselves and
        // retry once before giving up.
        if(create_errno == EINVAL && Tool::loadIfKldModule(it_prefix->second))
        {
            strlcpy(ifr.ifr_name, ifname.c_str(), sizeof(ifr.ifr_name));
            create_errno = (ioctl(sock.handle(), SIOCIFCREATE2, (caddr_t)&ifr) < 0) ? errno : 0;
        }
        if(create_errno != 0)
        {
            LOG_S(ERROR) << "execCmdIfAdd: cannot create interface " << ifname << " (" << strerror(create_errno) << ")";
            sock.close();
            return JSON_RESULT_ERR;
        }
    }
    sock.close();
    ifname = std::string(ifr.ifr_name);

    if(iftype == JSON_PARAM_IF_TYPE_LAGG && have_laggproto)
    {
        if(!Tool::laggSetProto(ifname, laggproto))
        {
            LOG_S(ERROR) << "execCmdIfAdd: rolling back lagg " << ifname << " after failing to set " << JSON_PARAM_LAGG_PROTO;
            Tool::clonedIfDestroy(ifname);
            return JSON_RESULT_ERR;
        }
    }

    if(iftype == JSON_PARAM_IF_TYPE_LAGG && have_rrlimit)
    {
        if(!Tool::laggSetRRLimit(ifname, rrlimit))
        {
            LOG_S(ERROR) << "execCmdIfAdd: rolling back lagg " << ifname << " after failing to set " << JSON_PARAM_LAGG_ROUNDROBIN_LIMIT;
            Tool::clonedIfDestroy(ifname);
            return JSON_RESULT_ERR;
        }
    }

    for(const auto& member : members)
    {
        bool added = (iftype == JSON_PARAM_IF_TYPE_LAGG) ? Tool::laggAddPort(ifname, member) : Tool::clonedIfAddMember(ifname, member);
        if(!added)
        {
            LOG_S(ERROR) << "execCmdIfAdd: rolling back " << ifname << " after failing to add member " << member;
            Tool::clonedIfDestroy(ifname);
            return JSON_RESULT_ERR;
        }
    }

    if(iftype == JSON_PARAM_IF_TYPE_BRIDGE || iftype == JSON_PARAM_IF_TYPE_LAGG)
    {
        int curflags = Tool::getIfFlags(ifname);
        if(curflags == 0 || !Tool::setIfFlags(ifname, curflags | IFF_UP))
        {
            LOG_S(ERROR) << "execCmdIfAdd: rolling back " << ifname << " after failing to bring it up";
            Tool::clonedIfDestroy(ifname);
            return JSON_RESULT_ERR;
        }
    }

    json res = {};
    res[JSON_PARAM_RESULT] = JSON_PARAM_SUCC;
    json res_data = { { JSON_PARAM_IF_NAME, ifname } };
    if(!members.empty())
        res_data[JSON_PARAM_MEMBERS] = members;
    res[JSON_PARAM_DATA] = res_data;
    return res;
}

json NmWorkerSys::execCmdIfRemove(NmCommandData* pcmd)
{
    static const std::map<std::string, std::string> TYPE_PREFIXES =
    {
        { JSON_PARAM_IF_TYPE_TUN, "tun" },
        { JSON_PARAM_IF_TYPE_TAP, "tap" },
        { JSON_PARAM_IF_TYPE_BRIDGE, Tool::BRIDGE_IF_PREFIX },
        { JSON_PARAM_IF_TYPE_EPAIR, "epair" },
        { JSON_PARAM_IF_TYPE_LAGG, Tool::LAGG_IF_PREFIX }
    };
    std::string ifname;
    json cmd_json = {};

    try {
        cmd_json = pcmd->getJsonData();
        ifname = cmd_json[JSON_PARAM_DATA][JSON_PARAM_IF_NAME];
    } catch (std::exception& e) {
        LOG_S(ERROR) << "Exception in execCmdIfRemove - cannot get interface name from JSON";
        return JSON_RESULT_ERR;
    }

    if(!Tool::isValidIfName(ifname))
    {
        LOG_S(ERROR) << "execCmdIfRemove: invalid interface name " << ifname;
        return JSON_RESULT_ERR;
    }

    bool known_type = false;
    for(const auto& [type, prefix] : TYPE_PREFIXES)
    {
        if(ifname.starts_with(prefix))
        {
            known_type = true;
            break;
        }
    }
    if(!known_type)
    {
        LOG_S(ERROR) << "execCmdIfRemove: interface " << ifname << " is not a supported clone type (TUN/TAP/BRIDGE/EPAIR/LAGG)";
        return { { JSON_PARAM_RESULT, JSON_PARAM_ERR }, {JSON_PARAM_ERR, JSON_DATA_ERR_NOT_IMPLEMENTED} };
    }

    if(ifname.starts_with(TYPE_PREFIXES.at(JSON_PARAM_IF_TYPE_EPAIR)) && if_nametoindex(ifname.c_str()) == 0)
    {
        // Accept the bare "epair<N>" form in addition to the actual
        // "epair<N>a"/"epair<N>b" kernel interface names: destroying
        // either side destroys the whole pair, so resolve to the "a" side.
        std::string epair_a_side = ifname + "a";
        if(if_nametoindex(epair_a_side.c_str()) != 0)
            ifname = epair_a_side;
    }

    if(if_nametoindex(ifname.c_str()) == 0)
    {
        LOG_S(ERROR) << "execCmdIfRemove: interface " << ifname << " does not exist";
        return JSON_RESULT_NOTFOUND;
    }

    if(!Tool::clonedIfDestroy(ifname))
        return JSON_RESULT_ERR;

    return JSON_RESULT_SUCCESS;
}

json NmWorkerSys::execCmdRcConfRead(NmCommandData*)
{
    std::string rcconf_name;
    if( sp_conf!=nullptr )
        rcconf_name = sp_conf->getConfigValue(CONF_SECT_SYSTEM, CONF_KEY_RCCONF_FILE);
    if(rcconf_name.empty())
        rcconf_name = RCCONF_FILENAME_DEFAULT;
    LOG_S(INFO) << "execCmdRcConfRead: Trying to read from " << rcconf_name;
    prcConf = std::make_unique<RcConf>(rcconf_name, 0);
    if(!prcConf->iniLoad())
    {
        LOG_S(ERROR) << "execCmdRcConfRead: Cannot load " << rcconf_name;
        return JSON_RESULT_ERR;
    }
    return prcConf->getRcIpConfig();
}

json NmWorkerSys::execCmdRcConfWrite(NmCommandData *pcmd)
{
    json cmd_json = {};
    json jdata = {};
    short n_backups = -1;
    std::string str_nbackups;
    std::string rcconf_name;
    if( sp_conf!=nullptr )
        rcconf_name = sp_conf->getConfigValue(CONF_SECT_SYSTEM, CONF_KEY_RCCONF_FILE);
    if(rcconf_name.empty())
        rcconf_name = RCCONF_FILENAME_DEFAULT;
    LOG_S(INFO) << "execCmdRcConfWrite: Trying to write to " << rcconf_name;
    if( sp_conf!=nullptr )
        str_nbackups = sp_conf->getConfigValue(CONF_SECT_SYSTEM, CONF_KEY_RCCONF_BACKUPS);
    if(str_nbackups.empty())
        n_backups = (short)strtol(RCCONF_BACKUPS_DEFAULT.c_str(), nullptr, 10);
    else
        n_backups = (short)strtol(str_nbackups.c_str(), nullptr, 10);
    if(n_backups==0)
        LOG_S(WARNING) << "File " << rcconf_name << " will be overwritten, backups are disabled in " << sp_conf->getConfigFileName()
                       << " : " << CONF_KEY_RCCONF_FILE << "=" << str_nbackups;
    prcConf = std::make_unique<RcConf>(rcconf_name, n_backups);
    try {
        cmd_json = pcmd->getJsonData();
        jdata = cmd_json[JSON_PARAM_DATA];
    } catch (std::exception& e) {
        LOG_S(ERROR) << "Exception in execCmdRcConfWrite - cannot get data from json";
        return JSON_RESULT_ERR;
    }
    if(!prcConf->rotateRcConfFile())
        return JSON_RESULT_ERR;
    if(!prcConf->setRcIpConfig(jdata))
        return JSON_RESULT_ERR;
    else
        return JSON_RESULT_SUCCESS;
}

json NmWorkerSys::execCmdJailList(NmCommandData*)
{
    /* Moved to Tool::getJails() */
    /*
    int jflags=0;
    std::vector<JailParam> vect_jails;
    std::vector<nlohmann::json> vect_jails_json;
    nlohmann::json res_jails = {};
    nlohmann::json res_data = {};

    JailParam::InitRequestParams();
    while(true)
    {
        std::unique_ptr<JailParam> pjail = std::make_unique<JailParam>();
        JailParam::Lastjid = pjail->GetJailParams(jflags);
        if(JailParam::Lastjid<0)
            break;
        vect_jails.push_back(*pjail.get());
    }
    if (errno != 0 && errno != ENOENT)
    {
        LOG_S(ERROR) << "Error in execCmdJailList, got from GetJailParams: " << jail_errmsg;
        if(vect_jails.empty())
        {
            return JSON_RESULT_ERR;
        }
    }
    */
    nlohmann::json res_jails = {};
    nlohmann::json res_data = {};
    std::vector<nlohmann::json> vect_jails_json;
    std::vector<JailParam> vect_jails = Tool::getJails();
    for(auto jail : vect_jails)
    {
        vect_jails_json.push_back(jail.GetJailJson());
    }
    res_data[JSON_PARAM_JAILS] = vect_jails_json;
    res_jails[JSON_PARAM_RESULT] = JSON_PARAM_SUCC;
    res_jails[JSON_PARAM_DATA] = res_data;
    return res_jails;
}

json NmWorkerSys::execCmdResolvConfRead(NmCommandData*)
{
    std::string resolvconf_name;
    if( sp_conf!=nullptr )
        resolvconf_name = sp_conf->getConfigValue(CONF_SECT_SYSTEM, CONF_KEY_RESOLVCONF_FILE);
    if(resolvconf_name.empty())
        resolvconf_name = RESOLVCONF_FILENAME_DEFAULT;
    LOG_S(INFO) << "execCmdResolvConfRead: Trying to read from " << resolvconf_name;
    presConf = std::make_unique<ResolvConf>(resolvconf_name, 0);
    return presConf->getConfig();

}

json NmWorkerSys::execCmdResolvConfWrite(NmCommandData *pcmd)
{
    json cmd_json = {};
    json jdata = {};
    std::string resolvconf_name;
    short num_bkp = std::stol(RESOLVCONF_BACKUPS_DEFAULT);
    if( sp_conf!=nullptr )
    {
        resolvconf_name = sp_conf->getConfigValue(CONF_SECT_SYSTEM, CONF_KEY_RESOLVCONF_FILE);
        auto strnum = sp_conf->getConfigValue(CONF_SECT_SYSTEM, CONF_KEY_RESOLVCONF_BACKUPS);
        if(!strnum.empty())
        {
            try
            {
                num_bkp = std::stol(strnum);
            }
            catch (std::exception& e)
            {
                LOG_S(WARNING) << "Invalid value for " << CONF_KEY_RESOLVCONF_BACKUPS << " in config file " <<
                    sp_conf->getConfigFileName() << ": " << strnum;
            }
        }
    }
    if(resolvconf_name.empty())
        resolvconf_name = RESOLVCONF_FILENAME_DEFAULT;
    LOG_S(INFO) << "execCmdResolvConfWrite: Trying to write to " << resolvconf_name;
    presConf = std::make_unique<ResolvConf>(resolvconf_name, num_bkp);
    try
    {
        cmd_json = pcmd->getJsonData();
        jdata = cmd_json[JSON_PARAM_DATA];
    }
    catch (std::exception& e)
    {
        LOG_S(ERROR) << "Exception in execCmdResolvConfWrite - cannot get data from json";
        return JSON_RESULT_ERR;
    }
    if(!presConf->rotateConfFile())
        return JSON_RESULT_ERR;
    if(!presConf->setConfig(jdata))
        return JSON_RESULT_ERR;
    else
        return JSON_RESULT_SUCCESS;
}
