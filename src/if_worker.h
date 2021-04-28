#ifndef IF_WORKER_H
#define IF_WORKER_H

#include <ifaddrs.h>
#include <sys/sockio.h>
#define LOGURU_WITH_STREAMS 1
#include "loguru/loguru.hpp"
#include "sockpp/socket.h"
#include "sockpp/version.h"
#include "nmworker.h"
#include "interface.h"
#include "tool.h"

class if_worker : public nmworker
{
protected:
    static constexpr nmcommand Cmds[] =
    {
        { nmscope::INTERFACE, nmcmd::IP_ADDR_SET },
        { nmscope::INTERFACE, nmcmd::IP4_DHCP_ENABLE },
        { nmscope::INTERFACE, nmcmd::IP6_DHCP_ENABLE },
        { nmscope::INTERFACE, nmcmd::IP_ADDR_ADD },
        { nmscope::INTERFACE, nmcmd::IP_ADDR_REMOVE },
        { nmscope::INTERFACE, nmcmd::MTU_GET },
        { nmscope::INTERFACE, nmcmd::MTU_SET },
        { nmscope::INTERFACE, nmcmd::MAC_ADDR_GET },
        { nmscope::INTERFACE, nmcmd::MAC_ADDR_SET }
//        { nmscope::INTERFACE, nmcmd::IF_ENABLE },
//        { nmscope::INTERFACE, nmcmd::IF_DISABLE },
    };
//    int getIfFlags(std::string);
//    bool setIfFlags(std::string, int);
    std::string ifName = "";
    address_base* getMainIfAddr(short family);
    bool removeIfAddr(const address_base*);
    bool addIfAddr(addr*);
//    addr* getAddrFromJson(json);
public:
    if_worker();
    ~if_worker();
    nmscope getScope();
    json execCmd(nmcommand_data*);
    bool isValidCmd(nmcommand_data*);
    json execCmdIpAddrSet(nmcommand_data*);
    json execCmdIpAddrAdd(nmcommand_data*);
    json execCmdIpAddrRemove(nmcommand_data*);
    json execCmdMtuGet(nmcommand_data*);
    json execCmdMtuSet(nmcommand_data*);
//    json execCmdIfEnable(nmcommand_data*);
//    json execCmdIfDisable(nmcommand_data*);
};

#endif // IF_WORKER_H
