#include "interface.h"

Interface::Interface(std::string name) : strName(name), hasIPv4(false), hasIPv6(false),
                                         isIfUp(false), isDhcpEnabled(false), ifStatus(MediaStatus::UNKNOWN)
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
