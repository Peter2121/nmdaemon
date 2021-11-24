#ifndef NMCONFIG_H
#define NMCONFIG_H

#include <string>
#define LOGURU_WITH_STREAMS 1
#include "loguru/loguru.hpp"
#include "inifile/inifile.h"

/****** Default values ********

[nmdaemon]
socket_path=/var/run/nmd.socket
socket_uid=root
socket_gid=wheel
socket_mod=0660
log_level=INFO

[rcconf]
file=/etc/rc.conf

*******************************/

class nmconfig
{
protected:
    static inline const std::string sections[] =
    {
        "nmdaemon",
        "rcconf",
        "SYSTEM",
        "INTERFACE",
        "ROUTE",
        "WPA"
    };
    std::string confFileName;
    CIniFile* confIniFile;
    bool isValidSection(std::string);
public:
    nmconfig(std::string);
    ~nmconfig();
    bool iniLoad();
    std::string getConfigValue(std::string, std::string);
};

#endif // NMCONFIG_H
