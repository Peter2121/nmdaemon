#ifndef RESOLVCONF_H
#define RESOLVCONF_H

#include <string>
#include "loguru/loguru.hpp"
#include "json/json.hpp"
#include "tool.h"
#include "addressip4.h"

using json = nlohmann::json;

class ResolvConf
{
protected:
    static inline const std::string NAMESERVER = "nameserver";
    static inline const std::string DOMAIN = "domain";
    static inline const std::string SEARCH = "search";

    std::string confFileName;
    short nBackups;

public:
    ResolvConf(std::string path, short nbkp);
    bool rotateConfFile();
    json getConfig();
    bool setConfig(json);
};

#endif // RESOLVCONF_H
