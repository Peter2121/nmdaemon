#include "system_worker.h"

system_worker::system_worker()
{
    prcConf = nullptr;
}

system_worker::~system_worker()
{
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
    nlohmann::json res_ifaces = {};
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
    res_ifaces[JSON_PARAM_RESULT] = JSON_PARAM_SUCC;
    res_ifaces[JSON_PARAM_DATA] = vectIfsJson;

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
    std::string rcconf_name;
    if( sp_conf!=nullptr )
        rcconf_name = sp_conf->getConfigValue(CONF_SECT_SYSTEM, CONF_KEY_RCCONF_FILE);
    if(rcconf_name.empty())
        rcconf_name = RCCONF_FILENAME_DEFAULT;
    LOG_S(INFO) << "execCmdRcConfRead: Trying to read from " << rcconf_name;
    prcConf = std::make_unique<rcconf>(rcconf_name, 0);
    if(!prcConf->iniLoad())
    {
        LOG_S(ERROR) << "execCmdRcConfRead: Cannot load " << rcconf_name;
        return JSON_RESULT_ERR;
    }
    return prcConf->getRcIpConfig();
}

json system_worker::execCmdRcConfWrite(nmcommand_data *pcmd)
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
    LOG_S(INFO) << "execCmdRcConfRead: Trying to write to " << rcconf_name;
    if( sp_conf!=nullptr )
        str_nbackups = sp_conf->getConfigValue(CONF_SECT_SYSTEM, CONF_KEY_RCCONF_BACKUPS);
    if(str_nbackups.empty())
        n_backups = (short)strtol(RCCONF_BACKUPS_DEFAULT.c_str(), nullptr, 10);
    else
        n_backups = (short)strtol(str_nbackups.c_str(), nullptr, 10);
    if(n_backups==0)
        LOG_S(WARNING) << "File " << rcconf_name << " will be overwritten, backups are disabled in " << sp_conf->getConfigFileName()
                       << " : " << CONF_KEY_RCCONF_FILE << "=" << str_nbackups;
    prcConf = std::make_unique<rcconf>(rcconf_name, n_backups);
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
