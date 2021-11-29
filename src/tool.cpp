#include "tool.h"
#include "addr.h"

std::shared_ptr<addr> tool::getAddrFromJson(json cmd)
{
    json cmd_json_data = {};
    std::string str_ifaddr = "";
    std::string str_ifmask = "";
    std::string str_ifgw = "";
    std::string str_ifbcast = "";
    short ip_family = 0;
    std::shared_ptr<address_base> sp_ifaddr = nullptr;
    std::shared_ptr<address_base> sp_ifmask = nullptr;
    std::shared_ptr<address_base> sp_ifdata = nullptr;
    ipaddr_type ip_type;
    std::shared_ptr<addr> sp_addr = nullptr;

    if(!cmd.contains(JSON_PARAM_DATA))
    {
        LOG_S(ERROR) << "getAddrFromJson - JSON does not contain data section";
        return sp_addr;
    }

    cmd_json_data = cmd[JSON_PARAM_DATA];

    if( cmd_json_data.contains(JSON_PARAM_IPV4_ADDR) ||
        cmd_json_data.contains(JSON_PARAM_IPV4_MASK) ||
        cmd_json_data.contains(JSON_PARAM_IPV4_BCAST) ||
        cmd_json_data.contains(JSON_PARAM_IPV4_GW) )
    {
        ip_family = AF_INET;
    }
    else if( cmd_json_data.contains(JSON_PARAM_IPV6_ADDR) ||
             cmd_json_data.contains(JSON_PARAM_IPV6_MASK) ||
             cmd_json_data.contains(JSON_PARAM_IPV6_BCAST) ||
             cmd_json_data.contains(JSON_PARAM_IPV6_GW) )
    {
        ip_family = AF_INET6;
    }
    else
    {
        LOG_S(ERROR) << "getAddrFromJson - cannot determine IP address family";
        return sp_addr;
    }

    switch(ip_family)
    {
        case AF_INET:
            if(cmd_json_data.contains(JSON_PARAM_IPV4_ADDR))
                str_ifaddr = cmd_json_data[JSON_PARAM_IPV4_ADDR];
            if(cmd_json_data.contains(JSON_PARAM_IPV4_MASK))
                str_ifmask = cmd_json_data[JSON_PARAM_IPV4_MASK];
            if(cmd_json_data.contains(JSON_PARAM_IPV6_BCAST))
            {
                str_ifbcast = cmd_json_data[JSON_PARAM_IPV6_BCAST];
                ip_type = ipaddr_type::BCAST;
            }
            else if(cmd_json_data.contains(JSON_PARAM_IPV4_GW))
            {
                str_ifgw = cmd_json_data[JSON_PARAM_IPV4_GW];
                ip_type = ipaddr_type::PPP;
            }
            break;
        case AF_INET6:
            if(cmd_json_data.contains(JSON_PARAM_IPV6_ADDR))
                str_ifaddr = cmd_json_data[JSON_PARAM_IPV6_ADDR];
            if(cmd_json_data.contains(JSON_PARAM_IPV6_MASK))
                str_ifmask = cmd_json_data[JSON_PARAM_IPV6_MASK];
            if(cmd_json_data.contains(JSON_PARAM_IPV6_BCAST))
            {
                str_ifbcast = cmd_json_data[JSON_PARAM_IPV6_BCAST];
                ip_type = ipaddr_type::BCAST;
            }
            else if(cmd_json_data.contains(JSON_PARAM_IPV6_GW))
            {
                str_ifgw = cmd_json_data[JSON_PARAM_IPV6_GW];
                ip_type = ipaddr_type::PPP;
            }
            break;
    }

    if( str_ifaddr.empty() && str_ifmask.empty() && str_ifgw.empty() && str_ifbcast.empty() )
    {
        LOG_S(ERROR) << "getAddrFromJson - cannot get IP address from JSON";
        return sp_addr;
    }

    switch(ip_family)
    {
        case AF_INET:
            try
            {
                if(!str_ifaddr.empty())
                    sp_ifaddr = std::make_shared<address_ip4>(str_ifaddr);
                if(!str_ifmask.empty())
                    sp_ifmask = std::make_shared<address_ip4>(str_ifmask);
                if( ip_type==ipaddr_type::BCAST && !str_ifbcast.empty() )
                    sp_ifdata=std::make_shared<address_ip4>(str_ifbcast);
                else if( ((ip_type==ipaddr_type::PPP)||(ip_type==ipaddr_type::ROUTE)) && (!str_ifgw.empty()) )
                    sp_ifdata=std::make_shared<address_ip4>(str_ifgw);
            }
            catch (std::exception& e)
            {
                LOG_S(ERROR) << "Cannot create ip4 address from JSON parameters";
                return sp_addr;
            }
            break;
        case AF_INET6:
            try
            {
                if(!str_ifaddr.empty())
                    sp_ifaddr = std::make_shared<address_ip6>(str_ifaddr);
                if(!str_ifmask.empty())
                    sp_ifmask = std::make_shared<address_ip6>(str_ifmask);
                if( ip_type==ipaddr_type::BCAST && !str_ifbcast.empty() )
                    sp_ifdata=std::make_shared<address_ip6>(str_ifbcast);
                else if( ((ip_type==ipaddr_type::PPP)||(ip_type==ipaddr_type::ROUTE)) && (!str_ifgw.empty()) )
                    sp_ifdata=std::make_shared<address_ip6>(str_ifgw);
            }
            catch (std::exception& e)
            {
                LOG_S(ERROR) << "Cannot create ip6 address from JSON parameters";
                return sp_addr;
            }
            break;
    }

    try {
        sp_addr = std::make_shared<addr>(sp_ifaddr, sp_ifmask, sp_ifdata, ip_type, false);
    } catch (std::exception& e) {
        LOG_S(ERROR) << "Cannot create if_addr from JSON parameters";
        return nullptr;
    }
    return sp_addr;
}

int tool::getIfFlags(std::string ifname)
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

bool tool::setIfFlags(std::string ifname, int setflags)
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

std::string tool::getIfPrimaryAddr4(std::string ifname)
{
    struct ifreq ifr;
    std::string str_addr4;
    struct sockaddr_in *sa_in;
    char address[INET_ADDRSTRLEN];

    memset(&ifr, 0, sizeof(struct ifreq));
    strlcpy(ifr.ifr_name, ifname.c_str(), sizeof(ifr.ifr_name));
    ifr.ifr_addr.sa_family = AF_INET;
    sockpp::socket sock = sockpp::socket::create(AF_INET, SOCK_DGRAM);

    if (ioctl(sock.handle(), SIOCGIFADDR, (caddr_t)&ifr) < 0)
    {
        LOG_S(ERROR) << "Cannot get primary IPv4 address for interface: " << ifname;
        sock.close();
        return str_addr4;
    }
    sock.close();

    sa_in = (struct sockaddr_in *) &ifr.ifr_addr;
    if(sa_in->sin_addr.s_addr!=0) {
        inet_ntop( AF_INET, &(sa_in->sin_addr), address, sizeof(address) );
        str_addr4 = std::string(address);
    }

    return str_addr4;
}

bool tool::isValidGw4(uint32_t addr, uint32_t mask, uint32_t gw)
{
    if( (addr==0) || (mask==0) || (gw==0) )
        return false;

    uint32_t result1 = (~mask) & addr;
    if(result1==0)   // No valuable bits in addr
        return false;
    uint32_t result2 = (~mask) & gw;
    if(result2==0)   // No valuable bits in gw
        return false;
    if( (result1^result2) == 0 ) // Overlapped addr and gw
        return false;
    if( ((~mask)^result2) ==0 )  // Gw is bcast address
        return false;

    result1 = mask & addr;
    result2 = mask & gw;
    if( (result1^result2) != 0 ) // addr and gw in different networks
        return false;

    return true;
}

bool tool::isValidBcast4(uint32_t addr, uint32_t mask, uint32_t bcast)
{
    if( (addr==0) || (mask==0) || (bcast==0) )
        return false;

    if( ((~mask)&addr) == 0 )    // No valuable bits in addr
        return false;

    uint32_t result1 = (~mask) & bcast;
    if( ((~mask)^result1) != 0 )   // Overlapped mask and bcast
        return false;

    result1 = mask & addr;
    uint32_t result2 = mask & bcast;
    if( (result1^result2) != 0 ) // addr and bcast in different networks
        return false;

    return true;
}
