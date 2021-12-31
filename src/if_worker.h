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
#include "interface.h"
#include "tool.h"

class if_worker : public NmWorker
{
protected:
    static constexpr NmCommand Cmds[] =
    {
        { NmScope::INTERFACE, NmCmd::IP_ADDR_SET },
        { NmScope::INTERFACE, NmCmd::IP4_ADDR_GET },
        { NmScope::INTERFACE, NmCmd::IP6_ADDR_GET },
        { NmScope::INTERFACE, NmCmd::IP4_DHCP_ENABLE },
        { NmScope::INTERFACE, NmCmd::IP6_DHCP_ENABLE },
        { NmScope::INTERFACE, NmCmd::IP_ADDR_ADD },
        { NmScope::INTERFACE, NmCmd::IP_ADDR_REMOVE },
        { NmScope::INTERFACE, NmCmd::MTU_GET },
        { NmScope::INTERFACE, NmCmd::MTU_SET },
        { NmScope::INTERFACE, NmCmd::MAC_ADDR_GET },
        { NmScope::INTERFACE, NmCmd::MAC_ADDR_SET }
//        { nmscope::INTERFACE, nmcmd::IF_ENABLE },
//        { nmscope::INTERFACE, nmcmd::IF_DISABLE },
    };
//    int getIfFlags(std::string);
//    bool setIfFlags(std::string, int);
    static inline const std::string DHCP_CLIENT_EXEC = "/sbin/dhclient";
    std::string ifName = "";
    std::shared_ptr<AddressBase> getMainIfAddr(short family);
    bool removeIfAddr(std::shared_ptr<AddressBase>);
    bool addIfAddr(std::shared_ptr<AddressGroup>);
    bool isDHCPEnabled();
    bool termDHCPClient();
    bool killDHCPClient();
    bool enableDHCP();
//    addr* getAddrFromJson(json);
public:
    if_worker();
    ~if_worker();
    NmScope getScope();
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
