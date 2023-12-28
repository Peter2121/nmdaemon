#include "rcconf.h"

RcConf::RcConf(std::string path, short nbkp) : rcFileName(path), nBackups(nbkp)
{
    rcIniFile = new CIniFile();
}

RcConf::~RcConf()
{
    delete rcIniFile;
}

bool RcConf::iniLoad()
{
    return rcIniFile->Load(rcFileName);
}

bool RcConf::iniSave()
{
    return rcIniFile->Save(rcFileName);
}

// TODO: Decode ipv6 configuration
// TODO: Consider scnlib https://scnlib.dev/ to simplify the decoding
json RcConf::getRcIpConfig()
{
    std::string strDefaultRouter = "";
    std::string key = "";
    std::string_view key_view;
    std::string value = "";
    std::string element;
    std::string name;
    std::string alias_name;
    std::string_view element_view;
    std::stringstream ss;
    std::shared_ptr<AddressIp4> spipaddr4=nullptr;
    std::shared_ptr<AddressIp4> spipmask4=nullptr;
    std::shared_ptr<AddressIp4> spipgw4=nullptr;
    std::unique_ptr<AddressGroup> upaddr4=nullptr;
    const char DELIM = ' ';
    const std::string quotes = "\"";
    const std::string point = ".";
    size_t pos0,pos1,pos2;
    json jdata {};
    json jif;
    json jrt;
    json jaliases;
    json jarInterfaces = json::array();
    json jarRoutes = json::array();
    json jret {};
    std::map<std::string, std::string> mapInterfaces;
    std::map<std::string, std::string> mapRoutes;
    std::set<std::string> setActiveRoutes;
    bool is_primary = false;
    bool aliases_processed = false;

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
            is_primary = false;
            if(pos1=ifname.find_last_of("_"); pos1!=std::string::npos)
            {
                if(pos2=ifname.find(ALIASES_SUFFIX); pos2!=std::string::npos)
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
                    aliases_processed = true;
                    continue;
                }
                else if(pos2=ifname.find(ALIAS_SUFFIX); pos2!=std::string::npos)
                //          ifname     ifconfig
                // ifconfig_em0_alias0="inet 192.168.212.102 netmask 255.255.255.0"
                // ifconfig_em0_alias1="inet 192.168.212.202 netmask 0xffffff00"
                // ifconfig_em0_alias1="inet 192.168.212.202/24"
                // TODO: check that the aliases numbers start from 0 (if not - they are ignored by system)
                {
                    if(aliases_processed)
                        // ALIASES_SUFFIX has priority over ALIAS_SUFFIX
                        continue;
                    name=ifname.substr(0, pos1);
                    alias_name=ifname.substr(pos1+1, std::string::npos);
                    jdata = getIpConfFromString(ifconfig);
                    if(!jdata.empty())
                    {
                        jaliases[name].push_back(jdata);
                        //LOG_S(INFO) << "getRcIpConfig got IP " << alias_name << " alias data for interface " << name << ":" << std::endl << jdata.dump();
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
                is_primary = true;
            }

            jdata = getIpConfFromString(ifconfig);
            if(!jdata.empty())
            {
                if(is_primary)
                    jdata[JSON_PARAM_ADDR_PRIMARY] = is_primary;
                jif[JSON_PARAM_ADDRESSES].push_back(jdata);
                jarInterfaces.push_back(jif);
            }
        }
        jdata.clear();
    }

    if(!mapRoutes.empty())
    {
        for (const auto& [rtname, rtconfig] : mapRoutes)
        {
            jrt[JSON_PARAM_RT_NAME] = rtname;
            jdata = getRouteConfFromString(rtconfig);
            if(!jdata.empty())
            {
                jrt[JSON_PARAM_ADDRESSES] = jdata;
                jarRoutes.push_back(jrt);
            }
        }
        jdata.clear();
    }
// Iterate interfaces to insert default route and aliases
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
        try
        {
            spipaddr4 = std::make_shared<AddressIp4>(j[JSON_PARAM_ADDRESSES][0][JSON_PARAM_IPV4_ADDR].get<std::string>());
            spipmask4 = std::make_shared<AddressIp4>(j[JSON_PARAM_ADDRESSES][0][JSON_PARAM_IPV4_MASK].get<std::string>());
            spipgw4 = std::make_shared<AddressIp4>(strDefaultRouter);
            upaddr4 = std::make_unique<AddressGroup>(spipaddr4, spipmask4, spipgw4, AddressGroupType::PPP);
            if(upaddr4->isValidIp())
            {
                LOG_S(INFO) << "Validated IP4 default router : " << j[JSON_PARAM_ADDRESSES][0][JSON_PARAM_IPV4_ADDR].get<std::string>() << " / " << j[JSON_PARAM_ADDRESSES][0][JSON_PARAM_IPV4_MASK].get<std::string>() << " " << strDefaultRouter;
                j[JSON_PARAM_ADDRESSES][0][JSON_PARAM_IPV4_GW] = strDefaultRouter;
            }
        } catch (std::exception& e) {
            LOG_S(WARNING) << "getRcIpConfig cannot validate IP configuration";
        }
    }
// Iterate routes to remove inactive ones and validate the active ones
    for(auto &jit : jarRoutes.items())
    {
        if(setActiveRoutes.contains(jit.value().at(JSON_PARAM_RT_NAME).get<std::string>()))
        {
            jdata = jit.value().at(JSON_PARAM_ADDRESSES);
            try {
                spipaddr4 = std::make_shared<AddressIp4>(jdata[JSON_PARAM_IPV4_ADDR].get<std::string>());
                spipmask4 = std::make_shared<AddressIp4>(jdata[JSON_PARAM_IPV4_MASK].get<std::string>());
                spipgw4 = std::make_shared<AddressIp4>(jdata[JSON_PARAM_IPV4_GW].get<std::string>());
                LOG_S(INFO) << "Active validated IP4 route " << jit.value().at(JSON_PARAM_RT_NAME).get<std::string>() << ": " << jdata[JSON_PARAM_IPV4_ADDR].get<std::string>() << " / " << jdata[JSON_PARAM_IPV4_MASK].get<std::string>() << " " << jdata[JSON_PARAM_IPV4_GW].get<std::string>();
                jit.value().emplace(JSON_PARAM_STATUS,JSON_DATA_ENABLED);
            } catch (std::exception& e) {
                LOG_S(WARNING) << "getRcIpConfig cannot validate route configuration for route " << jit.value().at(JSON_PARAM_RT_NAME).get<std::string>();
            }
        }
        else
        {
            LOG_S(INFO) << "getRcIpConfig: inactive route " << jit.value().at(JSON_PARAM_RT_NAME).get<std::string>();
            jit.value().emplace(JSON_PARAM_STATUS,JSON_DATA_DISABLED);
        }
    }
    jdata.clear();
    jret[JSON_PARAM_RESULT] = JSON_PARAM_SUCC;
    jdata[JSON_PARAM_INTERFACES] = jarInterfaces;
    jdata[JSON_PARAM_ROUTES] = jarRoutes;
    jret[JSON_PARAM_DATA] = jdata;
    return jret;
}

json RcConf::getIpConfFromString(std::string ifconfig)
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

json RcConf::getRouteConfFromString(std::string rtconfig)
{
    unsigned int arrIntIp4Conf[12];
    const std::string quotes = "\"";
    const std::string space = " ";
    unsigned int imask;
    struct in_addr ip_mask;
    std::string element;
//    std::string full_mask = "255.255.255.255";
    std::string rt_format_net_1 = ROUTE_PREFIX_NET + " %3d.%3d.%3d.%3d/%2d %3d.%3d.%3d.%3d"; // -net 192.168.32.0/24 192.168.213.252
    std::string rt_format_net_2 = ROUTE_PREFIX_NET + " %3d.%3d.%3d.%3d -netmask %10x %3d.%3d.%3d.%3d"; // -net 192.168.32.0 -netmask 0xffffff00 192.168.213.252
    std::string rt_format_net_3 = ROUTE_PREFIX_NET + " %3d.%3d.%3d.%3d -netmask %3d.%3d.%3d.%3d %3d.%3d.%3d.%3d"; // -net 192.168.32.0 -netmask 255.255.255.0 192.168.213.252
    std::string rt_format_host_1 = ROUTE_PREFIX_HOST + " %3d.%3d.%3d.%3d %3d.%3d.%3d.%3d"; // -host 191.1.200.4 192.168.213.254
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
        jdata[JSON_PARAM_IPV4_MASK] = IPV4_MASK_HOST;
        jdata[JSON_PARAM_IPV4_GW]   = std::to_string(arrIntIp4Conf[4]) + "." +
                                      std::to_string(arrIntIp4Conf[5]) + "." +
                                      std::to_string(arrIntIp4Conf[6]) + "." +
                                      std::to_string(arrIntIp4Conf[7]);
    }
    return jdata;
}

std::string RcConf::getStrInetMaskFromPrefix(int prefix)
{
    unsigned long lmask = (0xFFFFFFFF << (32 - prefix)) & 0xFFFFFFFF;
    return std::to_string(lmask>>24) + "." +
              std::to_string((lmask>>16)&0xFF) + "." +
              std::to_string((lmask>>8)&0xFF) + "." +
              std::to_string(lmask&0xFF);
}

bool RcConf::rotateRcConfFile()
{
    /*
    std::string rc_file_name1;
    std::string rc_file_name2;
    bool error = false;
    if(nBackups<=0)
        return true;
    rc_file_name1 = rcFileName + FILE_VERSIONS_DELIMITER + std::to_string(nBackups);
    try
    {
        if(std::filesystem::exists(rc_file_name1))
        {
            LOG_S(INFO) << "Removing " << rc_file_name1 << " during rcconf file rotation";
            std::filesystem::remove(rc_file_name1);
        }

    }
    catch(std::exception& e)
    {
        LOG_S(WARNING) << "Exception in rotateRcConfFile: " << e.what();
    }
    for(short i=nBackups; i>1; i--)
    {
        rc_file_name1 = rcFileName + FILE_VERSIONS_DELIMITER + std::to_string(i-1);
        rc_file_name2 = rcFileName + FILE_VERSIONS_DELIMITER + std::to_string(i);
        try
        {
            if(std::filesystem::exists(rc_file_name1))
            {
                LOG_S(INFO) << "Moving " << rc_file_name1 << " to " << rc_file_name2 << " during rcconf file rotation";
                std::filesystem::copy(rc_file_name1, rc_file_name2);
                std::filesystem::remove(rc_file_name1);
            }
        }
        catch(std::exception& e)
        {
            LOG_S(WARNING) << "Exception in rotateRcConfFile: " << e.what();
            error = true;
        }
    }
    try
    {
        LOG_S(INFO) << "Copy " << rcFileName << " to " << rc_file_name2 << " during rcconf file rotation";
        std::filesystem::copy(rcFileName, rc_file_name1);
    }
    catch (std::exception& e)
    {
        LOG_S(WARNING) << "Exception in rotateRcConfFile: " << e.what();
        error = true;
    }
    return !error;
    */
    return Tool::rotateConfigFile(rcFileName, nBackups, FILE_VERSIONS_DELIMITER);
}

bool RcConf::setRcIpConfig(json rcdata)
{
    json jar_interfaces = json::array();
    json jar_addresses = json::array();
    json jar_routes = json::array();
    bool have_interfaces = false;
    bool have_routes = false;
    bool is_addr_primary = false;
    bool is_route_enabled = false;
    bool is_route_to_host = false;
    std::string str_conf_key;
    std::string str_conf_value;
    std::string str_old_conf_value;
    std::string str_if_name;
    std::string str_rt_name;
    std::string ip_addr;
    std::string ip_mask;
    std::string ip_gw;
    std::string str_rt_status;
    std::string str_active_routes;
    size_t pos1;
    std::string element;

    if(!iniLoad())
        return JSON_RESULT_ERR;

    try
    {
        jar_interfaces = rcdata[JSON_PARAM_INTERFACES];
        jar_routes = rcdata[JSON_PARAM_ROUTES];
    }
    catch(std::exception& e)
    {
        LOG_S(WARNING) << "setRcIpConfig: exception reading json received: " << e.what();
    }

    if(!jar_interfaces.empty())
        have_interfaces = true;
    if(!jar_routes.empty())
        have_routes = true;
    if(!have_interfaces && !have_routes)
    {
        LOG_S(ERROR) << "setRcIpConfig: no usable data in json received";
        return false;
    }

    for(const auto &jif : jar_interfaces)
    {
        try
        {
            str_if_name = jif.at(JSON_PARAM_IF_NAME);
            if(str_if_name.empty())
            {
                LOG_S(WARNING) << "setRcIpConfig cannot get interface name";
                continue;
            }
//          Fix VLAN interface name
            if(pos1=str_if_name.find_last_of("."); pos1!=std::string::npos)
            {
                if(element=str_if_name.substr(pos1+1,std::string::npos); strtol(element.c_str(),nullptr,10)>0)
                    str_if_name.replace(pos1, 1, "_");
            }
            jar_addresses = jif.at(JSON_PARAM_ADDRESSES);
            if(jar_addresses.empty())
            {
                LOG_S(WARNING) << "setRcIpConfig cannot get addresses for interface " << str_if_name;
                continue;
            }
        }
        catch (std::exception& e)
        {
            LOG_S(WARNING) << "Exception in setRcIpConfig trying to access addresses of interface " << str_if_name;
            LOG_S(WARNING) << e.what();
            continue;
        }
//  First cycle - put principal address and default route
        for(const auto &jad : jar_addresses)
        {
            is_addr_primary = false;
            try
            {
                is_addr_primary = jad.at(JSON_PARAM_ADDR_PRIMARY);
            }
            catch(std::exception&)
            {
                continue;
            }
            try
            {
                if(is_addr_primary)
                {
//  TODO: deal with IPv6 addresses
//  TODO: implement basic data check
                    ip_addr = jad.at(JSON_PARAM_IPV4_ADDR);
                    ip_mask = "";
                    if(ip_addr.find(DHCP_SUFFIX) == std::string::npos)
                    {
                        ip_mask = jad.at(JSON_PARAM_IPV4_MASK);
                    }
                    str_conf_key = IFCONFIG_KEY_PREFIX + str_if_name;
                    if(ip_mask.empty())
                        str_conf_value = "\"" + ip_addr + "\"";
                    else
                        str_conf_value = "\"" + INET_ADDR + " " + ip_addr + " "
                                              + INET_MASK + " " + ip_mask + "\"";
                    str_old_conf_value = rcIniFile->GetKeyValue(DEFAULT_SECTION, str_conf_key);
                    if(str_conf_value != str_old_conf_value)
                        rcIniFile->SetKeyValue(DEFAULT_SECTION, str_conf_key, str_conf_value);
                }
            }
            catch(std::exception&)
            {
                LOG_S(WARNING) << "setRcIpConfig cannot decode data: " << jad.dump() << " for interface " << str_if_name;
                continue;
            }
            try
            {
                if(is_addr_primary)
                {
                    str_conf_key = DEFAULT_ROUTE_KEY;
                    ip_gw = jad.at(JSON_PARAM_IPV4_GW);
                    str_conf_value = "\"" + ip_gw + "\"";
                    str_old_conf_value = rcIniFile->GetKeyValue(DEFAULT_SECTION, str_conf_key);
                    if(str_conf_value != str_old_conf_value)
                        rcIniFile->SetKeyValue(DEFAULT_SECTION_A, str_conf_key, str_conf_value);
                }
            }
            catch(std::exception&)
            {
            }
        }
//  Second cycle - put aliases
//  TODO: integrate the second cycle into the first one
        str_conf_key = IFCONFIG_KEY_PREFIX + str_if_name + "_" + ALIASES_SUFFIX;
        str_conf_value = "";
        for(const auto &jad : jar_addresses)
        {
            is_addr_primary = false;
            try
            {
                is_addr_primary = jad.at(JSON_PARAM_ADDR_PRIMARY);
            }
            catch(std::exception&)
            {
            }
            try
            {
                if(!is_addr_primary)
                {
//  TODO: deal with IPv6 addresses
//  TODO: implement basic data check
                    ip_addr = jad.at(JSON_PARAM_IPV4_ADDR);
                    ip_mask = jad.at(JSON_PARAM_IPV4_MASK);
                    if(!str_conf_value.empty())
                        str_conf_value += " ";
                    str_conf_value += INET_ADDR + " " + ip_addr + " "
                                    + INET_MASK + " " + ip_mask;
                }
            }
            catch(std::exception&)
            {
                LOG_S(WARNING) << "setRcIpConfig cannot decode data: " << jad.dump() << " for interface " << str_if_name;
                continue;
            }
        }
        if(!str_conf_value.empty())
        {
            str_conf_value = "\"" + str_conf_value + "\"";
            str_old_conf_value = rcIniFile->GetKeyValue(DEFAULT_SECTION, str_conf_key);
            if(str_conf_value != str_old_conf_value)
                rcIniFile->SetKeyValue(DEFAULT_SECTION, str_conf_key, str_conf_value);
        }
        for(int i=0; i<MAX_ALIASES; i++)
        // Remove old records like ifconfig_em0_alias0="..." from config file
        {
            str_conf_key = IFCONFIG_KEY_PREFIX + str_if_name + "_" + ALIAS_SUFFIX + std::to_string(i);
            rcIniFile->GetSection(DEFAULT_SECTION)->RemoveKey(str_conf_key);
            //auto section = rcIniFile->GetSection(DEFAULT_SECTION);
            //section->RemoveKey(str_conf_key);
        }
    }

//  Put routes
    jar_addresses.clear();
    str_active_routes = "";
    for(const auto &jrt : jar_routes)
    {
        try
        {
            str_rt_name = jrt.at(JSON_PARAM_RT_NAME);
            if(str_rt_name.empty())
            {
                LOG_S(WARNING) << "setRcIpConfig cannot get route name";
                continue;
            }
            str_rt_status = jrt.at(JSON_PARAM_STATUS);
            if(str_rt_status.empty())
            {
                LOG_S(WARNING) << "setRcIpConfig cannot get status for route " << str_rt_name;
                continue;
            }
            is_route_enabled = str_rt_status==JSON_DATA_ENABLED;
            jar_addresses = jrt.at(JSON_PARAM_ADDRESSES);
            if(jar_addresses.empty())
            {
                LOG_S(WARNING) << "setRcIpConfig cannot get addresses for route " << str_rt_name;
                continue;
            }
        }
        catch (std::exception& e)
        {
            LOG_S(WARNING) << "Exception in setRcIpConfig trying to access addresses of route " << str_rt_name;
            LOG_S(WARNING) << e.what();
            continue;
        }

        str_conf_value = "";
        try
        {
//  TODO: deal with IPv6 addresses
//  TODO: implement basic data check
            ip_addr = jar_addresses.at(JSON_PARAM_IPV4_ADDR);
            ip_mask = jar_addresses.at(JSON_PARAM_IPV4_MASK);
            ip_gw = jar_addresses.at(JSON_PARAM_IPV4_GW);
            if( ip_addr.empty() || ip_mask.empty() || ip_gw.empty() )
            {
                LOG_S(WARNING) << "setRcIpConfig received invalid addresses for route " << str_rt_name;
                continue;
            }
            if(is_route_enabled)
            {
                if(!str_active_routes.empty())
                    str_active_routes += " ";
                str_active_routes += str_rt_name;
            }
            is_route_to_host = ip_mask==IPV4_MASK_HOST;
            str_conf_key = ROUTE_KEY_PREFIX + str_rt_name;
            if(is_route_to_host)
            {
                str_conf_value = "\"" + ROUTE_PREFIX_HOST + " " + ip_addr + " " + ip_gw + "\"";
            }
            else
            {
                str_conf_value = "\"" + ROUTE_PREFIX_NET + " " + ip_addr + " -" + INET_MASK + " " + ip_mask + " " + ip_gw + "\"";
            }
            str_old_conf_value = rcIniFile->GetKeyValue(DEFAULT_SECTION, str_conf_key);
            if(str_conf_value != str_old_conf_value)
                rcIniFile->SetKeyValue(DEFAULT_SECTION, str_conf_key, str_conf_value);
        }
        catch(std::exception&)
        {
            LOG_S(WARNING) << "setRcIpConfig cannot decode data: " << jar_addresses.dump() << " for route " << str_rt_name;
            continue;
        }
    }

    // TODO: remove old routes!!!

    if(!str_active_routes.empty())
    {
        str_conf_key = ROUTES_KEY;
        str_conf_value = "\"" + str_active_routes + "\"";
        str_old_conf_value = rcIniFile->GetKeyValue(DEFAULT_SECTION, str_conf_key);
        if(str_conf_value != str_old_conf_value)
            rcIniFile->SetKeyValue(DEFAULT_SECTION, str_conf_key, str_conf_value);
    }

    return iniSave();
}

