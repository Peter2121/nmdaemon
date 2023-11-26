#ifndef RCCONF_H
#define RCCONF_H

#include <string>
#include <stdlib.h>
#include <map>
#include <filesystem>
#include <cstdio>
#include <arpa/inet.h>
//#define LOGURU_WITH_STREAMS 1
#include "loguru/loguru.hpp"
#include "json/json.hpp"
#include "inifile/inifile.h"
#include "nmjsonconst.h"
#include "addressgroup.h"

// We need this to use IF_NAME_MAXLEN in sscanf format string and in destination char array definition
#define IF_NAME_MAXLEN 24  // TODO: check kernel sources to find the correct value for maximum length of interface name
#define STR_(X) #X
#define STR(X) STR_(X)

using json = nlohmann::json;

class RcConf
{
protected:
    static inline const std::string IFCONFIG_KEY_PREFIX = "ifconfig_";
    static inline const std::string DEFAULT_ROUTE_KEY = "defaultrouter";
    static inline const std::string ROUTES_KEY = "static_routes";
    static inline const std::string ROUTE_KEY_PREFIX = "route_";
    static inline const std::string INET_ADDR = "inet";
    static inline const std::string INET_MASK = "netmask";
    static inline const std::string DHCP_SUFFIX = "DHCP";
    static inline const std::string ALIASES_SUFFIX = "aliases";
    static inline const std::string ALIAS_SUFFIX = "alias";
    static inline const std::string FILE_VERSIONS_DELIMITER = ".";
    static inline const std::string IPV4_MASK_HOST = "255.255.255.255";
    static inline const std::string ROUTE_PREFIX_HOST = "-host";
    static inline const std::string ROUTE_PREFIX_NET = "-net";
    std::string rcFileName;
    CIniFile* rcIniFile;
    std::string getStrInetMaskFromPrefix(int);
    json getIpConfFromString(std::string);
    json getRouteConfFromString(std::string);
    short nBackups;
public:
    RcConf(std::string, short);
    ~RcConf();
    bool iniLoad();
    bool iniSave();
    bool rotateRcConfFile();
    json getRcIpConfig();
    bool setRcIpConfig(json);
};

#endif // RCCONF_H
