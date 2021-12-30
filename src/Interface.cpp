#include "Interface.h"

Interface::Interface(std::string name) : strName(name), hasIPv4(false), hasIPv6(false), isIfUp(false)
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
        std::shared_ptr<addr> spa = std::make_shared<addr>(ifa);
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

void Interface::addAddress(std::shared_ptr<addr> spa)
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
    retIfJson[JSON_PARAM_DHCP_ENABLED] = tool::isDHCPEnabled(strName);

    for(auto addr : spVectAddrs)
    {
        addrJson = addr->getAddrJson();
        vectAddrsJson.push_back(addrJson);
    }

    retIfJson[JSON_PARAM_ADDRESSES] = vectAddrsJson;

    return retIfJson;
}

