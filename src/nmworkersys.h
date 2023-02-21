#ifndef SYS_WORKER_H
#define SYS_WORKER_H

#include <ifaddrs.h>
#include <sys/sockio.h>
//#define LOGURU_WITH_STREAMS 1
#include "loguru/loguru.hpp"
#include "sockpp/socket.h"
#include "sockpp/version.h"
#include "nmdaemon.h"
#include "nmworkerbase.h"
#include "interface.h"
#include "tool.h"
#include "rcconf.h"
#include "jailparam.h"

extern std::shared_ptr<NmConfig> sp_conf;

class NmWorkerSys : public NmWorkerBase
{
protected:
    static constexpr NmCommand Cmds[] =
    {
        { NmScope::SYSTEM, NmCmd::IF_ADD },
        { NmScope::SYSTEM, NmCmd::IF_REMOVE },
        { NmScope::SYSTEM, NmCmd::IF_ENABLE },
        { NmScope::SYSTEM, NmCmd::IF_DISABLE },
        { NmScope::SYSTEM, NmCmd::IF_LIST },
        { NmScope::SYSTEM, NmCmd::RCCONF_READ },
        { NmScope::SYSTEM, NmCmd::RCCONF_WRITE },
        { NmScope::SYSTEM, NmCmd::JAIL_LIST }
    };
    static inline const std::string CONF_SECT_SYSTEM = "SYSTEM";
    static inline const std::string CONF_KEY_RCCONF_FILE = "rcconf_file";
    static inline const std::string RCCONF_FILENAME_DEFAULT = "/etc/rc.conf";
    static inline const std::string CONF_KEY_RCCONF_BACKUPS = "rcconf_backups";
    static inline const std::string RCCONF_BACKUPS_DEFAULT="5";
    std::unique_ptr<RcConf> prcConf;
//    int getIfFlags(std::string);
//    bool setIfFlags(std::string, int);
public:
    NmWorkerSys();
    ~NmWorkerSys();
    NmScope getScope();
    json execCmd(NmCommandData*);
    bool isValidCmd(NmCommandData*);
    json execCmdIfList(NmCommandData*);
    json execCmdIfEnable(NmCommandData*);
    json execCmdIfDisable(NmCommandData*);
    json execCmdRcConfRead(NmCommandData*);
    json execCmdRcConfWrite(NmCommandData*);
    json execCmdJailList(NmCommandData*);
};

#endif // SYS_WORKER_H
