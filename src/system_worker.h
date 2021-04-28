#ifndef SYS_WORKER_H
#define SYS_WORKER_H

#include <ifaddrs.h>
#include <sys/sockio.h>
#define LOGURU_WITH_STREAMS 1
#include "loguru/loguru.hpp"
#include "sockpp/socket.h"
#include "sockpp/version.h"
#include "nmworker.h"
#include "interface.h"
#include "tool.h"

class system_worker : public nmworker
{
protected:
    static constexpr nmcommand Cmds[] =
    {
        { nmscope::SYSTEM, nmcmd::IF_ADD },
        { nmscope::SYSTEM, nmcmd::IF_REMOVE },
        { nmscope::SYSTEM, nmcmd::IF_ENABLE },
        { nmscope::SYSTEM, nmcmd::IF_DISABLE },
        { nmscope::SYSTEM, nmcmd::IF_LIST }
    };
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
};

#endif // SYS_WORKER_H
