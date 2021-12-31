#include "if_worker.h"

if_worker::if_worker()
{
}

if_worker::~if_worker()
{
}

NmScope if_worker::getScope()
{
    return NmScope::INTERFACE;
}

json if_worker::execCmd(nmcommand_data* pcmd)
{
    switch (pcmd->getCommand().cmd)
    {
        case NmCmd::IP_ADDR_SET :
            return execCmdIpAddrSet(pcmd);
        case NmCmd::IP4_ADDR_GET :
            return execCmdIpAddrGet(pcmd);
        case NmCmd::IP_ADDR_ADD :
            return execCmdIpAddrAdd(pcmd);
        case NmCmd::IP_ADDR_REMOVE :
            return execCmdIpAddrRemove(pcmd);
        case NmCmd::MTU_GET :
            return execCmdMtuGet(pcmd);
        case NmCmd::MTU_SET :
            return execCmdMtuSet(pcmd);
        case NmCmd::IP4_DHCP_ENABLE :
            return execCmdDHCPEnable(pcmd);
        case NmCmd::IP6_ADDR_GET :
        case NmCmd::IP6_DHCP_ENABLE :
        case NmCmd::MAC_ADDR_GET :
        case NmCmd::MAC_ADDR_SET :
            return { { JSON_PARAM_RESULT, JSON_PARAM_ERR }, {JSON_PARAM_ERR, JSON_DATA_ERR_NOT_IMPLEMENTED} };
        default :
            return { { JSON_PARAM_RESULT, JSON_PARAM_ERR }, {JSON_PARAM_ERR, JSON_DATA_ERR_INVALID_COMMAND} };
    }
}

bool if_worker::isValidCmd(nmcommand_data* pcmd)
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
static bool set_address_and_mask(struct in_aliasreq *ifra, host_t *addr, u_int8_t netmask)
{
    host_t *mask;
    memcpy(&ifra->ifra_addr, addr->get_sockaddr(addr), *addr->get_sockaddr_len(addr));
    // set the same address as destination address
    memcpy(&ifra->ifra_dstaddr, addr->get_sockaddr(addr), *addr->get_sockaddr_len(addr));
    mask = host_create_netmask(addr->get_family(addr), netmask);
    if (!mask)
    {
        DBG1(DBG_LIB, "invalid netmask: %d", netmask);
        return FALSE;
    }
    memcpy(&ifra->ifra_mask, mask->get_sockaddr(mask), *mask->get_sockaddr_len(mask));
    mask->destroy(mask);
    return TRUE;
}

//  Set the address using the more flexible SIOCAIFADDR/SIOCDIFADDR commands on FreeBSD 10 an newer.
static bool set_address_impl(private_tun_device_t *this, host_t *addr, u_int8_t netmask)
{
    struct in_aliasreq ifra;
    memset(&ifra, 0, sizeof(ifra));
    strncpy(ifra.ifra_name, this->if_name, IFNAMSIZ);
    if (this->address)
    {	// remove the existing address first
        if (!set_address_and_mask(&ifra, this->address, this->netmask))
        {
            return FALSE;
        }
        if (ioctl(this->sock, SIOCDIFADDR, &ifra) < 0)
        {
            DBG1(DBG_LIB, "failed to remove existing address on %s: %s",
                 this->if_name, strerror(errno));
            return FALSE;
        }
    }
    if (!set_address_and_mask(&ifra, addr, netmask))
    {
        return FALSE;
    }
    if (ioctl(this->sock, SIOCAIFADDR, &ifra) < 0)
    {
        DBG1(DBG_LIB, "failed to add address on %s: %s",
             this->if_name, strerror(errno));
        return FALSE;
    }
    return TRUE;
}

struct ifaliasreq {
    char    ifra_name[IFNAMSIZ];   // if name,	e.g. "en0"
    struct  sockaddr	ifra_addr;
    struct  sockaddr	ifra_broadaddr;
    struct  sockaddr	ifra_mask;
}


    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, ifname);
    ifr.ifr_addr.sa_family = AF_INET;
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if(s < 0)
    {
        perror ("socket error");
        exit(1);
    }
    result = ioctl(s, SIOCGIFADDR, &ifr);
    if(result < 0)
    {
        perror("ioctl ifr error");
        exit(1);
    }

*/

// The return value must be deallocated in calling function
std::shared_ptr<AddressBase> if_worker::getMainIfAddr(short family) // family: AF_INET / AF_INET6
{
    struct ifreq ifr;
    std::shared_ptr<AddressBase> spaddr=nullptr;

    if( (family!=AF_INET) && (family!=AF_INET6) )
    {
        LOG_S(ERROR) << "Call of getMainIfAddr with invalid family value";
        return nullptr;
    }

    memset(&ifr, 0, sizeof(struct ifreq));
    strlcpy(ifr.ifr_name, ifName.c_str(), sizeof(ifr.ifr_name));
    ifr.ifr_addr.sa_family = family;

    sockpp::socket sock = sockpp::socket::create(AF_INET, SOCK_DGRAM);
    if (ioctl(sock.handle(), SIOCGIFADDR, (caddr_t)&ifr) < 0)
    {
        LOG_S(INFO) << "Cannot get current interface " << ifName << " address";
        sock.close();
        return nullptr;
    }
    sock.close();

    try {
        switch(family)
        {
            case AF_INET:
                spaddr = std::make_unique<AddressIp4>((sockaddr_in*)&ifr.ifr_addr);
                break;
            case AF_INET6:
                spaddr = std::make_unique<AddressIp6>((sockaddr_in6*)&ifr.ifr_addr);
                break;
        }
    } catch (std::exception& e) {
        LOG_S(ERROR) << "Cannot get current interface " << ifName << "address";
        return nullptr;
    }

    LOG_S(INFO) << "Got current IP address: " + spaddr->getStrAddr() + " for interface " << ifName;
    return spaddr;
}

bool if_worker::removeIfAddr(std::shared_ptr<AddressBase> spaddrb)
{
    struct ifaliasreq ifra;
    memset(&ifra, 0, sizeof(struct ifaliasreq));

    strlcpy(ifra.ifra_name, ifName.c_str(), sizeof(ifra.ifra_name));
    memcpy(&ifra.ifra_addr, spaddrb->getSockAddr(), sizeof(struct sockaddr));

    sockpp::socket sock = sockpp::socket::create(AF_INET, SOCK_DGRAM);
    if (ioctl(sock.handle(), SIOCDIFADDR, (caddr_t)&ifra) < 0)
    {
        LOG_S(ERROR) << "Cannot remove address " << spaddrb->getStrAddr() << " from interface " << ifName;
        sock.close();
        return false;
    }

    sock.close();
    return true;
}

bool if_worker::addIfAddr(std::shared_ptr<AddressGroup> paddr)
{
    struct ifaliasreq ifra;
    memset(&ifra, 0, sizeof(struct ifaliasreq));
    const AddressBase* ab = nullptr;
    const struct sockaddr* sa = nullptr;

    strlcpy(ifra.ifra_name, ifName.c_str(), sizeof(ifra.ifra_name));

//  Set new address
    ab = paddr->getAddrAB();
    sa = (struct sockaddr*)ab->getSockAddr();
    memcpy(&ifra.ifra_addr, sa, sizeof(struct sockaddr));

//  Set new mask
    ab = paddr->getMaskAB();
    sa = (struct sockaddr*)ab->getSockAddr();
    memcpy(&ifra.ifra_mask, sa, sizeof(struct sockaddr));

    sockpp::socket sock = sockpp::socket::create(AF_INET, SOCK_DGRAM);
    if (ioctl(sock.handle(), SIOCAIFADDR, &ifra) < 0)
    {
        LOG_S(ERROR) << "Cannot add address:\n" << paddr->getAddrString() << "to interface " << ifName;
        sock.close();
        return false;
    }

    sock.close();
    return true;
}

// The return value must be deallocated in calling function
// TODO: make netmask optional
/********* MOVED TO tool::getAddrFromJson ************
addr* if_worker::getAddrFromJson(json cmd)
{
    json cmd_json_data = {};
    std::string str_ifaddr = "";
    std::string str_ifmask = "";
    short ip_family = 0;
    address_base* ifaddr = nullptr;
    address_base* ifmask = nullptr;
    addr* if_addr = nullptr;

    try {
        cmd_json_data = cmd[JSON_PARAM_DATA];

        if(cmd_json_data.contains(JSON_PARAM_IPV4_ADDR)) {
            str_ifaddr = cmd[JSON_PARAM_DATA][JSON_PARAM_IPV4_ADDR];
            str_ifmask = cmd[JSON_PARAM_DATA][JSON_PARAM_IPV4_MASK];
            ip_family = AF_INET;
        }
        else if(cmd_json_data.contains(JSON_PARAM_IPV6_ADDR))
        {
            str_ifaddr = cmd[JSON_PARAM_DATA][JSON_PARAM_IPV6_ADDR];
            str_ifmask = cmd[JSON_PARAM_DATA][JSON_PARAM_IPV6_MASK];
            ip_family = AF_INET6;
        }
    } catch (std::exception& e) {
        LOG_S(ERROR) << "Exception in getAddrFromJson - cannot get IP address/mask";
        return if_addr;
    }

    if(str_ifaddr.empty() || str_ifmask.empty()) {
        LOG_S(ERROR) << "Cannot get IP address/mask from JSON";
        return if_addr;
    }

    switch(ip_family) {
        case AF_INET:
            try {
                ifaddr = new address_ip4(str_ifaddr);
                ifmask = new address_ip4(str_ifmask);
            } catch (std::exception& e) {
                LOG_S(ERROR) << "Cannot create ip4 address from JSON parameters";
                return if_addr;
            }
            break;
        case AF_INET6:
            try {
                ifaddr = new address_ip6(str_ifaddr);
                ifmask = new address_ip6(str_ifmask);
            } catch (std::exception& e) {
                LOG_S(ERROR) << "Cannot create ip6 address from JSON parameters";
                return if_addr;
            }
            break;
        default:
            LOG_S(ERROR) << "Cannot get address type from JSON";
            return if_addr;
    }
    try {
        if_addr = new addr(ifaddr, ifmask, nullptr, ipaddr_type::BCAST, false, true);
    } catch (std::exception& e) {
        LOG_S(ERROR) << "Cannot create if_addr from JSON parameters";
        return nullptr;
    }
    return if_addr;
}
*/
json if_worker::execCmdIpAddrSet(nmcommand_data* pcmd)
{
    std::string str_ifaddr = "";
    std::string str_ifmask = "";
    std::shared_ptr<AddressBase> spcuraddr=nullptr;
    json cmd = {};

    try {
        cmd = pcmd->getJsonData();
        ifName = cmd[JSON_PARAM_DATA][JSON_PARAM_IF_NAME];
    } catch (std::exception& e) {
        LOG_S(ERROR) << "Exception in execCmdIpAddrSet - cannot get interface parameters";
        return JSON_RESULT_ERR;
    }

    auto if_addr = Tool::getAddrFromJson(cmd);
    if(if_addr==nullptr)
    {
        LOG_S(ERROR) << "Cannot decode JSON data";
        return JSON_RESULT_ERR;
    }

    spcuraddr = getMainIfAddr(if_addr->getAddrAB()->getFamily());
    if(spcuraddr==nullptr)
    {
        LOG_S(INFO) << "Cannot get current IP address of interface " << ifName;
    }
    else
    {
        LOG_S(INFO) << "Got current primary IP address " << spcuraddr->getStrAddr() << " from interface " << ifName;
        if(isDHCPEnabled())
        {
            if(!termDHCPClient())
            {
                LOG_S(ERROR) << "Cannot stop DHCP client";
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        if(isDHCPEnabled())
        {
            if(!killDHCPClient())
            {
                LOG_S(ERROR) << "Cannot kill DHCP client";
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        if(isDHCPEnabled())
        {
            LOG_S(ERROR) << "DHCP client still running for interface " << ifName;
            LOG_S(ERROR) << "Continue anyway...";
        }
        if(!removeIfAddr(spcuraddr)) {
            LOG_S(ERROR) << "Cannot remove current IP address from interface " << ifName;
            return JSON_RESULT_ERR;
        }
        LOG_S(INFO) << "Primary IP address deleted from interface " << ifName;
    }
    if(!addIfAddr(if_addr)) {
        LOG_S(ERROR) << "Cannot add new IP address to interface " << ifName;
        return JSON_RESULT_ERR;
    }
    LOG_S(INFO) << "New IP address set to interface " << ifName;
    return JSON_RESULT_SUCCESS;
}

json if_worker::execCmdIpAddrAdd(nmcommand_data* pcmd)
{
    std::string str_ifaddr = "";
    std::string str_ifmask = "";
    json cmd = {};

    try {
        cmd = pcmd->getJsonData();
        ifName = cmd[JSON_PARAM_DATA][JSON_PARAM_IF_NAME];
    } catch (std::exception& e) {
        LOG_S(ERROR) << "Exception in execCmdIpAddrSet - cannot get interface parameters";
        return JSON_RESULT_ERR;
    }

    auto if_addr = Tool::getAddrFromJson(cmd);
    if(if_addr==nullptr)
    {
        LOG_S(ERROR) << "Cannot decode JSON data";
        return JSON_RESULT_ERR;
    }

    if(!addIfAddr(if_addr)) {
        LOG_S(ERROR) << "Cannot add IP address to interface " << ifName;
        return JSON_RESULT_ERR;
    }
    LOG_S(INFO) << "New IP address added to interface " << ifName;
    return JSON_RESULT_SUCCESS;
}

json if_worker::execCmdIpAddrRemove(nmcommand_data* pcmd)
{
    std::string str_ifaddr = "";
    std::string str_ifmask = "";
    json cmd = {};

    try {
        cmd = pcmd->getJsonData();
        ifName = cmd[JSON_PARAM_DATA][JSON_PARAM_IF_NAME];
    } catch (std::exception& e) {
        LOG_S(ERROR) << "Exception in execCmdIpAddrSet - cannot get interface parameters";
        return JSON_RESULT_ERR;
    }

    std::shared_ptr<AddressGroup> if_addr = Tool::getAddrFromJson(cmd);
    if(if_addr==nullptr)
    {
        LOG_S(ERROR) << "Cannot decode JSON data";
        return JSON_RESULT_ERR;
    }

    if(!removeIfAddr(if_addr->getAddr())) {
        LOG_S(ERROR) << "Cannot remove IP address from interface " << ifName;
        return JSON_RESULT_ERR;
    }
    LOG_S(INFO) << "IP address removed from interface " << ifName;
    return JSON_RESULT_SUCCESS;
}

json if_worker::execCmdMtuGet(nmcommand_data* pcmd)
{
    struct ifreq ifr;
    json cmd = {};
    json res = {};
    json res_data = {};

    memset(&ifr, 0, sizeof(struct ifreq));

    try {
        cmd = pcmd->getJsonData();
        ifName = cmd[JSON_PARAM_DATA][JSON_PARAM_IF_NAME];
    } catch (std::exception& e) {
        LOG_S(ERROR) << "Exception in execCmdIpAddrSet - cannot get interface name";
        return JSON_RESULT_ERR;
    }

    strlcpy(ifr.ifr_name, ifName.c_str(), sizeof(ifr.ifr_name));

    sockpp::socket sock = sockpp::socket::create(AF_INET, SOCK_DGRAM);
    if (ioctl(sock.handle(), SIOCGIFMTU, (caddr_t)&ifr) < 0)
    {
        LOG_S(INFO) << "Cannot get current interface " << ifName << " MTU";
        sock.close();
        return JSON_RESULT_ERR;
    }

    sock.close();
    res_data[JSON_PARAM_MTU] = ifr.ifr_mtu;
    res[JSON_PARAM_RESULT] = JSON_PARAM_SUCC;
    res[JSON_PARAM_DATA] = res_data;
    return res;
}

json if_worker::execCmdMtuSet(nmcommand_data* pcmd)
{
    struct ifreq ifr;
    int mtu = 0;
    const int MTU_MAX = 9192;
    json cmd = {};
    json cmd_data = {};
    json res = {};
    json res_data = {};

    memset(&ifr, 0, sizeof(struct ifreq));

    try
    {
        cmd = pcmd->getJsonData();
        ifName = cmd[JSON_PARAM_DATA][JSON_PARAM_IF_NAME];
        cmd_data = cmd[JSON_PARAM_DATA];
        mtu = cmd_data[JSON_PARAM_MTU];
    } catch (std::exception& e)
    {
        LOG_S(ERROR) << "Exception in execCmdMtuSet - cannot get command data";
        return JSON_RESULT_ERR;
    }

    if( (mtu<=0) || (mtu>MTU_MAX) )
    {
            LOG_S(ERROR) << "execCmdMtuSet error - incorrect MTU value: " << mtu;
            return JSON_RESULT_ERR;
    }

    strlcpy(ifr.ifr_name, ifName.c_str(), sizeof(ifr.ifr_name));
    ifr.ifr_mtu = mtu;

    sockpp::socket sock = sockpp::socket::create(AF_INET, SOCK_DGRAM);
    if (ioctl(sock.handle(), SIOCSIFMTU, (caddr_t)&ifr) < 0)
    {
        LOG_S(INFO) << "Cannot set current interface " << ifName << " MTU to " << mtu;
        sock.close();
        return JSON_RESULT_ERR;
    }

    sock.close();
    return JSON_RESULT_SUCCESS;
}

json if_worker::execCmdIpAddrGet(nmcommand_data *pcmd)
{
    json cmd = {};
    json res = {};
    json res_data = {};
    std::string str_addr;

    cmd = pcmd->getJsonData();
    ifName = cmd[JSON_PARAM_DATA][JSON_PARAM_IF_NAME];

    str_addr = Tool::getIfPrimaryAddr4(ifName);
    if(!str_addr.empty())
    {
        res_data[JSON_PARAM_IPV4_ADDR] = str_addr;
        res[JSON_PARAM_RESULT] = JSON_PARAM_SUCC;
        res[JSON_PARAM_DATA] = res_data;
    }
    else
    {
        res = JSON_RESULT_ERR;
    }
    return res;
}

bool if_worker::isDHCPEnabled()
{
    return Tool::isDHCPEnabled(ifName);
}

bool if_worker::termDHCPClient()
{
    return Tool::termDHCPClient(ifName);
}

bool if_worker::killDHCPClient()
{
    return Tool::termDHCPClient(ifName, SIGKILL);
}

json if_worker::execCmdDHCPEnable(nmcommand_data* pcmd)
{
    json cmd = {};

    try {
        cmd = pcmd->getJsonData();
        ifName = cmd[JSON_PARAM_DATA][JSON_PARAM_IF_NAME];
    } catch (std::exception& e) {
        LOG_S(ERROR) << "Exception in execCmdDHCPEnable - cannot get interface parameters";
        return JSON_RESULT_ERR;
    }

    std::shared_ptr<AddressBase> sp_cur_addr = getMainIfAddr(AF_INET);
    if(!removeIfAddr(sp_cur_addr))
    {
        LOG_S(ERROR) << "Cannot remove current address " << sp_cur_addr->getStrAddr() << " from interface ifName";
        LOG_S(ERROR) << "Enabling DHCP on this interface anyway...";
    }

    if(enableDHCP())
        return JSON_RESULT_SUCCESS;
    else
        return JSON_RESULT_ERR;
}

bool if_worker::enableDHCP()
{
    if(isDHCPEnabled())
        return true;

    std::string cmd_dhcp_client = DHCP_CLIENT_EXEC + " " + ifName;
    LOG_S(INFO) << "Starting DHCP client as: " << cmd_dhcp_client;
    int retval = system(cmd_dhcp_client.c_str());
    LOG_S(INFO) << "DHCP client returned: " << retval;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if(isDHCPEnabled())
        return true;
    else
        return false;
}
