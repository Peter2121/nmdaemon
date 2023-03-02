#include "interface.h"

Interface::Interface(std::string name) : strName(name), hasIPv4(false), hasIPv6(false),
                                         isIfUp(false), isIfRunning(false), isDhcpEnabled(false), ifStatus(MediaStatus::UNKNOWN)
{
}

Interface::~Interface()
{
}

void Interface::setName(std::string name)
{
    strName = name;
}

void Interface::addAddress(struct ifaddrs* ifa)
{
    try
    {
        std::shared_ptr<AddressGroup> spa = std::make_shared<AddressGroup>(ifa);
        spVectAddrs.push_back(spa);
        if(spa->isUp())
            isIfUp = true;
        if(spa->isRunning())
            isIfRunning = true;
        if(spa->getFamily() == AF_INET)
            hasIPv4 = true;
        if(spa->getFamily() == AF_INET6)
            hasIPv6 = true;
    } catch (std::exception& e)
    {
        LOG_S(ERROR) << "Cannot add address of family " << (int)ifa->ifa_addr->sa_family << " to interface " << ifa->ifa_name;
    }
}

void Interface::addAddress(std::shared_ptr<AddressGroup> spa)
{
    if(spa==nullptr)
    {
        LOG_S(ERROR) << "interface::addAddress error: NULL pointer received";
        return;
    }
    try
    {
        spVectAddrs.push_back(spa);
        if(spa->isUp())
            isIfUp = true;
        if(spa->isRunning())
            isIfRunning = true;
        if(spa->getFamily() == AF_INET)
            hasIPv4 = true;
        if(spa->getFamily() == AF_INET6)
            hasIPv6 = true;
    } catch (std::exception& e)
    {
        LOG_S(ERROR) << "Cannot add address: " << spa->getAddrString();
    }
}

std::string Interface::getName() const
{
    return strName;
}

/*
{
    "INTERFACE NAME" : "em0",
    "DHCP_ENABLED" : false,
    "ADDRESSES" :
    [
        {
            "ADDRESS TYPE" : "BCAST",
            "BCAST" : {
                "IPV4 ADDRESS" : "192.168.211.1",
                "IPV4 SUBNET MASK" : "255.255.255.0",
                "IPV4 BROADCAST ADDRESS" : "192.168.211.255"
        },
        {
            "ADDRESS TYPE" : "BCAST",
            "BCAST" : {
                "IPV4 ADDRESS" : "192.168.211.21",
                "IPV4 SUBNET MASK" : "255.255.255.0",
                "IPV4 BROADCAST ADDRESS" : "192.168.211.255"
        },
        {
            "ADDRESS TYPE" : "LOOPBACK",
            "LOOPBACK" : {
                "IPV4 ADDRESS" : "127.0.0.1",
                "IPV4 SUBNET MASK" : "255.0.0.0"
        },
        {
            "ADDRESS TYPE" : "LINK",
            "LINK" : {
                "LINK ADDRESS" : "5c:26:0a:83:d2:f1"
        }
    ]
}
*/

const nlohmann::json Interface::getIfJson() const
{
    nlohmann::json retIfJson;
    nlohmann::json addrJson;
    std::vector<nlohmann::json> vectAddrsJson;

    retIfJson[JSON_PARAM_IF_NAME] = strName;

    if(ifStatus != MediaStatus::UNKNOWN)
        retIfJson[JSON_PARAM_STATUS] = std::string(magic_enum::enum_name(ifStatus));

    if(!mediaDesc.empty())
        retIfJson[JSON_PARAM_MEDIA] = mediaDesc;

    if(isDhcpEnabled)
        retIfJson[JSON_PARAM_DHCP_ENABLED] = isDhcpEnabled;

    retIfJson[JSON_PARAM_UP] = isIfUp;
    retIfJson[JSON_PARAM_RUNNING] = isIfRunning;

    for(auto addr : spVectAddrs)
    {
        addrJson = addr->getAddrJson();
        vectAddrsJson.push_back(addrJson);
    }

    retIfJson[JSON_PARAM_ADDRESSES] = vectAddrsJson;

    return retIfJson;
}

MediaStatus Interface::getStatus() const
{
    return ifStatus;
}

void Interface::setStatus(MediaStatus status)
{
    ifStatus = status;
}

void Interface::setDhcpStatus(bool status)
{
    isDhcpEnabled = status;
}

bool Interface::getDhcpStatus() const
{
    return isDhcpEnabled;
}

const std::string &Interface::getMediaDesc() const
{
    return mediaDesc;
}

void Interface::setMediaDesc(const std::string &newMediaDesc)
{
    mediaDesc = newMediaDesc;
}

void Interface::findPrimaryAddress()
{
    std::vector<std::string> addr4_jails;
    std::vector<std::string> addr6_jails;
    std::vector<std::shared_ptr<AddressGroup>> addr4_if;
    std::vector<std::shared_ptr<AddressGroup>> addr6_if;
    std::string addr4_primary = "";
    std::string addr6_primary = "";

    if(spVectAddrs.empty())
        return;
    if(spVectAddrs.size() == 1)
    {
        spVectAddrs[0]->setPrimary(true);
        return;
    }
    bool is_dhcp_enabled = Tool::isDHCPEnabled(strName);
    std::string dhcp_addr = "";
    if(is_dhcp_enabled)
        dhcp_addr = Tool::getLastDHCPLeaseAddress(strName);
    std::vector<JailParam> vect_jails = Tool::getJails();
    std::string def_addr = Tool::getIfPrimaryAddr4(strName);
    for (auto &jail : vect_jails)
    {
        std::vector<std::string> vect_addr4 = jail.GetJailIpv4Addresses();
        for (auto addr4 : vect_addr4)
        {
            addr4_jails.push_back(addr4);
        }
        std::vector<std::string> vect_addr6 = jail.GetJailIpv4Addresses();
        for (auto addr6 : vect_addr6)
        {
            addr6_jails.push_back(addr6);
        }
    }
//  Create temp lists of if addresses
    for(auto addrg : spVectAddrs)
    {
        if(addrg->getFamily() == AF_INET)
            addr4_if.push_back(addrg);
        if(addrg->getFamily() == AF_INET6)
            addr6_if.push_back(addrg);
    }
//  Remove all jail addresses from temp lists
    auto end4 = std::remove_if( addr4_if.begin(), addr4_if.end(),
                                [&addr4_jails](std::shared_ptr<AddressGroup> const &ag)
                                {
                                    std::string addr = ag->getAddr()->getStrAddr();
                                    return ( std::find(addr4_jails.begin(), addr4_jails.end(), addr) != addr4_jails.end() );
                                }
                             );
    addr4_if.erase(end4, addr4_if.end());
    auto end6 = std::remove_if( addr6_if.begin(), addr6_if.end(),
                                [&addr6_jails](std::shared_ptr<AddressGroup> const &ag)
                                {
                                    std::string addr = ag->getAddr()->getStrAddr();
                                    return ( std::find(addr6_jails.begin(), addr6_jails.end(), addr) != addr6_jails.end() );
                                }
                             );
    addr6_if.erase(end6, addr6_if.end());
    if(!addr6_if.empty())
    {
//  TODO: Check for DHCP6 etc.
        addr6_primary = addr6_if[0]->getAddr()->getStrAddr();
    }
    if(!addr4_if.empty())
    {
        if(is_dhcp_enabled && !dhcp_addr.empty())
        {
            auto agprim = std::find_if( addr4_if.begin(), addr4_if.end(),
                                        [&dhcp_addr](std::shared_ptr<AddressGroup> const &ag)
                                        {
                                            return (ag->getAddr()->getStrAddr() == dhcp_addr);
                                        }
                                      );
            if(agprim != addr4_if.end())
            {
                addr4_primary = dhcp_addr;
            }
        }
        else if(!def_addr.empty())
        {
            auto agprim = std::find_if( addr4_if.begin(), addr4_if.end(),
                                        [&def_addr](std::shared_ptr<AddressGroup> const &ag)
                                        {
                                            return (ag->getAddr()->getStrAddr() == def_addr);
                                        }
                                      );
            if(agprim != addr4_if.end())
            {
                addr4_primary = def_addr;
            }
        }
        if(addr4_primary.empty())
            addr4_primary = addr4_if[0]->getAddr()->getStrAddr();
    }
    for(auto addrg : spVectAddrs)
    {
        if( (addrg->getFamily() == AF_INET) &&
            (addrg->getAddr()->getStrAddr() == addr4_primary) )
                addrg->setPrimary(true);
        if( (addrg->getFamily() == AF_INET6) &&
            (addrg->getAddr()->getStrAddr() == addr6_primary) )
                addrg->setPrimary(true);
    }
}
