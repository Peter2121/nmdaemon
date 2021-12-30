#ifndef IF_WORKER_H
#define IF_WORKER_H

#include <thread>
#include <ifaddrs.h>
#include <sys/sockio.h>
//#define LOGURU_WITH_STREAMS 1
#include "loguru/loguru.hpp"
#include "sockpp/socket.h"
#include "sockpp/version.h"
#include "nmworker.h"
#include "Interface.h"
#include "tool.h"

class if_worker : public nmworker
{
protected:
    static constexpr nmcommand Cmds[] =
    {
        { nmscope::INTERFACE, nmcmd::IP_ADDR_SET },
        { nmscope::INTERFACE, nmcmd::IP4_ADDR_GET },
        { nmscope::INTERFACE, nmcmd::IP6_ADDR_GET },
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
    static inline const std::string DHCP_CLIENT_EXEC = "/sbin/dhclient";
    std::string ifName = "";
    std::shared_ptr<address_base> getMainIfAddr(short family);
    bool removeIfAddr(std::shared_ptr<address_base>);
    bool addIfAddr(std::shared_ptr<addr>);
    bool isDHCPEnabled();
    bool termDHCPClient();
    bool killDHCPClient();
    bool enableDHCP();
//    addr* getAddrFromJson(json);
public:
    if_worker();
    ~if_worker();
    nmscope getScope();
    json execCmd(nmcommand_data*);
    bool isValidCmd(nmcommand_data*);
    json execCmdIpAddrSet(nmcommand_data*);
    json execCmdIpAddrGet(nmcommand_data*);
    json execCmdIpAddrAdd(nmcommand_data*);
    json execCmdIpAddrRemove(nmcommand_data*);
    json execCmdMtuGet(nmcommand_data*);
    json execCmdMtuSet(nmcommand_data*);
    json execCmdDHCPEnable(nmcommand_data*);
//    json execCmdIfEnable(nmcommand_data*);
//    json execCmdIfDisable(nmcommand_data*);
};

#endif // IF_WORKER_H
