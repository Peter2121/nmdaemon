#include "interface.h"

interface::interface(std::string name)
{
    strName = name;
    hasIPv4 = false;
    hasIPv6 = false;
    isIfUp = false;
}

interface::interface()
{
    strName = "";
    hasIPv4 = false;
    hasIPv6 = false;
    isIfUp = false;
}

interface::~interface()
{
    for(auto a : vectAddrs)
    {
        delete a;
    }
}

void interface::setName(std::string name)
{
    strName = name;
}

void interface::addAddress(struct ifaddrs* ifa)
{
    try
    {
        addr* a = new addr(ifa);
        vectAddrs.push_back(a);
        if(a->isUp())
            isIfUp = true;
        if(a->getFamily() == AF_INET)
            hasIPv4 = true;
        if(a->getFamily() == AF_INET6)
            hasIPv6 = true;
    } catch (std::exception& e)
    {
        LOG_S(ERROR) << "Cannot add address: " << ifa->ifa_name;
    }
}

const std::vector<addr*>* interface::getAddrs() const
{
    return &vectAddrs;
}

/*
{
    "INTERFACE NAME" : "em0",
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

const nlohmann::json interface::getIfJson() const
{
    nlohmann::json retIfJson;
    nlohmann::json addrJson;
    std::vector<nlohmann::json> vectAddrsJson;

    retIfJson[JSON_PARAM_IF_NAME] = strName;

    for(auto addr : vectAddrs)
    {
        addrJson = addr->getAddrJson();
        vectAddrsJson.push_back(addrJson);
    }

    retIfJson[JSON_PARAM_ADDRESSES] = vectAddrsJson;

    return retIfJson;
}

