#include "rcconf.h"

RcConf::RcConf(std::string path, short nbkp) : rcFileName(path), rcIniFile(std::make_unique<CIniFile>()), nBackups(nbkp)
{
}

RcConf::~RcConf()
{
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
    std::map<std::string, std::vector<std::string>> bridgeMembersByName;
    std::set<std::string> setActiveRoutes;
    std::set<std::string> ifacesWithAliasesForm;
    bool is_primary = false;

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
        // Interfaces that have an ifconfig_<name>_aliases (plural) record: that
        // form takes priority over ifconfig_<name>_aliasN (singular) for the
        // SAME interface. Collected up front, per interface name, rather than
        // tracked as a single running flag, so an _aliases record on one
        // interface can't suppress _aliasN parsing on a different interface
        // that happens to sort later in mapInterfaces.
        for (const auto& [ifname, ifconfig] : mapInterfaces)
        {
            if(pos1=ifname.find_last_of("_"); pos1!=std::string::npos && ifname.find(ALIASES_SUFFIX)!=std::string::npos)
                ifacesWithAliasesForm.insert(ifname.substr(0, pos1));
        }

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
                    continue;
                }
                else if(pos2=ifname.find(ALIAS_SUFFIX); pos2!=std::string::npos)
                //          ifname     ifconfig
                // ifconfig_em0_alias0="inet 192.168.212.102 netmask 255.255.255.0"
                // ifconfig_em0_alias1="inet 192.168.212.202 netmask 0xffffff00"
                // ifconfig_em0_alias1="inet 192.168.212.202/24"
                // TODO: check that the aliases numbers start from 0 (if not - they are ignored by system)
                {
                    name=ifname.substr(0, pos1);
                    if(ifacesWithAliasesForm.contains(name))
                        // ALIASES_SUFFIX has priority over ALIAS_SUFFIX, per interface
                        continue;
                    alias_name=ifname.substr(pos1+1, std::string::npos);
                    jdata = getIpConfFromString(ifconfig);
                    if(!jdata.empty())
                    {
                        jaliases[name].push_back(jdata);
                        //LOG_S(INFO) << "getRcIpConfig got IP " << alias_name << " alias data for interface " << name << ":" << std::endl << jdata.dump();
                    }
                    continue;
                }
                else if(element=vlanConfigNameToDisplay(ifname); element != ifname)
                // ifconfig_em0_101="inet 192.0.2.1/24"
                {
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

            element = ifconfig;
            Trim(element, quotes);
            if(Tool::classifyIfType(ifname) == JSON_PARAM_IF_TYPE_BRIDGE)
            {
                std::vector<std::string> members = extractBridgeMembers(element);
                if(!members.empty())
                    bridgeMembersByName[ifname] = members;
            }
            extractMtuAndCaps(element, jif);
            bool jif_has_extra = jif.contains(JSON_PARAM_MTU) || jif.contains(JSON_PARAM_TXCSUM_IPV4) || jif.contains(JSON_PARAM_TXCSUM_IPV6);
            jdata = getIpConfFromString(element);
            if(!jdata.empty())
            {
                if(is_primary)
                    jdata[JSON_PARAM_ADDR_PRIMARY] = is_primary;
                jif[JSON_PARAM_ADDRESSES].push_back(jdata);
            }
            if(!jdata.empty() || jif_has_extra)
                jarInterfaces.push_back(jif);
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
                jrt[JSON_PARAM_ADDRESSES] = json::array({jdata});
                jarRoutes.push_back(jrt);
            }
        }
        jdata.clear();
    }
// Iterate interfaces to insert default route and aliases
    for(auto& j : jarInterfaces)
    {
        // An interface can now reach this array with no ADDRESSES at all
        // (e.g. only MTU/capability settings, or bridge/tun/tap MEMBERS with
        // no address of its own) - nothing to validate a default route
        // against in that case.
        if(!j.contains(JSON_PARAM_ADDRESSES))
            continue;
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
// Cloned interfaces (bridge, ...): attach MEMBERS (parsed out of
// ifconfig_<name>'s "addm" tokens, in the main loop above) to an interface
// already present (one that also carries an address), or add a
// members-only entry for one that doesn't (e.g. a bridge with no address).
    for(const auto& [cloned_name, cloned_members] : bridgeMembersByName)
    {
        bool found = false;
        for(auto& j : jarInterfaces)
        {
            if(j[JSON_PARAM_IF_NAME].get<std::string>() == cloned_name)
            {
                j[JSON_PARAM_MEMBERS] = cloned_members;
                found = true;
                break;
            }
        }
        if(!found)
        {
            json j_cloned;
            j_cloned[JSON_PARAM_IF_NAME] = cloned_name;
            j_cloned[JSON_PARAM_MEMBERS] = cloned_members;
            jarInterfaces.push_back(j_cloned);
        }
    }
// Iterate routes to remove inactive ones and validate the active ones
    for(auto &jit : jarRoutes.items())
    {
        if(setActiveRoutes.contains(jit.value().at(JSON_PARAM_RT_NAME).get<std::string>()))
        {
            jdata = jit.value().at(JSON_PARAM_ADDRESSES).at(0);
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

std::string RcConf::ipv4FromOctets(unsigned int a, unsigned int b, unsigned int c, unsigned int d)
{
    return std::to_string(a) + "." + std::to_string(b) + "." + std::to_string(c) + "." + std::to_string(d);
}

bool RcConf::validOctets(unsigned int a, unsigned int b, unsigned int c, unsigned int d)
{
    return a <= 255 && b <= 255 && c <= 255 && d <= 255;
}

std::string RcConf::vlanConfigNameToDisplay(const std::string& ifname)
{
    size_t pos = ifname.find_last_of("_");
    if(pos == std::string::npos || strtol(ifname.substr(pos+1, std::string::npos).c_str(), nullptr, 10) <= 0)
        return ifname;
    std::string result = ifname;
    result.replace(pos, 1, ".");
    return result;
}

std::string RcConf::vlanDisplayNameToConfig(const std::string& ifname)
{
    size_t pos = ifname.find_last_of(".");
    if(pos == std::string::npos || strtol(ifname.substr(pos+1, std::string::npos).c_str(), nullptr, 10) <= 0)
        return ifname;
    std::string result = ifname;
    result.replace(pos, 1, "_");
    return result;
}

std::vector<std::string> RcConf::parseIfNameList(const std::string& value)
{
    std::vector<std::string> names;
    std::stringstream ss(value);
    std::string name;

    while(ss >> name)
        names.push_back(name);
    return names;
}

std::string RcConf::joinIfNameList(const std::vector<std::string>& names)
{
    std::string result;

    for(const auto& name : names)
    {
        if(!result.empty())
            result += " ";
        result += name;
    }
    return result;
}

std::vector<std::string> RcConf::extractBridgeMembers(std::string& ifconfig)
{
    std::vector<std::string> members;
    std::vector<std::string> remaining_tokens;
    std::stringstream ss(ifconfig);
    std::string token;

    while(ss >> token)
    {
        if(token == BRIDGE_MEMBER_TOKEN)
        {
            if(ss >> token)
                members.push_back(token);
            continue;
        }
        if(token == IF_UP_TOKEN)
            continue;
        remaining_tokens.push_back(token);
    }

    ifconfig = joinIfNameList(remaining_tokens);
    return members;
}

void RcConf::extractMtuAndCaps(std::string& ifconfig, json& jif)
{
    std::vector<std::string> remaining_tokens;
    std::stringstream ss(ifconfig);
    std::string token;

    while(ss >> token)
    {
        if(token == MTU_TOKEN)
        {
            std::string mtu_str;
            char* end = nullptr;
            long mtu_val;
            if(ss >> mtu_str)
            {
                mtu_val = strtol(mtu_str.c_str(), &end, 10);
                if(end != nullptr && *end == '\0' && mtu_val > 0 && mtu_val <= 65535)
                    jif[JSON_PARAM_MTU] = (int)mtu_val;
            }
            continue;
        }
        if(token == TXCSUM_TOKEN)
        {
            jif[JSON_PARAM_TXCSUM_IPV4] = JSON_PARAM_ON;
            continue;
        }
        if(token == "-" + TXCSUM_TOKEN)
        {
            jif[JSON_PARAM_TXCSUM_IPV4] = JSON_PARAM_OFF;
            continue;
        }
        if(token == TXCSUM6_TOKEN)
        {
            jif[JSON_PARAM_TXCSUM_IPV6] = JSON_PARAM_ON;
            continue;
        }
        if(token == "-" + TXCSUM6_TOKEN)
        {
            jif[JSON_PARAM_TXCSUM_IPV6] = JSON_PARAM_OFF;
            continue;
        }
        remaining_tokens.push_back(token);
    }

    ifconfig = joinIfNameList(remaining_tokens);
}

json RcConf::getIpConfFromString(std::string ifconfig)
{
    unsigned int o1, o2, o3, o4;
    unsigned int m1, m2, m3, m4;
    unsigned int prefix;
    unsigned int hexmask;
    int consumed;
    struct in_addr ip_mask;
    const std::string quotes = "\"";
    const std::string space = " ";
    json jdata = {};

    Trim(ifconfig, quotes);
    Trim(ifconfig, space);

    if(ifconfig.find(DHCP_SUFFIX) != std::string::npos)
    {
        jdata[JSON_PARAM_IPV4_ADDR] = ifconfig;
        return jdata;
    }

    // "inet 127.0.0.251/24"
    if(sscanf(ifconfig.c_str(), IF_FORMAT_CIDR.c_str(), &o1, &o2, &o3, &o4, &prefix) == 5 &&
       validOctets(o1, o2, o3, o4) && prefix > 0 && prefix <= 32)
    {
        jdata[JSON_PARAM_IPV4_ADDR] = ipv4FromOctets(o1, o2, o3, o4);
        jdata[JSON_PARAM_IPV4_MASK] = getStrInetMaskFromPrefix(prefix);
        return jdata;
    }

    // "inet 127.0.0.251 netmask 255.255.255.0"
    if(sscanf(ifconfig.c_str(), IF_FORMAT_NETMASK_DOTTED.c_str(), &o1, &o2, &o3, &o4, &m1, &m2, &m3, &m4, &consumed) == 8 &&
       consumed == static_cast<int>(ifconfig.length()) &&
       validOctets(o1, o2, o3, o4) && validOctets(m1, m2, m3, m4))
    {
        jdata[JSON_PARAM_IPV4_ADDR] = ipv4FromOctets(o1, o2, o3, o4);
        jdata[JSON_PARAM_IPV4_MASK] = ipv4FromOctets(m1, m2, m3, m4);
        return jdata;
    }

    // "inet 127.0.0.251 netmask 0xffffff00"
    if(sscanf(ifconfig.c_str(), IF_FORMAT_NETMASK_HEX.c_str(), &o1, &o2, &o3, &o4, &hexmask, &consumed) == 5 &&
       consumed == static_cast<int>(ifconfig.length()) &&
       validOctets(o1, o2, o3, o4))
    {
        hexmask = htonl(hexmask);
        ip_mask.s_addr = hexmask;
        jdata[JSON_PARAM_IPV4_ADDR] = ipv4FromOctets(o1, o2, o3, o4);
        jdata[JSON_PARAM_IPV4_MASK] = std::string(inet_ntoa(ip_mask));
        return jdata;
    }

    return jdata;
}

json RcConf::getRouteConfFromString(std::string rtconfig)
{
    unsigned int o1, o2, o3, o4;
    unsigned int m1, m2, m3, m4;
    unsigned int g1, g2, g3, g4;
    unsigned int prefix;
    unsigned int hexmask;
    int consumed;
    struct in_addr ip_mask;
    const std::string quotes = "\"";
    const std::string space = " ";
    json jdata = {};

    Trim(rtconfig, quotes);
    Trim(rtconfig, space);

    // "-net 192.168.32.0/24 192.168.213.252"
    if(sscanf(rtconfig.c_str(), RT_FORMAT_NET_CIDR.c_str(), &o1, &o2, &o3, &o4, &prefix, &g1, &g2, &g3, &g4) == 9 &&
       validOctets(o1, o2, o3, o4) && validOctets(g1, g2, g3, g4) && prefix > 0 && prefix <= 32)
    {
        jdata[JSON_PARAM_IPV4_ADDR] = ipv4FromOctets(o1, o2, o3, o4);
        jdata[JSON_PARAM_IPV4_MASK] = getStrInetMaskFromPrefix(prefix);
        jdata[JSON_PARAM_IPV4_GW] = ipv4FromOctets(g1, g2, g3, g4);
        return jdata;
    }

    // "-net 192.168.32.0 -netmask 255.255.255.0 192.168.213.252"
    if(sscanf(rtconfig.c_str(), RT_FORMAT_NET_DOTTED.c_str(),
              &o1, &o2, &o3, &o4, &m1, &m2, &m3, &m4, &g1, &g2, &g3, &g4, &consumed) == 12 &&
       consumed == static_cast<int>(rtconfig.length()) &&
       validOctets(o1, o2, o3, o4) && validOctets(m1, m2, m3, m4) && validOctets(g1, g2, g3, g4))
    {
        jdata[JSON_PARAM_IPV4_ADDR] = ipv4FromOctets(o1, o2, o3, o4);
        jdata[JSON_PARAM_IPV4_MASK] = ipv4FromOctets(m1, m2, m3, m4);
        jdata[JSON_PARAM_IPV4_GW] = ipv4FromOctets(g1, g2, g3, g4);
        return jdata;
    }

    // "-net 192.168.32.0 -netmask 0xffffff00 192.168.213.252"
    if(sscanf(rtconfig.c_str(), RT_FORMAT_NET_HEX.c_str(), &o1, &o2, &o3, &o4, &hexmask, &g1, &g2, &g3, &g4, &consumed) == 9 &&
       consumed == static_cast<int>(rtconfig.length()) &&
       validOctets(o1, o2, o3, o4) && validOctets(g1, g2, g3, g4))
    {
        hexmask = htonl(hexmask);
        ip_mask.s_addr = hexmask;
        jdata[JSON_PARAM_IPV4_ADDR] = ipv4FromOctets(o1, o2, o3, o4);
        jdata[JSON_PARAM_IPV4_MASK] = std::string(inet_ntoa(ip_mask));
        jdata[JSON_PARAM_IPV4_GW] = ipv4FromOctets(g1, g2, g3, g4);
        return jdata;
    }

    // "-host 191.1.200.4 192.168.213.254"
    if(sscanf(rtconfig.c_str(), RT_FORMAT_HOST.c_str(), &o1, &o2, &o3, &o4, &g1, &g2, &g3, &g4) == 8 &&
       validOctets(o1, o2, o3, o4) && validOctets(g1, g2, g3, g4))
    {
        jdata[JSON_PARAM_IPV4_ADDR] = ipv4FromOctets(o1, o2, o3, o4);
        jdata[JSON_PARAM_IPV4_MASK] = IPV4_MASK_HOST;
        jdata[JSON_PARAM_IPV4_GW] = ipv4FromOctets(g1, g2, g3, g4);
        return jdata;
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
    std::set<std::string> submitted_route_names;
    std::set<std::string> cloned_interface_names;
    const std::string quotes = "\"";
    bool is_dhcp = false;

    if(!iniLoad())
        return false;

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
        // Cloned interfaces (bridge, tun, tap; lagg deferred - its own token
        // vocabulary is more involved). A cloned-type interface needs to be
        // listed in cloned_interfaces regardless of whether it has an
        // address or MEMBERS, so /etc/rc.d/netif actually creates it at
        // boot - so it's also allowed through below with neither (a bare
        // registration request, e.g. a tun/tap meant only to exist for
        // software like OpenVPN to open on its own, not an error). Only
        // bridge currently supports MEMBERS, folded into the same
        // ifconfig_<name> value as the address, as "addm <member>" tokens -
        // the documented FreeBSD rc.conf(5) convention (e.g.
        // ifconfig_bridge0="addm em0 addm em1 up"). MEMBERS for any other
        // type is rejected now rather than silently written where
        // rc.d/netif wouldn't recognize it as membership at all.
        std::string str_members_portion;
        std::string iftype_classified;
        bool is_bridge = false;
        bool is_cloned_type = false;
        bool members_given = false;
        bool wrote_primary_ifconfig = false;
        try
        {
            str_if_name = jif.at(JSON_PARAM_IF_NAME);
            if(str_if_name.empty())
            {
                LOG_S(WARNING) << "setRcIpConfig cannot get interface name";
                continue;
            }
            if(!Tool::isValidIfName(str_if_name))
            {
                LOG_S(WARNING) << "setRcIpConfig rejected invalid interface name";
                continue;
            }
//          Fix VLAN interface name
            str_if_name = vlanDisplayNameToConfig(str_if_name);
            iftype_classified = Tool::classifyIfType(str_if_name);
            is_bridge = iftype_classified == JSON_PARAM_IF_TYPE_BRIDGE;
            is_cloned_type = is_bridge || iftype_classified == JSON_PARAM_IF_TYPE_TUN || iftype_classified == JSON_PARAM_IF_TYPE_TAP;
            members_given = jif.contains(JSON_PARAM_MEMBERS) && is_bridge;
            jar_addresses = jif.contains(JSON_PARAM_ADDRESSES) ? jif.at(JSON_PARAM_ADDRESSES) : json::array();
            if(jar_addresses.empty() && !jif.contains(JSON_PARAM_MEMBERS) && !is_cloned_type &&
               !jif.contains(JSON_PARAM_MTU) && !jif.contains(JSON_PARAM_TXCSUM_IPV4) && !jif.contains(JSON_PARAM_TXCSUM_IPV6))
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
        if(is_cloned_type)
            cloned_interface_names.insert(str_if_name);
        if(jif.contains(JSON_PARAM_MEMBERS) && !is_bridge)
        {
            LOG_S(WARNING) << "setRcIpConfig: " << JSON_PARAM_MEMBERS << " is not supported for interface " << str_if_name;
        }
        else if(members_given)
        {
            std::vector<std::string> member_names;
            bool members_valid = true;
            try
            {
                member_names = jif.at(JSON_PARAM_MEMBERS).get<std::vector<std::string>>();
            }
            catch(std::exception& e)
            {
                LOG_S(WARNING) << "setRcIpConfig: invalid " << JSON_PARAM_MEMBERS << " list for " << str_if_name;
                members_valid = false;
            }
            for(const auto& member : member_names)
            {
                if(!Tool::isValidIfName(member))
                {
                    LOG_S(WARNING) << "setRcIpConfig: invalid member interface name " << member << " for " << str_if_name;
                    members_valid = false;
                    break;
                }
            }
            if(members_valid)
            {
                std::vector<std::string> member_tokens;
                for(const auto& member : member_names)
                {
                    member_tokens.push_back(BRIDGE_MEMBER_TOKEN);
                    member_tokens.push_back(member);
                }
                str_members_portion = joinIfNameList(member_tokens);
            }
            else
            {
                members_given = false;
            }
        }
//  MTU and checksum-offload capability toggles: universal ifconfig(8)
//  keywords, valid for any interface (not just a cloned type), folded into
//  the same ifconfig_<name> value as the address/members portion.
        std::vector<std::string> extra_config_tokens;
        if(jif.contains(JSON_PARAM_MTU))
        {
            try
            {
                int mtu = jif.at(JSON_PARAM_MTU);
                if(mtu > 0 && mtu <= 65535)
                {
                    extra_config_tokens.push_back(MTU_TOKEN);
                    extra_config_tokens.push_back(std::to_string(mtu));
                }
                else
                {
                    LOG_S(WARNING) << "setRcIpConfig: invalid " << JSON_PARAM_MTU << " value for " << str_if_name;
                }
            }
            catch(std::exception& e)
            {
                LOG_S(WARNING) << "setRcIpConfig: invalid " << JSON_PARAM_MTU << " value for " << str_if_name;
            }
        }
        if(jif.contains(JSON_PARAM_TXCSUM_IPV4))
        {
            try
            {
                std::string onoff = jif.at(JSON_PARAM_TXCSUM_IPV4);
                if(onoff == JSON_PARAM_ON)
                    extra_config_tokens.push_back(TXCSUM_TOKEN);
                else if(onoff == JSON_PARAM_OFF)
                    extra_config_tokens.push_back("-" + TXCSUM_TOKEN);
                else
                    LOG_S(WARNING) << "setRcIpConfig: " << JSON_PARAM_TXCSUM_IPV4 << " must be " << JSON_PARAM_ON << " or " << JSON_PARAM_OFF << " for " << str_if_name;
            }
            catch(std::exception& e)
            {
                LOG_S(WARNING) << "setRcIpConfig: invalid " << JSON_PARAM_TXCSUM_IPV4 << " value for " << str_if_name;
            }
        }
        if(jif.contains(JSON_PARAM_TXCSUM_IPV6))
        {
            try
            {
                std::string onoff = jif.at(JSON_PARAM_TXCSUM_IPV6);
                if(onoff == JSON_PARAM_ON)
                    extra_config_tokens.push_back(TXCSUM6_TOKEN);
                else if(onoff == JSON_PARAM_OFF)
                    extra_config_tokens.push_back("-" + TXCSUM6_TOKEN);
                else
                    LOG_S(WARNING) << "setRcIpConfig: " << JSON_PARAM_TXCSUM_IPV6 << " must be " << JSON_PARAM_ON << " or " << JSON_PARAM_OFF << " for " << str_if_name;
            }
            catch(std::exception& e)
            {
                LOG_S(WARNING) << "setRcIpConfig: invalid " << JSON_PARAM_TXCSUM_IPV6 << " value for " << str_if_name;
            }
        }
        std::string str_extra_portion = joinIfNameList(extra_config_tokens);
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
                    ip_addr = jad.at(JSON_PARAM_IPV4_ADDR);
                    ip_mask = "";
                    if(ip_addr.find(DHCP_SUFFIX) == std::string::npos)
                    {
                        ip_mask = jad.at(JSON_PARAM_IPV4_MASK);
                        is_dhcp = false;
                        if(!Tool::isValidIpAddr(ip_addr) || !Tool::isValidIpAddr(ip_mask))
                            throw std::invalid_argument("invalid IPv4 address or mask");
                    }
                    else
                    {
                        is_dhcp = true;
                        if(!Tool::isValidRcWord(ip_addr))
                            throw std::invalid_argument("invalid DHCP keyword");
                    }
                    str_conf_key = IFCONFIG_KEY_PREFIX + str_if_name;
                    {
                        std::vector<std::string> ifconfig_tokens;
                        if(!str_members_portion.empty())
                            ifconfig_tokens.push_back(str_members_portion);
                        if(ip_mask.empty())
                            ifconfig_tokens.push_back(ip_addr);
                        else
                            ifconfig_tokens.push_back(INET_ADDR + " " + ip_addr + " " + INET_MASK + " " + ip_mask);
                        if(!str_extra_portion.empty())
                            ifconfig_tokens.push_back(str_extra_portion);
                        if(is_bridge)
                            ifconfig_tokens.push_back(IF_UP_TOKEN);
                        str_conf_value = "\"" + joinIfNameList(ifconfig_tokens) + "\"";
                    }
                    str_old_conf_value = rcIniFile->GetKeyValue(DEFAULT_SECTION, str_conf_key);
                    if(str_conf_value != str_old_conf_value)
                        rcIniFile->SetKeyValue(DEFAULT_SECTION, str_conf_key, str_conf_value);
                    wrote_primary_ifconfig = true;
                }
            }
            catch(std::exception& e)
            {
                LOG_S(WARNING) << "setRcIpConfig cannot decode data: " << jad.dump() << " for interface " << str_if_name << " : " << e.what();
                continue;
            }
            try
            {
                if(is_addr_primary)
                {
                    str_conf_key = DEFAULT_ROUTE_KEY;
                    if(!is_dhcp)
                    {
                        ip_gw = jad.at(JSON_PARAM_IPV4_GW);
                        if(!Tool::isValidIpAddr(ip_gw))
                            throw std::invalid_argument("invalid gateway address");
                        str_conf_value = "\"" + ip_gw + "\"";
                        str_old_conf_value = rcIniFile->GetKeyValue(DEFAULT_SECTION, str_conf_key);
                        if(str_conf_value != str_old_conf_value)
                            rcIniFile->SetKeyValue(DEFAULT_SECTION, str_conf_key, str_conf_value);
                    }
                    else
                    {
                        rcIniFile->GetSection(DEFAULT_SECTION)->RemoveKey(str_conf_key);
                    }
                }
            }
            catch(std::exception& e)
            {
                LOG_S(WARNING) << "setRcIpConfig cannot decode data: " << jad.dump() << " for interface " << str_if_name << " : " << e.what();
            }
        }
//  No primary address was written above, but there's still something to
//  persist: a cloned-type interface (members-only bridge, or a bare tun/tap
//  that's only meant to exist, e.g. for software like OpenVPN to open on
//  its own - gets at minimum "up"), and/or MTU/capability settings on a
//  plain interface with no address of its own.
        if(!wrote_primary_ifconfig && (is_cloned_type || !str_extra_portion.empty()))
        {
            str_conf_key = IFCONFIG_KEY_PREFIX + str_if_name;
            std::vector<std::string> ifconfig_tokens;
            if(!str_members_portion.empty())
                ifconfig_tokens.push_back(str_members_portion);
            if(!str_extra_portion.empty())
                ifconfig_tokens.push_back(str_extra_portion);
            if(is_cloned_type)
                ifconfig_tokens.push_back(IF_UP_TOKEN);
            str_conf_value = "\"" + joinIfNameList(ifconfig_tokens) + "\"";
            str_old_conf_value = rcIniFile->GetKeyValue(DEFAULT_SECTION, str_conf_key);
            if(str_conf_value != str_old_conf_value)
                rcIniFile->SetKeyValue(DEFAULT_SECTION, str_conf_key, str_conf_value);
        }
//  Second cycle - put aliases
//  TODO: integrate the second cycle into the first one
        str_conf_key = IFCONFIG_KEY_PREFIX + str_if_name + "_" + ALIASES_SUFFIX;
        str_conf_value = "";
        if(!is_dhcp)
        {
            for(const auto &jad : jar_addresses)
            {
                is_addr_primary = false;
                try
                {
                    is_addr_primary = jad.at(JSON_PARAM_ADDR_PRIMARY);
                }
                catch(std::exception& e)
                {
                    LOG_S(WARNING) << "setRcIpConfig cannot decode data: " << jad.dump() << " : " << e.what();
                }
                try
                {
                    if(!is_addr_primary)
                    {
    //  TODO: deal with IPv6 addresses
                        ip_addr = jad.at(JSON_PARAM_IPV4_ADDR);
                        ip_mask = jad.at(JSON_PARAM_IPV4_MASK);
                        if(!Tool::isValidIpAddr(ip_addr) || !Tool::isValidIpAddr(ip_mask))
                            throw std::invalid_argument("invalid alias address or mask");
                        if(!str_conf_value.empty())
                            str_conf_value += " ";
                        str_conf_value += INET_ADDR + " " + ip_addr + " "
                                        + INET_MASK + " " + ip_mask;
                    }
                }
                catch(std::exception& e)
                {
                    LOG_S(WARNING) << "setRcIpConfig cannot decode data: " << jad.dump() << " for interface " << str_if_name << " : " << e.what();
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
        }
        else
        {
            rcIniFile->GetSection(DEFAULT_SECTION)->RemoveKey(str_conf_key);
        }
        // Remove stale records like ifconfig_em0_alias0="..." from the config file:
        // only keys that actually exist are looked up and removed (rather than
        // blindly probing hundreds of indices on every write).
        {
            CIniSection* section = rcIniFile->GetSection(DEFAULT_SECTION);
            std::string alias_key_prefix = IFCONFIG_KEY_PREFIX + str_if_name + "_" + ALIAS_SUFFIX;
            std::set<std::string> stale_alias_keys;
            for(const auto& key : section->GetKeys())
            {
                std::string key_name = key->GetKeyName();
                if(key_name.length() > alias_key_prefix.length() &&
                   key_name.compare(0, alias_key_prefix.length(), alias_key_prefix) == 0 &&
                   key_name.find_first_not_of("0123456789", alias_key_prefix.length()) == std::string::npos)
                {
                    stale_alias_keys.insert(key_name);
                }
            }
            for(const auto& key_name : stale_alias_keys)
                section->RemoveKey(key_name);
        }
    }

//  Ensure every interface that was just given MEMBERS is tracked in
//  cloned_interfaces, without disturbing any name already listed there.
    if(!cloned_interface_names.empty())
    {
        std::string str_existing_cloned = rcIniFile->GetKeyValue(DEFAULT_SECTION, CLONED_INTERFACES_KEY);
        Trim(str_existing_cloned, quotes);
        std::vector<std::string> existing_cloned_list = parseIfNameList(str_existing_cloned);
        std::set<std::string> all_cloned(existing_cloned_list.begin(), existing_cloned_list.end());
        size_t before = all_cloned.size();
        all_cloned.insert(cloned_interface_names.begin(), cloned_interface_names.end());
        if(all_cloned.size() != before)
        {
            std::vector<std::string> new_cloned_list(all_cloned.begin(), all_cloned.end());
            str_conf_value = "\"" + joinIfNameList(new_cloned_list) + "\"";
            rcIniFile->SetKeyValue(DEFAULT_SECTION, CLONED_INTERFACES_KEY, str_conf_value);
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
            if(!Tool::isValidName(str_rt_name))
            {
                LOG_S(WARNING) << "setRcIpConfig rejected invalid route name";
                continue;
            }
            submitted_route_names.insert(str_rt_name);
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
            jar_addresses = jar_addresses.at(0);
        }
        catch (std::exception& e)
        {
            LOG_S(WARNING) << "Exception in setRcIpConfig trying to decode data for route " << str_rt_name << " : " << e.what();
            continue;
        }

        str_conf_value = "";
        try
        {
//  TODO: deal with IPv6 addresses
            ip_addr = jar_addresses.at(JSON_PARAM_IPV4_ADDR);
            ip_mask = jar_addresses.at(JSON_PARAM_IPV4_MASK);
            ip_gw = jar_addresses.at(JSON_PARAM_IPV4_GW);
            if( !Tool::isValidIpAddr(ip_addr) || !Tool::isValidIpAddr(ip_mask) || !Tool::isValidIpAddr(ip_gw) )
            {
                LOG_S(WARNING) << "setRcIpConfig received invalid addresses " << jar_addresses.dump() << " for route " << str_rt_name;
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

    // Remove stale route_<name> records for routes no longer present in the submitted
    // config, mirroring the alias cleanup above: only keys that actually exist are
    // looked up and removed. Scoped to have_routes so an interfaces-only write (no
    // ROUTES data at all) can't be misread as "delete every route".
    if(have_routes)
    {
        CIniSection* section = rcIniFile->GetSection(DEFAULT_SECTION);
        std::set<std::string> stale_route_keys;
        for(const auto& key : section->GetKeys())
        {
            std::string key_name = key->GetKeyName();
            if(key_name.compare(0, ROUTE_KEY_PREFIX.length(), ROUTE_KEY_PREFIX) == 0 &&
               !submitted_route_names.contains(key_name.substr(ROUTE_KEY_PREFIX.length())))
            {
                stale_route_keys.insert(key_name);
            }
        }
        for(const auto& key_name : stale_route_keys)
            section->RemoveKey(key_name);

        str_conf_key = ROUTES_KEY;
        if(!str_active_routes.empty())
        {
            str_conf_value = "\"" + str_active_routes + "\"";
            str_old_conf_value = rcIniFile->GetKeyValue(DEFAULT_SECTION, str_conf_key);
            if(str_conf_value != str_old_conf_value)
                rcIniFile->SetKeyValue(DEFAULT_SECTION, str_conf_key, str_conf_value);
        }
        else
        {
            section->RemoveKey(str_conf_key);
        }
    }

    return iniSave();
}

