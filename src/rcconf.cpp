#include "rcconf.h"

rcconf::rcconf(std::string path) : rcFileName(path)
{
    rcIniFile = new CIniFile();
}

rcconf::~rcconf()
{
    delete rcIniFile;
}

bool rcconf::iniLoad()
{
    return rcIniFile->Load(rcFileName);
}

// TODO: Decode ipv6 configuration
json rcconf::getRcIpConfig()
{
    std::string strDefaultRouter = "";
    std::string key = "";
    std::string_view key_view;
    std::string value = "";
    std::string element;
    std::string name;
    std::string_view element_view;
    std::stringstream ss;
    address_ip4* pipaddr4;
    address_ip4* pipmask4;
    address_ip4* pipgw4;
    addr* paddr4;
    const char DELIM = ' ';
    const std::string quotes = "\"";
    const std::string point = ".";
    size_t pos0,pos1,pos2;
    json jdata;
    json jif;
    json jrt;
    json jaliases;
    json jarInterfaces = json::array();
    json jarRoutes = json::array();
    json jret {};
    std::map<std::string, std::string> mapInterfaces;
    std::map<std::string, std::string> mapRoutes;
    std::set<std::string> setActiveRoutes;

    for( SecIndex::const_iterator itr = rcIniFile->GetSections().begin(); itr != rcIniFile->GetSections().end(); ++itr )
    {
        for( KeyIndex::const_iterator kitr = (*itr)->GetKeys().begin(); kitr != (*itr)->GetKeys().end(); kitr++ )
        {
            key = (*kitr)->GetKeyName();
            value = (*kitr)->GetValue();
            if(key==DEFAULT_ROUTE_KEY)
            {
                LOG_S(INFO) << "getRcIpConfig: found " << key << " : " << value;
                element = value;
                Trim(element, quotes);
                strDefaultRouter = element;
                continue;
            }
            if(key==ROUTES_KEY)
            {
                LOG_S(INFO) << "getRcIpConfig: found " << key << " : " << value;
                element = value;
                Trim(element, quotes);
                ss = std::stringstream(element);
                while (getline (ss, element, DELIM))
                {
                    element_view = element;
                    element_view.remove_prefix(std::min(element.find_first_not_of(" "), element.size()));
                    element_view.remove_suffix(std::min(element.size() - element.find_last_not_of(" ") - 1, element.size()));
                    setActiveRoutes.insert(static_cast<std::string>(element_view));
                }
                LOG_S(INFO) << "getRcIpConfig: found " << setActiveRoutes.size() << " active routes";
                continue;
            }
            key_view = key;
            if(key_view.starts_with(IFCONFIG_KEY_PREFIX))
            {
                LOG_S(INFO) << "getRcIpConfig: found " << key << " : " << value;
                element = key.substr(IFCONFIG_KEY_PREFIX.length());
                mapInterfaces.emplace(element,value);
                continue;
            }
            if(key_view.starts_with(ROUTE_KEY_PREFIX))
            {
                LOG_S(INFO) << "getRcIpConfig: found " << key << " : " << value;
                element = key.substr(ROUTE_KEY_PREFIX.length());
                mapRoutes.emplace(element,value);
                continue;
            }
        }
    }

    jaliases.clear();
    if(!mapInterfaces.empty())
    {
        for (const auto& [ifname, ifconfig] : mapInterfaces)
        {
            jdata.clear();
            jif.clear();
            if(pos1=ifname.find_last_of("_"); pos1!=std::string::npos)
            {
                if(pos2=ifname.find(ALIAS_SUFFIX); pos2!=std::string::npos)
                // ifconfig_ed0_aliases="inet 127.0.0.251 netmask 0xffffffff inet 127.0.0.252 netmask 0xffffffff inet 127.0.0.253 netmask 0xffffffff inet 127.0.0.254 netmask 0xffffffff"
                {
                    name=ifname.substr(0, pos1);
                    if(jaliases.contains(name))
                        // Only first aliases record will be processed
                        continue;
                    jaliases[name] = json::array();
                    element = ifconfig;
                    Trim(element, quotes);
                    pos0 = 0;
                    while(true)
                    {
                        pos1 = element.find(INET_ADDR, pos0);
                        if(pos1==std::string::npos)
                            break;
                        pos2 = element.find(INET_ADDR,pos1+INET_ADDR.length());
                        if(pos2==std::string::npos)
                            pos2 = element.length();
                        jdata = getIpConfFromString(element.substr(pos1,pos2-pos1));
                        if(!jdata.empty())
                        {
                            jaliases[name].push_back(jdata);
                        }
                        pos0 = pos2;
                    }
                    continue;
                }
                else if(element=ifname.substr(pos1+1,std::string::npos); strtol(element.c_str(),nullptr,10)>0)
                // ifconfig_em0_101="inet 192.0.2.1/24"
                {
                    element = ifname;
                    element.replace(pos1, 1, point);
                    jif[JSON_PARAM_IF_NAME] = element;
                }
                else
                {
                    LOG_S(INFO) << "getRcIpConfig: interface " << ifname << " is not supported (yet)";
                    continue;
                }
            }
            else
            {
                jif[JSON_PARAM_IF_NAME] = ifname;
            }

            jdata = getIpConfFromString(ifconfig);
            if(!jdata.empty())
            {
                jif[JSON_PARAM_ADDRESSES].push_back(jdata);
                jarInterfaces.push_back(jif);
            }
        }
    }

    if(!mapRoutes.empty())
    {
        for (const auto& [rtname, rtconfig] : mapRoutes)
        {
            jrt[JSON_PARAM_RT_NAME] = rtname;
            jdata = getRouteConfFromString(rtconfig);
            if(!jdata.empty())
            {
                jrt[JSON_PARAM_DATA] = jdata;
                jarRoutes.push_back(jrt);
            }
        }
    }
// Iterate interfaces to insert default route and aliases
    paddr4 = nullptr; pipgw4 = nullptr; pipmask4 = nullptr; pipaddr4 = nullptr;
    for(auto& j : jarInterfaces)
    {
        if(j[JSON_PARAM_ADDRESSES][0][JSON_PARAM_IPV4_ADDR].get<std::string>().find(DHCP_SUFFIX)!=std::string::npos)
            continue;
        name = j[JSON_PARAM_IF_NAME].get<std::string>();
        if(jaliases.contains(name))
        {
            for(auto& jj : jaliases[name])
            {
                j[JSON_PARAM_ADDRESSES].push_back(jj);
            }
        }
        try {
            pipaddr4 = new address_ip4(j[JSON_PARAM_ADDRESSES][0][JSON_PARAM_IPV4_ADDR].get<std::string>());
            pipmask4 = new address_ip4(j[JSON_PARAM_ADDRESSES][0][JSON_PARAM_IPV4_MASK].get<std::string>());
            pipgw4 = new address_ip4(strDefaultRouter);
            paddr4 = new addr(pipaddr4, pipmask4, pipgw4, ipaddr_type::PPP);
            if(paddr4->isValidIp())
            {
                LOG_S(INFO) << "Validated IP4 default router : " << j[JSON_PARAM_ADDRESSES][0][JSON_PARAM_IPV4_ADDR].get<std::string>() << " / " << j[JSON_PARAM_ADDRESSES][0][JSON_PARAM_IPV4_MASK].get<std::string>() << " " << strDefaultRouter;
                j[JSON_PARAM_ADDRESSES][0][JSON_PARAM_IPV4_GW] = strDefaultRouter;
            }
            delete paddr4; paddr4 = nullptr;
            delete pipgw4; pipgw4 = nullptr;
            delete pipmask4; pipmask4 = nullptr;
            delete pipaddr4; pipaddr4 = nullptr;
        } catch (std::exception& e) {
            LOG_S(WARNING) << "getRcIpConfig cannot validate IP configuration";
            if(paddr4 != nullptr)
            {
                delete paddr4; paddr4 = nullptr;
            }
            if(pipgw4 != nullptr)
            {
                delete pipgw4; pipgw4 = nullptr;
            }
            if(pipmask4 != nullptr)
            {
                delete pipmask4; pipmask4 = nullptr;
            }
            if(pipaddr4 != nullptr)
            {
                delete pipaddr4; pipaddr4 = nullptr;
            }
//            return {};  // TODO: Do we really need to return nothing here?
        }
    }
// Iterate routes to remove inactive ones and validate the active ones
    for(auto &jit : jarRoutes.items())
    {
        if(setActiveRoutes.contains(jit.value().at(JSON_PARAM_RT_NAME).get<std::string>()))
        {
            jdata = jit.value().at(JSON_PARAM_DATA);
            try {
                pipaddr4 = new address_ip4(jdata[JSON_PARAM_IPV4_ADDR].get<std::string>());
                pipmask4 = new address_ip4(jdata[JSON_PARAM_IPV4_MASK].get<std::string>());
                pipgw4 = new address_ip4(jdata[JSON_PARAM_IPV4_GW].get<std::string>());
                LOG_S(INFO) << "Active validated IP4 route " << jit.value().at(JSON_PARAM_RT_NAME).get<std::string>() << ": " << jdata[JSON_PARAM_IPV4_ADDR].get<std::string>() << " / " << jdata[JSON_PARAM_IPV4_MASK].get<std::string>() << " " << jdata[JSON_PARAM_IPV4_GW].get<std::string>();
                jit.value().emplace(JSON_PARAM_STATUS,JSON_DATA_ENABLED);
                delete paddr4; paddr4 = nullptr;
                delete pipgw4; pipgw4 = nullptr;
                delete pipmask4; pipmask4 = nullptr;
                delete pipaddr4; pipaddr4 = nullptr;
            } catch (std::exception& e) {
                LOG_S(WARNING) << "getRcIpConfig cannot validate route configuration for route " << jit.value().at(JSON_PARAM_RT_NAME).get<std::string>();
                if(paddr4 != nullptr)
                {
                    delete paddr4; paddr4 = nullptr;
                }
                if(pipgw4 != nullptr)
                {
                    delete pipgw4; pipgw4 = nullptr;
                }
                if(pipmask4 != nullptr)
                {
                    delete pipmask4; pipmask4 = nullptr;
                }
    //            return {};  // TODO: Do we really need to return nothing here?
            }
        }
        else
        {
            LOG_S(INFO) << "getRcIpConfig: inactive route " << jit.value().at(JSON_PARAM_RT_NAME).get<std::string>();
            jit.value().emplace(JSON_PARAM_STATUS,JSON_DATA_DISABLED);
//            vectInactiveRoutes.push_back(stoi(jit.key()));
        }
    }
//    std::sort(vectInactiveRoutes.rbegin(), vectInactiveRoutes.rend());
    /*
    for (int &iit : vectInactiveRoutes)
    {
        LOG_S(INFO) << "getRcIpConfig: trying to remove route number " << iit << " from routes list";
        jarRoutes.erase(jarRoutes.begin() + iit);
    }
    */
    jret[JSON_PARAM_RESULT] = JSON_PARAM_SUCC;
    jret[JSON_PARAM_INTERFACES] = jarInterfaces;
    jret[JSON_PARAM_ROUTES] = jarRoutes;
    return jret;
}

json rcconf::getIpConfFromString(std::string ifconfig)
{
    unsigned int arrIntIp4Conf[12];
    unsigned int imask;
    struct in_addr ip_mask;
    std::string element;
    const std::string quotes = "\"";
    const std::string space = " ";
    json jdata = {};
    std::string full_mask = "255.255.255.255";
    std::string if_format_1 = "inet %3d.%3d.%3d.%3d/%2d";   // inet 127.0.0.251/24
    std::string if_format_2 = "inet %3d.%3d.%3d.%3d netmask %10x";   // inet 127.0.0.251 netmask 0xffffff00
    std::string if_format_3 = "inet %3d.%3d.%3d.%3d netmask %3d.%3d.%3d.%3d";   // inet 127.0.0.251 netmask 255.255.255.0

    Trim(ifconfig, quotes);
    Trim(ifconfig, space);

    if(ifconfig.find(DHCP_SUFFIX)!=std::string::npos)
    {
        jdata[JSON_PARAM_IPV4_ADDR] = ifconfig;
    }
    else if( memset(arrIntIp4Conf, 0, 12*sizeof(int));
             (sscanf(ifconfig.c_str(), if_format_1.c_str(),
                &arrIntIp4Conf[0], &arrIntIp4Conf[1], &arrIntIp4Conf[2], &arrIntIp4Conf[3], &imask)==5) &&
                    (imask>0) && (imask<=32) )
    // inet 127.0.0.251/24
    {
        jdata[JSON_PARAM_IPV4_MASK] = getStrInetMaskFromPrefix(imask);
        jdata[JSON_PARAM_IPV4_ADDR] = std::to_string(arrIntIp4Conf[0]) + "." +
                                      std::to_string(arrIntIp4Conf[1]) + "." +
                                      std::to_string(arrIntIp4Conf[2]) + "." +
                                      std::to_string(arrIntIp4Conf[3]);
    }
    else if( memset(arrIntIp4Conf, 0, 12*sizeof(int));
             (sscanf(ifconfig.c_str(), if_format_3.c_str(),
                     &arrIntIp4Conf[0], &arrIntIp4Conf[1], &arrIntIp4Conf[2], &arrIntIp4Conf[3],
                     &arrIntIp4Conf[4], &arrIntIp4Conf[5], &arrIntIp4Conf[6], &arrIntIp4Conf[7])==8) )
    // "inet 127.0.0.251 netmask 255.255.255.0"
    {
        jdata[JSON_PARAM_IPV4_ADDR] = std::to_string(arrIntIp4Conf[0]) + "." +
                                      std::to_string(arrIntIp4Conf[1]) + "." +
                                      std::to_string(arrIntIp4Conf[2]) + "." +
                                      std::to_string(arrIntIp4Conf[3]);
        jdata[JSON_PARAM_IPV4_MASK] = std::to_string(arrIntIp4Conf[4]) + "." +
                                      std::to_string(arrIntIp4Conf[5]) + "." +
                                      std::to_string(arrIntIp4Conf[6]) + "." +
                                      std::to_string(arrIntIp4Conf[7]);
    }
    else if( memset(arrIntIp4Conf, 0, 12*sizeof(int));
             (sscanf(ifconfig.c_str(), if_format_2.c_str(),
                     &arrIntIp4Conf[0], &arrIntIp4Conf[1], &arrIntIp4Conf[2], &arrIntIp4Conf[3], &imask)==5) )
    // "inet 127.0.0.251 netmask 0xffffff00"
    {
        imask = htonl(imask);
        ip_mask.s_addr = imask;
        element = std::string(inet_ntoa(ip_mask));
        jdata[JSON_PARAM_IPV4_MASK] = element;
        jdata[JSON_PARAM_IPV4_ADDR] = std::to_string(arrIntIp4Conf[0]) + "." +
                                      std::to_string(arrIntIp4Conf[1]) + "." +
                                      std::to_string(arrIntIp4Conf[2]) + "." +
                                      std::to_string(arrIntIp4Conf[3]);
    }
    return jdata;
}

json rcconf::getRouteConfFromString(std::string rtconfig)
{
    unsigned int arrIntIp4Conf[12];
    const std::string quotes = "\"";
    const std::string space = " ";
    unsigned int imask;
    struct in_addr ip_mask;
    std::string element;
    std::string full_mask = "255.255.255.255";
    std::string rt_format_net_1 = "-net %3d.%3d.%3d.%3d/%2d %3d.%3d.%3d.%3d"; // -net 192.168.32.0/24 192.168.213.252
    std::string rt_format_net_2 = "-net %3d.%3d.%3d.%3d -netmask %10x %3d.%3d.%3d.%3d"; // -net 192.168.32.0 -netmask 0xffffff00 192.168.213.252
    std::string rt_format_net_3 = "-net %3d.%3d.%3d.%3d -netmask %3d.%3d.%3d.%3d %3d.%3d.%3d.%3d"; // -net 192.168.32.0 -netmask 255.255.255.0 192.168.213.252
    std::string rt_format_host_1 = "-host %3d.%3d.%3d.%3d %3d.%3d.%3d.%3d"; // -host 191.1.200.4 192.168.213.254
    json jdata = {};

    Trim(rtconfig, quotes);
    Trim(rtconfig, space);

    if( memset(arrIntIp4Conf, 0, 12*sizeof(int));
        (sscanf(rtconfig.c_str(), rt_format_net_1.c_str(),
                &arrIntIp4Conf[0], &arrIntIp4Conf[1], &arrIntIp4Conf[2], &arrIntIp4Conf[3], &imask,
                &arrIntIp4Conf[4], &arrIntIp4Conf[5], &arrIntIp4Conf[6], &arrIntIp4Conf[7])==9) &&
                    (imask>0) && (imask<=32) )
    // -net 192.168.32.0/24 192.168.213.252
    {
        jdata[JSON_PARAM_IPV4_MASK] = getStrInetMaskFromPrefix(imask);
        jdata[JSON_PARAM_IPV4_ADDR] = std::to_string(arrIntIp4Conf[0]) + "." +
                                      std::to_string(arrIntIp4Conf[1]) + "." +
                                      std::to_string(arrIntIp4Conf[2]) + "." +
                                      std::to_string(arrIntIp4Conf[3]);
        jdata[JSON_PARAM_IPV4_GW]   = std::to_string(arrIntIp4Conf[4]) + "." +
                                      std::to_string(arrIntIp4Conf[5]) + "." +
                                      std::to_string(arrIntIp4Conf[6]) + "." +
                                      std::to_string(arrIntIp4Conf[7]);
    }
    else if( memset(arrIntIp4Conf, 0, 12*sizeof(int));
             (sscanf(rtconfig.c_str(), rt_format_net_3.c_str(),
                     &arrIntIp4Conf[0], &arrIntIp4Conf[1], &arrIntIp4Conf[2], &arrIntIp4Conf[3],
                     &arrIntIp4Conf[4], &arrIntIp4Conf[5], &arrIntIp4Conf[6], &arrIntIp4Conf[7],
                     &arrIntIp4Conf[8], &arrIntIp4Conf[9], &arrIntIp4Conf[10], &arrIntIp4Conf[11])==12) )
    // -net 192.168.32.0 -netmask 255.255.255.0 192.168.213.252
    {
        jdata[JSON_PARAM_IPV4_ADDR] = std::to_string(arrIntIp4Conf[0]) + "." +
                                      std::to_string(arrIntIp4Conf[1]) + "." +
                                      std::to_string(arrIntIp4Conf[2]) + "." +
                                      std::to_string(arrIntIp4Conf[3]);
        jdata[JSON_PARAM_IPV4_MASK] = std::to_string(arrIntIp4Conf[4]) + "." +
                                      std::to_string(arrIntIp4Conf[5]) + "." +
                                      std::to_string(arrIntIp4Conf[6]) + "." +
                                      std::to_string(arrIntIp4Conf[7]);
        jdata[JSON_PARAM_IPV4_GW]   = std::to_string(arrIntIp4Conf[8]) + "." +
                                      std::to_string(arrIntIp4Conf[9]) + "." +
                                      std::to_string(arrIntIp4Conf[10]) + "." +
                                      std::to_string(arrIntIp4Conf[11]);
    }
    else if( memset(arrIntIp4Conf, 0, 12*sizeof(int));
             (sscanf(rtconfig.c_str(), rt_format_net_2.c_str(),
                     &arrIntIp4Conf[0], &arrIntIp4Conf[1], &arrIntIp4Conf[2], &arrIntIp4Conf[3], &imask,
                     &arrIntIp4Conf[4], &arrIntIp4Conf[5], &arrIntIp4Conf[6], &arrIntIp4Conf[7])==9) )
    // -net 192.168.32.0 -netmask 0xffffff00 192.168.213.252
    {
        imask = htonl(imask);
        ip_mask.s_addr = imask;
        element = std::string(inet_ntoa(ip_mask));
        jdata[JSON_PARAM_IPV4_MASK] = element;
        jdata[JSON_PARAM_IPV4_ADDR] = std::to_string(arrIntIp4Conf[0]) + "." +
                                      std::to_string(arrIntIp4Conf[1]) + "." +
                                      std::to_string(arrIntIp4Conf[2]) + "." +
                                      std::to_string(arrIntIp4Conf[3]);
        jdata[JSON_PARAM_IPV4_GW]   = std::to_string(arrIntIp4Conf[4]) + "." +
                                      std::to_string(arrIntIp4Conf[5]) + "." +
                                      std::to_string(arrIntIp4Conf[6]) + "." +
                                      std::to_string(arrIntIp4Conf[7]);
    }
    else if( memset(arrIntIp4Conf, 0, 12*sizeof(int));
             (sscanf(rtconfig.c_str(), rt_format_host_1.c_str(),
                     &arrIntIp4Conf[0], &arrIntIp4Conf[1], &arrIntIp4Conf[2], &arrIntIp4Conf[3],
                     &arrIntIp4Conf[4], &arrIntIp4Conf[5], &arrIntIp4Conf[6], &arrIntIp4Conf[7])==8) )
    // -host 191.1.200.4 192.168.213.254
    {
        jdata[JSON_PARAM_IPV4_ADDR] = std::to_string(arrIntIp4Conf[0]) + "." +
                                      std::to_string(arrIntIp4Conf[1]) + "." +
                                      std::to_string(arrIntIp4Conf[2]) + "." +
                                      std::to_string(arrIntIp4Conf[3]);
        jdata[JSON_PARAM_IPV4_MASK] = full_mask;
        jdata[JSON_PARAM_IPV4_GW]   = std::to_string(arrIntIp4Conf[4]) + "." +
                                      std::to_string(arrIntIp4Conf[5]) + "." +
                                      std::to_string(arrIntIp4Conf[6]) + "." +
                                      std::to_string(arrIntIp4Conf[7]);
    }
    return jdata;
}

std::string rcconf::getStrInetMaskFromPrefix(int prefix)
{
    unsigned long lmask = (0xFFFFFFFF << (32 - prefix)) & 0xFFFFFFFF;
    return std::to_string(lmask>>24) + "." +
              std::to_string((lmask>>16)&0xFF) + "." +
              std::to_string((lmask>>8)&0xFF) + "." +
              std::to_string(lmask&0xFF);
}

bool rcconf::setRcIpConfig(json*)
{
    return true;
}
