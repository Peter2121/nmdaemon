#ifndef RCCONF_H
#define RCCONF_H

#include <string>
#include <stdlib.h>
#include <map>
#include <arpa/inet.h>
//#define LOGURU_WITH_STREAMS 1
#include "loguru/loguru.hpp"
#include "json/json.hpp"
#include "inifile/inifile.h"
#include "nmjsonconst.h"
#include "addr.h"

using json = nlohmann::json;

class rcconf
{
protected:
    static inline const std::string IFCONFIG_KEY_PREFIX = "ifconfig_";
    static inline const std::string DEFAULT_ROUTE_KEY = "defaultrouter";
    static inline const std::string ROUTES_KEY = "static_routes";
    static inline const std::string ROUTE_KEY_PREFIX = "route_";
    static inline const std::string INET_ADDR = "inet";
    static inline const std::string INET_MASK = "netmask";
    static inline const std::string DHCP_SUFFIX = "DHCP";
    static inline const std::string ALIAS_SUFFIX = "aliases";
    std::string rcFileName;
    CIniFile* rcIniFile;
    std::string getStrInetMaskFromPrefix(int);
    json getIpConfFromString(std::string);
    json getRouteConfFromString(std::string);
public:
    rcconf(std::string);
    ~rcconf();
    bool iniLoad();
    json getRcIpConfig();
    bool setRcIpConfig(json*);
};

#endif // RCCONF_H
