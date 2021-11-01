#include "addr.h"

addr::addr(struct ifaddrs* ifa)
{
    if(ifa->ifa_addr->sa_family == AF_LINK)
        ipType = ipaddr_type::LINK;
    else if( (ifa->ifa_flags & IFF_POINTOPOINT ) != 0 )
        ipType = ipaddr_type::PPP;
    else if(ifa->ifa_broadaddr)
        ipType = ipaddr_type::BCAST;
    else
        throw nmExcept;

    if( (ifa->ifa_flags & IFF_LOOPBACK) != 0 )
        ipType = ipaddr_type::LOOPBACK;

    if( (ifa->ifa_flags & IFF_UP) != 0 )
        isAddrUp = true;
    else
        isAddrUp = false;

    ipAddress = nullptr;
    ipMask = nullptr;
    ipData = nullptr;
    memAddr = false;
    memMask = false;
    memData = false;

    switch(ifa->ifa_addr->sa_family)
    {
        case AF_INET:
            if(ifa->ifa_addr)
            {
                ipAddress = new address_ip4((sockaddr_in*)ifa->ifa_addr);
                memAddr = true;
            }
            else
                throw nmExcept;

            if(ifa->ifa_netmask)
            {
                ipMask = new address_ip4((sockaddr_in*)ifa->ifa_netmask);
                memMask = true;
            }
            else
                throw nmExcept;

            switch(ipType)
            {
                case ipaddr_type::BCAST:
                case ipaddr_type::LOOPBACK:
                    if(ifa->ifa_broadaddr)
                    {
                        ipData = new address_ip4((sockaddr_in*)ifa->ifa_broadaddr);
                        memData = true;
                    }
                    else
                        throw nmExcept;
                    break;
                case ipaddr_type::PPP:
                case ipaddr_type::ROUTE:
                    // We can have only IPv4 or only IPv6 ifa_dstaddr not the both
                    // In such case ifa->ifa_dstaddr->sa_family is 0
                    if( (ifa->ifa_dstaddr) && (ifa->ifa_dstaddr->sa_family==AF_INET) )
                    {
                        ipData = new address_ip4((sockaddr_in*)ifa->ifa_dstaddr);
                        memData = true;
                    }
                    break;
                default:
                    break;
            }
            break;  // end of case AF_INET
        case AF_INET6:
            if(ifa->ifa_addr)
            {
                ipAddress = new address_ip6((sockaddr_in6*)ifa->ifa_addr);
                memAddr = true;
            }
            else
                throw nmExcept;

            if(ifa->ifa_netmask)
            {
                ipMask = new address_ip6((sockaddr_in6*)ifa->ifa_netmask);
                memMask = true;
            }
            else
                throw nmExcept;

            switch(ipType)
            {
                case ipaddr_type::BCAST:
                    if(ifa->ifa_broadaddr)
                    {
                        ipData = new address_ip6((sockaddr_in6*)ifa->ifa_broadaddr);
                        memData = true;
                    }
                    else
                        throw nmExcept;
                    break;
                case ipaddr_type::PPP:
                case ipaddr_type::ROUTE:
                    // We can have only IPv4 or only IPv6 ifa_dstaddr not the both
                    // In such case ifa->ifa_dstaddr->sa_family is 0
                    if( (ifa->ifa_dstaddr) && (ifa->ifa_dstaddr->sa_family==AF_INET6) )
                    {
                        ipData = new address_ip6((sockaddr_in6*)ifa->ifa_dstaddr);
                        memData = true;
                    }
                    break;
                default:
                    break;
            }
            break;  // end of case AF_INET6
        case AF_LINK:
            if(ifa->ifa_addr)
            {
                ipAddress = new address_link((sockaddr_dl*)ifa->ifa_addr);
                memAddr = true;
            }
            else
                throw nmExcept;
            break;
        default:
            throw nmExcept;
            break;
    }
}

addr::addr(address_base* addr, address_base* mask, address_base* data, ipaddr_type type, bool up, bool mm)
{
    ipAddress = addr;
    ipMask = mask;
    ipData = data;
    memAddr = mm;
    memMask = mm;
    memData = mm;
    ipType = type;
    isAddrUp = up;
}

addr::~addr()
{
    if( (ipAddress != nullptr) && memAddr )
        delete ipAddress;
    if( (ipMask != nullptr) && memMask )
        delete ipMask;
    if( (ipData != nullptr) && memData )
        delete ipData;
}

void addr::setAddr(address_base* addr, bool ma)
{
    if( (ipAddress != nullptr) && memAddr )
        delete ipAddress;
    ipAddress = addr;
    memAddr = ma;
}

void addr::setMask(address_base* mask, bool mm)
{
    if( (ipMask != nullptr) && memMask )
        delete ipMask;
    ipMask = mask;
    memMask = mm;
}

void addr::setData(address_base* data, bool md)
{
    if( (ipData != nullptr) && memData )
        delete ipData;
    ipData = data;
    memData = md;
}

// TODO: customize separator and eol strings (take them from arguments)
const std::string addr::getAddrString() const
{
    std::string retAddrStr = "";
    std::string separator = ":";
    std::string eol = "\n";
    std::string strTitle = "";

    strTitle = std::string(magic_enum::enum_name(ipType));
    retAddrStr += JSON_PARAM_ADDR_TYPE + separator + strTitle + eol;

    if(ipAddress != nullptr)
    {
        switch(ipAddress->getFamily())
        {
            case AF_INET:
                strTitle = JSON_PARAM_IPV4_ADDR;
                break;
            case AF_INET6:
                strTitle = JSON_PARAM_IPV6_ADDR;
                break;
            case AF_LINK:
                strTitle = JSON_PARAM_LINK_ADDR;
                break;
            default:
                break;
        }

        retAddrStr += strTitle + separator + ipAddress->getStrAddr() + eol;

        switch(ipType)
        {
            case ipaddr_type::BCAST:
            case ipaddr_type::LOOPBACK:
                if(ipMask != nullptr)
                {
                    strTitle = (ipAddress->getFamily() == AF_INET) ? JSON_PARAM_IPV4_MASK : JSON_PARAM_IPV6_MASK;
                    retAddrStr += strTitle + separator + ipMask->getStrAddr() + eol;
                }
                if(ipData != nullptr)
                {
                    strTitle = (ipAddress->getFamily() == AF_INET) ? JSON_PARAM_IPV4_BCAST : JSON_PARAM_IPV6_BCAST;
                    retAddrStr += strTitle + separator + ipData->getStrAddr() + eol;
                }
                break;
            case ipaddr_type::PPP:
            case ipaddr_type::ROUTE:
                if(ipMask != nullptr)
                {
                    strTitle = (ipAddress->getFamily() == AF_INET) ? JSON_PARAM_IPV4_MASK : JSON_PARAM_IPV6_MASK;
                    retAddrStr += strTitle + separator + ipMask->getStrAddr() + eol;
                }
                if(ipData != nullptr)
                {
                    strTitle = (ipAddress->getFamily() == AF_INET) ? JSON_PARAM_IPV4_GW : JSON_PARAM_IPV6_GW;
                    retAddrStr += strTitle + separator + ipData->getStrAddr() + eol;
                }
                break;
            case ipaddr_type::LINK:
            default:
                break;
        }
    }
    return retAddrStr;
}

const nlohmann::json addr::getAddrJson() const
{
    nlohmann::json retAddrJson;
    nlohmann::json dataJson;

    std::string strTitle = "";
    std::string strMainTitle = "";

/*
    IPv4 broadcast address JSON example:
    {
        "ADDRESS TYPE" : "BCAST",
        "BCAST" : {
            "IPV4 ADDRESS" : "192.168.211.21",
            "IPV4 SUBNET MASK" : "255.255.255.0",
            "IPV4 BROADCAST ADDRESS" : "192.168.211.255"
        }
    }
*/
    strMainTitle = std::string(magic_enum::enum_name(ipType));
    retAddrJson[JSON_PARAM_ADDR_TYPE] = strMainTitle;

    if(ipAddress != nullptr)
    {
        switch(ipAddress->getFamily())
        {
            case AF_INET:
                strTitle = JSON_PARAM_IPV4_ADDR;
                break;
            case AF_INET6:
                strTitle = JSON_PARAM_IPV6_ADDR;
                break;
            case AF_LINK:
                strTitle = JSON_PARAM_LINK_ADDR;
                break;
            default:
                break;
        }

        dataJson[strTitle] = ipAddress->getStrAddr();

        switch(ipType)
        {
            case ipaddr_type::BCAST:
            case ipaddr_type::LOOPBACK:
                if(ipMask != nullptr)
                {
                    strTitle = (ipAddress->getFamily() == AF_INET) ? JSON_PARAM_IPV4_MASK : JSON_PARAM_IPV6_MASK;
                    dataJson[strTitle] = ipMask->getStrAddr();
                }
                if(ipData != nullptr)
                {
                    strTitle = (ipAddress->getFamily() == AF_INET) ? JSON_PARAM_IPV4_BCAST : JSON_PARAM_IPV6_BCAST;
                    dataJson[strTitle] = ipData->getStrAddr();
                }
                break;
            case ipaddr_type::PPP:
            case ipaddr_type::ROUTE:
                if(ipMask != nullptr)
                {
                    strTitle = (ipAddress->getFamily() == AF_INET) ? JSON_PARAM_IPV4_MASK : JSON_PARAM_IPV6_MASK;
                    dataJson[strTitle] = ipMask->getStrAddr();
                }
                if(ipData != nullptr)
                {
                    strTitle = (ipAddress->getFamily() == AF_INET) ? JSON_PARAM_IPV4_GW : JSON_PARAM_IPV6_GW;
                    dataJson[strTitle] = ipData->getStrAddr();
                }
                break;
            case ipaddr_type::LINK:
            // TODO: show link data (speed, status etc.)
            default:
                break;
        }

        retAddrJson[strMainTitle] = dataJson;
    }

    return retAddrJson;
}

bool addr::isUp() const
{
    return isAddrUp;
}

short addr::getFamily() const
{
    return ipAddress->getFamily();
}

const address_base* addr::getAddrAB() const
{
    return ipAddress;
}

const address_base* addr::getMaskAB() const
{
    return ipMask;
}

const address_base* addr::getDataAB() const
{
    return ipData;
}

bool addr::isValidIp() const
{
    switch(ipAddress->getFamily())
    {
        case AF_INET6:
            return isValidIp6();
        case AF_INET:
            return isValidIp4();
        case AF_LINK:
            return true;
    }
        return true;
}

bool addr::isValidIp4() const
{
    uint32_t addr_nbo=0;
    uint32_t mask_nbo=0;
    uint32_t gw_nbo=0;
    uint32_t bcast_nbo=0;

    if(ipAddress!=nullptr)
    {
        addr_nbo = ((struct sockaddr_in*)(ipAddress->getSockAddr()))->sin_addr.s_addr;
    }
    if(ipMask!=nullptr)
    {
        mask_nbo = ((struct sockaddr_in*)(ipMask->getSockAddr()))->sin_addr.s_addr;
    }
    switch(ipType)
    {
        case ipaddr_type::BCAST:
            if(ipData!=nullptr)
            {
                bcast_nbo = ((struct sockaddr_in*)(ipData->getSockAddr()))->sin_addr.s_addr;
                return tool::isValidBcast4(addr_nbo, mask_nbo, bcast_nbo);
            }
            return false;
        case ipaddr_type::PPP:
            if(ipData!=nullptr)
            {
                gw_nbo = ((struct sockaddr_in*)(ipData->getSockAddr()))->sin_addr.s_addr;
                return tool::isValidGw4(addr_nbo, mask_nbo, gw_nbo);
            }
            return false;
        default:
            return true; // Not (yet) implemented
    }
}

bool addr::isValidIp6() const
{
    return true; // Not (yet) implemented
}
