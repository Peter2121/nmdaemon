#include "system_worker.h"

system_worker::system_worker()
{
    prcConf = nullptr;
}

system_worker::~system_worker()
{
    if(prcConf!=nullptr)
        delete prcConf;
}

nmscope system_worker::getScope()
{
    return nmscope::SYSTEM;
}

json system_worker::execCmd(nmcommand_data* pcmd)
{
    switch (pcmd->getCommand().cmd)
    {
        case nmcmd::IF_LIST :
            return execCmdIfList(pcmd);
        case nmcmd::IF_ADD :
        case nmcmd::IF_REMOVE :
            return { { JSON_PARAM_RESULT, JSON_PARAM_ERR }, {JSON_PARAM_ERR, JSON_DATA_ERR_NOT_IMPLEMENTED} };
        case nmcmd::IF_ENABLE :
            return execCmdIfEnable(pcmd);
        case nmcmd::IF_DISABLE :
            return execCmdIfDisable(pcmd);
        case nmcmd::RCCONF_READ :
            return execCmdRcConfRead(pcmd);
        case nmcmd::RCCONF_WRITE :
            return execCmdRcConfWrite(pcmd);
        default :
            return { { JSON_PARAM_RESULT, JSON_PARAM_ERR }, {JSON_PARAM_ERR, JSON_DATA_ERR_INVALID_COMMAND} };
    }
}

bool system_worker::isValidCmd(nmcommand_data* pcmd)
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

json system_worker::execCmdIfList(nmcommand_data*)
{
    struct ifaddrs * ifaddrs_ptr;
    nlohmann::json retIfListJson;
    std::vector<nlohmann::json> vectIfsJson;
    int status;
    std::map<std::string, interface> ifMap;

    status = getifaddrs (& ifaddrs_ptr);
    if (status == -1) {
        LOG_S(ERROR) << "Error in getifaddrs: " << errno << " (" << strerror (errno) << ")";
    }

    while (ifaddrs_ptr) {
        ifMap[ifaddrs_ptr->ifa_name].setName(std::string(ifaddrs_ptr->ifa_name));
        ifMap[ifaddrs_ptr->ifa_name].addAddress(ifaddrs_ptr);
        ifaddrs_ptr = ifaddrs_ptr->ifa_next;
    }

    freeifaddrs (ifaddrs_ptr);

    for (auto iface=ifMap.begin(); iface!=ifMap.end(); ++iface)
    {
      vectIfsJson.push_back(iface->second.getIfJson());
    }

    nlohmann::json addrJson;

    retIfListJson[JSON_PARAM_INTERFACES] = vectIfsJson;

    return retIfListJson;
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
json system_worker::execCmdIfEnable(nmcommand_data* pcmd) {
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
    int curflags = tool::getIfFlags(ifname);
    if(curflags==0)
    {
        return JSON_RESULT_ERR;
    }
    curflags |= cmdflag;
    if(!tool::setIfFlags(ifname, curflags))
    {
        return JSON_RESULT_ERR;
    }
    return JSON_RESULT_SUCCESS;
}

json system_worker::execCmdIfDisable(nmcommand_data* pcmd) {
    struct ifreq ifr;
    std::string ifname = "";
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
    int curflags = tool::getIfFlags(ifname);
    if(curflags==0)
    {
        return JSON_RESULT_ERR;
    }
    curflags &= ~cmdflag;
    if(!tool::setIfFlags(ifname, curflags))
    {
        return JSON_RESULT_ERR;
    }
    return JSON_RESULT_SUCCESS;
}

json system_worker::execCmdRcConfRead(nmcommand_data*)
{
    if(prcConf!=nullptr)
        delete prcConf;
    prcConf = new rcconf(RCCONF_FILENAME);
    if(!prcConf->iniLoad())
    {
        LOG_S(ERROR) << "execCmdRcConfRead: Cannot load " << RCCONF_FILENAME;
        return JSON_RESULT_ERR;
    }
    return prcConf->getRcIpConfig();
}

json system_worker::execCmdRcConfWrite(nmcommand_data*)
{
    return { { JSON_PARAM_RESULT, JSON_PARAM_ERR }, {JSON_PARAM_ERR, JSON_DATA_ERR_NOT_IMPLEMENTED} };
}
