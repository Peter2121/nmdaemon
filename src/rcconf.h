#ifndef RCCONF_H
#define RCCONF_H

#include <string>
#include <map>
#define LOGURU_WITH_STREAMS 1
#include "loguru/loguru.hpp"
#include "json/json.hpp"
#include "inifile/inifile.h"

using json = nlohmann::json;

class rcconf
{
protected:
    static inline const std::string IFCONFIG_KEY_PREFIX = "ifconfig_";
    static inline const std::string DEFAULT_ROUTE_KEY = "defaultrouter";
    static inline const std::string ROUTES_KEY = "static_routes";
    static inline const std::string ROUTE_KEY_PREFIX = "route_";
    std::string rcFileName;
    CIniFile* rcIniFile;
public:
    rcconf(std::string);
    ~rcconf();
    bool iniLoad();
    json getRcIpConfig();
    bool setRcIpConfig(json*);
};

#endif // RCCONF_H
