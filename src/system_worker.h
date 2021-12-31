#ifndef SYS_WORKER_H
#define SYS_WORKER_H

#include <ifaddrs.h>
#include <sys/sockio.h>
//#define LOGURU_WITH_STREAMS 1
#include "loguru/loguru.hpp"
#include "sockpp/socket.h"
#include "sockpp/version.h"
#include "nmdaemon.h"
#include "nmworker.h"
#include "interface.h"
#include "tool.h"
#include "rcconf.h"

extern std::shared_ptr<NmConfig> sp_conf;

class system_worker : public NmWorker
{
protected:
    static constexpr nmcommand Cmds[] =
    {
        { nmscope::SYSTEM, nmcmd::IF_ADD },
        { nmscope::SYSTEM, nmcmd::IF_REMOVE },
        { nmscope::SYSTEM, nmcmd::IF_ENABLE },
        { nmscope::SYSTEM, nmcmd::IF_DISABLE },
        { nmscope::SYSTEM, nmcmd::IF_LIST },
        { nmscope::SYSTEM, nmcmd::RCCONF_READ },
        { nmscope::SYSTEM, nmcmd::RCCONF_WRITE }
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
    system_worker();
    ~system_worker();
    nmscope getScope();
    json execCmd(nmcommand_data*);
    bool isValidCmd(nmcommand_data*);
    json execCmdIfList(nmcommand_data*);
    json execCmdIfEnable(nmcommand_data*);
    json execCmdIfDisable(nmcommand_data*);
    json execCmdRcConfRead(nmcommand_data*);
    json execCmdRcConfWrite(nmcommand_data*);
};

#endif // SYS_WORKER_H
