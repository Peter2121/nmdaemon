#include "addr.h"

addr::addr(struct ifaddrs* ifa) : flags(0), isAddrPrimary(false)
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

    std::string if_name = std::string(ifa->ifa_name);
    std::string def_addr;

    switch(ifa->ifa_addr->sa_family)
    {
        case AF_INET:
            spIpAddress = std::make_shared<address_ip4>(reinterpret_cast<sockaddr_in*>(ifa->ifa_addr));
            if(ifa->ifa_netmask)
            {
                spIpMask = std::make_shared<address_ip4>(reinterpret_cast<sockaddr_in*>(ifa->ifa_netmask));
            }
            else
                throw nmExcept;

            def_addr = tool::getIfPrimaryAddr4(if_name);
            if( !def_addr.empty() && (def_addr==spIpAddress->getStrAddr()) )
                isAddrPrimary = true;
            switch(ipType)
            {
                case ipaddr_type::BCAST:
                case ipaddr_type::LOOPBACK:
                    if(ifa->ifa_broadaddr)
                    {
                        spIpData = std::make_shared<address_ip4>(reinterpret_cast<sockaddr_in*>(ifa->ifa_broadaddr));
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
                    }
                    break;
                default:
                    break;
            }
            break;  // end of case AF_INET
        case AF_INET6:
            spIpAddress = std::make_shared<address_ip6>(reinterpret_cast<sockaddr_in6*>(ifa->ifa_addr));
            if(ifa->ifa_netmask)
            {
                spIpMask = std::make_shared<address_ip6>(reinterpret_cast<sockaddr_in6*>(ifa->ifa_netmask));
            }
            else
                throw nmExcept;

            switch(ipType)
            {
                case ipaddr_type::BCAST:
                    if(ifa->ifa_broadaddr)
                    {
                        spIpData = std::make_shared<address_ip6>(reinterpret_cast<sockaddr_in6*>(ifa->ifa_broadaddr));
                    }
                    break;
                case ipaddr_type::PPP:
                case ipaddr_type::ROUTE:
                    // We can have only IPv4 or only IPv6 ifa_dstaddr not the both
                    // In such case ifa->ifa_dstaddr->sa_family is 0
                    if( (ifa->ifa_dstaddr) && (ifa->ifa_dstaddr->sa_family==AF_INET6) )
                    {
                        spIpData = std::make_shared<address_ip6>(reinterpret_cast<sockaddr_in6*>(ifa->ifa_dstaddr));
                    }
                    break;
                default:
                    break;
            }
            break;  // end of case AF_INET6
        case AF_LINK:
            spIpAddress = std::make_shared<address_link>(reinterpret_cast<sockaddr_dl*>(ifa->ifa_addr));
            break;
        default:
            throw nmExcept;
            break;
    }
}

addr::addr( std::shared_ptr<AddressBase> addr,
            std::shared_ptr<AddressBase> mask,
            std::shared_ptr<AddressBase> data,
            ipaddr_type type, bool up, int fl, bool primary) :
                ipType(type), spIpAddress(addr), spIpMask(mask), spIpData(data), flags(fl), isAddrUp(up), isAddrPrimary(primary)
{
}

addr::addr() :
    ipType(ipaddr_type::UNKNOWN), spIpAddress(nullptr), spIpMask(nullptr), spIpData(nullptr), flags(0), isAddrUp(false), isAddrPrimary(false)
{
}

addr::~addr()
{
}

void addr::setAddr(std::shared_ptr<AddressBase> spaddr)
{
    spIpAddress = spaddr;
}

void addr::setMask(std::shared_ptr<AddressBase> spmask)
{
    spIpMask = spmask;
}

void addr::setData(std::shared_ptr<AddressBase> spdata)
{
    spIpData = spdata;
}

void addr::setType(ipaddr_type type)
{
    ipType = type;
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
        },
        "PRIMARY" : true
    }
*/
    strMainTitle = std::string(magic_enum::enum_name(ipType));
    retAddrJson[JSON_PARAM_ADDR_TYPE] = strMainTitle;
    if(isAddrPrimary)
        retAddrJson[JSON_PARAM_ADDR_PRIMARY] = isAddrPrimary;

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

        switch(ipType)
        {
            case ipaddr_type::ROUTE:
                dataJson[JSON_PARAM_FLAGS] = getFlagsRoute();
                break;
            default:
            // TODO: add flags for other types of addresses
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

const AddressBase* addr::getAddrAB() const
{
    return spIpAddress.get();
}

const AddressBase* addr::getMaskAB() const
{
    return spIpMask.get();
}

const AddressBase* addr::getDataAB() const
{
    return spIpData.get();
}

const std::shared_ptr<AddressBase> addr::getAddr() const
{
    return spIpAddress;
}

const std::shared_ptr<AddressBase> addr::getMask() const
{
    return spIpMask;
}

const std::shared_ptr<AddressBase> addr::getData() const
{
    return spIpData;
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

int addr::getFlags() const
{
    return flags;
}

void addr::setFlags(int f)
{
    flags = f;
}

const std::vector<std::string> addr::getFlagsRoute() const
{
    std::vector<std::string> retFlags;
/*
#define	RTF_UP		0x1		// route usable
const std::string JSON_DATA_RTFLAG_UP = "UP";

#define	RTF_GATEWAY	0x2		// destination is a gateway
const std::string JSON_DATA_RTFLAG_GATEWAY = "GATEWAY";

#define	RTF_HOST	0x4		// host entry (net otherwise)
const std::string JSON_DATA_RTFLAG_HOST = "HOST";

#define	RTF_REJECT	0x8		// host or net unreachable
const std::string JSON_DATA_RTFLAG_REJECT = "REJECT";

#define	RTF_DYNAMIC	0x10	// created dynamically (by redirect)
const std::string JSON_DATA_RTFLAG_DYNAMIC = "DYNAMIC";

#define	RTF_MODIFIED	0x20	// modified dynamically (by redirect)
const std::string JSON_DATA_RTFLAG_MODIFIED = "MODIFIED";

#define RTF_LLINFO	0x400		// DEPRECATED - exists ONLY for backward compatibility
const std::string JSON_DATA_RTFLAG_LLINFO = "LLINFO";

#define RTF_STATIC	0x800		// manually added
const std::string JSON_DATA_RTFLAG_STATIC = "STATIC";

#define RTF_BLACKHOLE	0x1000	// just discard pkts (during updates)
const std::string JSON_DATA_RTFLAG_BLACKHOLE = "BLACKHOLE";

#define	RTF_FIXEDMTU	0x80000	// MTU was explicitly specified
const std::string JSON_DATA_RTFLAG_FIXEDMTU = "FIXEDMTU";

#define RTF_PINNED	0x100000	// route is immutable
const std::string JSON_DATA_RTFLAG_PINNED = "PINNED";

#define	RTF_LOCAL	0x200000 	// route represents a local address
const std::string JSON_DATA_RTFLAG_LOCAL = "LOCAL";

#define	RTF_BROADCAST	0x400000	// route represents a bcast address
const std::string JSON_DATA_RTFLAG_BROADCAST = "BROADCAST";

#define	RTF_MULTICAST	0x800000	// route represents a mcast address
const std::string JSON_DATA_RTFLAG_MULTICAST = "MULTICAST";

#define	RTF_STICKY	 0x10000000	// always route dst->src
const std::string JSON_DATA_RTFLAG_STICKY = "STICKY";
*/

    if(flags & RTF_UP)
        retFlags.push_back(JSON_DATA_RTFLAG_UP);

    if(flags & RTF_GATEWAY)
        retFlags.push_back(JSON_DATA_RTFLAG_GATEWAY);

    if(flags & RTF_HOST)
        retFlags.push_back(JSON_DATA_RTFLAG_HOST);

    if(flags & RTF_REJECT)
        retFlags.push_back(JSON_DATA_RTFLAG_REJECT);

    if(flags & RTF_DYNAMIC)
        retFlags.push_back(JSON_DATA_RTFLAG_DYNAMIC);

    if(flags & RTF_MODIFIED)
        retFlags.push_back(JSON_DATA_RTFLAG_MODIFIED);

    if(flags & RTF_LLINFO)
        retFlags.push_back(JSON_DATA_RTFLAG_LLINFO);

    if(flags & RTF_STATIC)
        retFlags.push_back(JSON_DATA_RTFLAG_STATIC);

    if(flags & RTF_BLACKHOLE)
        retFlags.push_back(JSON_DATA_RTFLAG_BLACKHOLE);

    if(flags & RTF_FIXEDMTU)
        retFlags.push_back(JSON_DATA_RTFLAG_FIXEDMTU);

    if(flags & RTF_PINNED)
        retFlags.push_back(JSON_DATA_RTFLAG_PINNED);

    if(flags & RTF_LOCAL)
        retFlags.push_back(JSON_DATA_RTFLAG_LOCAL);

    if(flags & RTF_BROADCAST)
        retFlags.push_back(JSON_DATA_RTFLAG_BROADCAST);

    if(flags & RTF_MULTICAST)
        retFlags.push_back(JSON_DATA_RTFLAG_MULTICAST);

    if(flags & RTF_STICKY)
        retFlags.push_back(JSON_DATA_RTFLAG_STICKY);

    return retFlags;
}

bool addr::isPrimary() const
{
    return isAddrPrimary;
}
