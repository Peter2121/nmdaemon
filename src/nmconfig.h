#ifndef NMCONFIG_H
#define NMCONFIG_H

#include <string>
//#define LOGURU_WITH_STREAMS 1
#include "loguru/loguru.hpp"
#include "inifile/inifile.h"

/****** Default values ********

[nmdaemon]
socket_path=/var/run/nmd.socket
socket_uid=root
socket_gid=wheel
socket_mod=660
stderr_log_level=INFO

[SYSTEM]
rcconf_file=/etc/rc.conf
rcconf_backups=5

*******************************/

class NmConfig
{
protected:
    static inline const std::string sections[] =
    {
        "nmdaemon",
        "SYSTEM",
        "INTERFACE",
        "ROUTE",
        "WPA"
    };
    std::string confFileName;
    CIniFile* confIniFile;
    bool isValidSection(const std::string) const;
public:
    NmConfig(std::string);
    ~NmConfig();
    bool iniLoad();
    std::string getConfigFileName() const;
    std::string getConfigValue(const std::string, const std::string) const;
};

#endif // NMCONFIG_H
