#include "addr.h"

addr::addr(struct ifaddrs* ifa)
{
    if(ifa==nullptr)
        throw nmExcept;
    if(ifa->ifa_addr==nullptr)
        throw nmExcept;

    if(ifa->ifa_addr->sa_family == AF_LINK)
        ipType = ipaddr_type::LINK;
    else if( (ifa->ifa_flags & IFF_POINTOPOINT ) != 0 )
        ipType = ipaddr_type::PPP;
    else if(ifa->ifa_addr->sa_family==AF_INET6)
        ipType = ipaddr_type::BCAST;
    else if((ifa->ifa_broadaddr) && (ifa->ifa_addr->sa_family==AF_INET))
        ipType = ipaddr_type::BCAST;
    else
        throw nmExcept;

    if( (ifa->ifa_flags & IFF_LOOPBACK) != 0 )
        ipType = ipaddr_type::LOOPBACK;

    if( (ifa->ifa_flags & IFF_UP) != 0 )
        isAddrUp = true;
    else
        isAddrUp = false;

//    ipAddress = nullptr;
//    ipMask = nullptr;
//    ipData = nullptr;
//    memAddr = false;
//    memMask = false;
//    memData = false;

    switch(ifa->ifa_addr->sa_family)
    {
        case AF_INET:
            spIpAddress = std::make_shared<address_ip4>(reinterpret_cast<sockaddr_in*>(ifa->ifa_addr));
//            ipAddress = spIpAddress.get();
//                ipAddress = new address_ip4((sockaddr_in*)ifa->ifa_addr);
//                memAddr = true;

            if(ifa->ifa_netmask)
            {
                spIpMask = std::make_shared<address_ip4>(reinterpret_cast<sockaddr_in*>(ifa->ifa_netmask));
//                ipMask = spIpMask.get();
//                ipMask = new address_ip4((sockaddr_in*)ifa->ifa_netmask);
//                memMask = true;
            }
            else
                throw nmExcept;

            switch(ipType)
            {
                case ipaddr_type::BCAST:
                case ipaddr_type::LOOPBACK:
                    if(ifa->ifa_broadaddr)
                    {
                        spIpData = std::make_shared<address_ip4>(reinterpret_cast<sockaddr_in*>(ifa->ifa_broadaddr));
//                        ipData = spIpData.get();
//                        ipData = new address_ip4((sockaddr_in*)ifa->ifa_broadaddr);
//                        memData = true;
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
                        spIpData = std::make_shared<address_ip4>(reinterpret_cast<sockaddr_in*>(ifa->ifa_dstaddr));
//                        ipData = spIpData.get();
//                        ipData = new address_ip4((sockaddr_in*)ifa->ifa_dstaddr);
//                        memData = true;
                    }
                    break;
                default:
                    break;
            }
            break;  // end of case AF_INET
        case AF_INET6:
            spIpAddress = std::make_shared<address_ip6>(reinterpret_cast<sockaddr_in6*>(ifa->ifa_addr));
//            ipAddress = spIpAddress.get();
//            ipAddress = new address_ip6((sockaddr_in6*)ifa->ifa_addr);
//            memAddr = true;

            if(ifa->ifa_netmask)
            {
                spIpMask = std::make_shared<address_ip6>(reinterpret_cast<sockaddr_in6*>(ifa->ifa_netmask));
//                ipMask = spIpMask.get();
//                ipMask = new address_ip6((sockaddr_in6*)ifa->ifa_netmask);
//                memMask = true;
            }
            else
                throw nmExcept;

            switch(ipType)
            {
                case ipaddr_type::BCAST:
                    if(ifa->ifa_broadaddr)
                    {
                        spIpData = std::make_shared<address_ip6>(reinterpret_cast<sockaddr_in6*>(ifa->ifa_broadaddr));
//                        ipData = spIpData.get();
//                        ipData = new address_ip6((sockaddr_in6*)ifa->ifa_broadaddr);
//                        memData = true;
                    }
                    break;
                case ipaddr_type::PPP:
                case ipaddr_type::ROUTE:
                    // We can have only IPv4 or only IPv6 ifa_dstaddr not the both
                    // In such case ifa->ifa_dstaddr->sa_family is 0
                    if( (ifa->ifa_dstaddr) && (ifa->ifa_dstaddr->sa_family==AF_INET6) )
                    {
                        spIpData = std::make_shared<address_ip6>(reinterpret_cast<sockaddr_in6*>(ifa->ifa_dstaddr));
//                        ipData = spIpData.get();
//                        ipData = new address_ip6((sockaddr_in6*)ifa->ifa_dstaddr);
//                        memData = true;
                    }
                    break;
                default:
                    break;
            }
            break;  // end of case AF_INET6
        case AF_LINK:
            spIpAddress = std::make_shared<address_link>(reinterpret_cast<sockaddr_dl*>(ifa->ifa_addr));
//            ipAddress = spIpAddress.get();
//            ipAddress = new address_link((sockaddr_dl*)ifa->ifa_addr);
//            memAddr = true;
            break;
        default:
            throw nmExcept;
            break;
    }
}
/*
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
*/
addr::addr(std::shared_ptr<address_base> addr, std::shared_ptr<address_base> mask, std::shared_ptr<address_base> data, ipaddr_type type, bool up)
{
    if(addr!=nullptr)
    {
        spIpAddress = addr;
//        ipAddress = addr.get();
    }
    else
    {
        spIpAddress = nullptr;
//        ipAddress = nullptr;
    }
    if(mask!=nullptr)
    {
        spIpMask = mask;
//        ipMask = mask.get();
    }
    else
    {
        spIpMask = nullptr;
//        ipMask = nullptr;
    }

    if(data!=nullptr)
    {
        spIpData = data;
//        ipData = data.get();
    }
    else
    {
        spIpData = nullptr;
//        ipData = nullptr;
    }

//    memAddr = false;
//    memMask = false;
//    memData = false;
    ipType = type;
    isAddrUp = up;
}

addr::~addr()
{
    /*
    if( (ipAddress != nullptr) && memAddr )
        delete ipAddress;
    if( (ipMask != nullptr) && memMask )
        delete ipMask;
    if( (ipData != nullptr) && memData )
        delete ipData;
    */
}
/*
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
*/
void addr::setData(std::shared_ptr<address_base> spdata)
{
    spIpData = spdata;
//    ipData = spdata.get();
//    memData = false;
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

    if(spIpAddress != nullptr)
    {
        switch(spIpAddress->getFamily())
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

        retAddrStr += strTitle + separator + spIpAddress->getStrAddr() + eol;

        switch(ipType)
        {
            case ipaddr_type::BCAST:
            case ipaddr_type::LOOPBACK:
                if(spIpMask != nullptr)
                {
                    strTitle = (spIpAddress->getFamily() == AF_INET) ? JSON_PARAM_IPV4_MASK : JSON_PARAM_IPV6_MASK;
                    retAddrStr += strTitle + separator + spIpMask->getStrAddr() + eol;
                }
                if(spIpData != nullptr)
                {
                    strTitle = (spIpAddress->getFamily() == AF_INET) ? JSON_PARAM_IPV4_BCAST : JSON_PARAM_IPV6_BCAST;
                    retAddrStr += strTitle + separator + spIpData->getStrAddr() + eol;
                }
                break;
            case ipaddr_type::PPP:
            case ipaddr_type::ROUTE:
                if(spIpMask != nullptr)
                {
                    strTitle = (spIpAddress->getFamily() == AF_INET) ? JSON_PARAM_IPV4_MASK : JSON_PARAM_IPV6_MASK;
                    retAddrStr += strTitle + separator + spIpMask->getStrAddr() + eol;
                }
                if(spIpData != nullptr)
                {
                    strTitle = (spIpAddress->getFamily() == AF_INET) ? JSON_PARAM_IPV4_GW : JSON_PARAM_IPV6_GW;
                    retAddrStr += strTitle + separator + spIpData->getStrAddr() + eol;
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

    if(spIpAddress != nullptr)
    {
        switch(spIpAddress->getFamily())
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

        dataJson[strTitle] = spIpAddress->getStrAddr();

        switch(ipType)
        {
            case ipaddr_type::BCAST:
            case ipaddr_type::LOOPBACK:
                if(spIpMask != nullptr)
                {
                    strTitle = (spIpAddress->getFamily() == AF_INET) ? JSON_PARAM_IPV4_MASK : JSON_PARAM_IPV6_MASK;
                    dataJson[strTitle] = spIpMask->getStrAddr();
                }
                if(spIpData != nullptr)
                {
                    strTitle = (spIpAddress->getFamily() == AF_INET) ? JSON_PARAM_IPV4_BCAST : JSON_PARAM_IPV6_BCAST;
                    dataJson[strTitle] = spIpData->getStrAddr();
                }
                break;
            case ipaddr_type::PPP:
            case ipaddr_type::ROUTE:
                if(spIpMask != nullptr)
                {
                    strTitle = (spIpAddress->getFamily() == AF_INET) ? JSON_PARAM_IPV4_MASK : JSON_PARAM_IPV6_MASK;
                    dataJson[strTitle] = spIpMask->getStrAddr();
                }
                if(spIpData != nullptr)
                {
                    strTitle = (spIpAddress->getFamily() == AF_INET) ? JSON_PARAM_IPV4_GW : JSON_PARAM_IPV6_GW;
                    dataJson[strTitle] = spIpData->getStrAddr();
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
    return spIpAddress->getFamily();
}

const address_base* addr::getAddrAB() const
{
    return spIpAddress.get();
}

const address_base* addr::getMaskAB() const
{
    return spIpMask.get();
}

const address_base* addr::getDataAB() const
{
    return spIpData.get();
}

bool addr::isValidIp() const
{
    switch(spIpAddress->getFamily())
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

    if(spIpAddress!=nullptr)
    {
        addr_nbo = ((struct sockaddr_in*)(spIpAddress->getSockAddr()))->sin_addr.s_addr;
    }
    if(spIpMask!=nullptr)
    {
        mask_nbo = ((struct sockaddr_in*)(spIpMask->getSockAddr()))->sin_addr.s_addr;
    }
    switch(ipType)
    {
        case ipaddr_type::BCAST:
            if(spIpData!=nullptr)
            {
                bcast_nbo = ((struct sockaddr_in*)(spIpData->getSockAddr()))->sin_addr.s_addr;
                return tool::isValidBcast4(addr_nbo, mask_nbo, bcast_nbo);
            }
            return false;
        case ipaddr_type::PPP:
            if(spIpData!=nullptr)
            {
                gw_nbo = ((struct sockaddr_in*)(spIpData->getSockAddr()))->sin_addr.s_addr;
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
