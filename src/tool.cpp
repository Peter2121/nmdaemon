#include "tool.h"

addr* tool::getAddrFromJson(json cmd)
{
    json cmd_json_data = {};
    std::string str_ifaddr = "";
    std::string str_ifmask = "";
    std::string str_ifgw = "";
    std::string str_ifbcast = "";
    short ip_family = 0;
    address_base* ifaddr = nullptr;
    address_base* ifmask = nullptr;
    address_base* ifdata = nullptr;
    ipaddr_type ip_type;
    addr* if_addr = nullptr;

    if(!cmd.contains(JSON_PARAM_DATA))
    {
        LOG_S(ERROR) << "getAddrFromJson - JSON does not contain data section";
        return if_addr;
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
        return if_addr;
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
        return if_addr;
    }

    switch(ip_family)
    {
        case AF_INET:
            try
            {
                if(!str_ifaddr.empty())
                    ifaddr = new address_ip4(str_ifaddr);
                if(!str_ifmask.empty())
                    ifmask = new address_ip4(str_ifmask);
                if( ip_type==ipaddr_type::BCAST && !str_ifbcast.empty() )
                    ifdata=new address_ip4(str_ifbcast);
                else if( ip_type==ipaddr_type::PPP && !str_ifgw.empty() )
                    ifdata=new address_ip4(str_ifgw);
            }
            catch (std::exception& e)
            {
                LOG_S(ERROR) << "Cannot create ip4 address from JSON parameters";
                return if_addr;
            }
            break;
        case AF_INET6:
            try
            {
                if(!str_ifaddr.empty())
                    ifaddr = new address_ip6(str_ifaddr);
                if(!str_ifmask.empty())
                    ifmask = new address_ip6(str_ifmask);
                if( ip_type==ipaddr_type::BCAST && !str_ifbcast.empty() )
                    ifdata=new address_ip6(str_ifbcast);
                else if( ip_type==ipaddr_type::PPP && !str_ifgw.empty() )
                    ifdata=new address_ip6(str_ifgw);
            }
            catch (std::exception& e)
            {
                LOG_S(ERROR) << "Cannot create ip6 address from JSON parameters";
                return if_addr;
            }
            break;
    }

    try {
        if_addr = new addr(ifaddr, ifmask, ifdata, ip_type, false, true);
    } catch (std::exception& e) {
        LOG_S(ERROR) << "Cannot create if_addr from JSON parameters";
        return nullptr;
    }
    return if_addr;
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
