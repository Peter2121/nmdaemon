#ifndef NMWORKERIF_H
#define NMWORKERIF_H

#include <thread>
#include <ifaddrs.h>
#include <sys/sockio.h>
//#define LOGURU_WITH_STREAMS 1
#include "loguru/loguru.hpp"
#include "sockpp/socket.h"
#include "sockpp/version.h"
#include "nmworkerbase.h"
#include "interface.h"
#include "tool.h"

class NmWorkerIf : public NmWorkerBase
{
protected:
    static constexpr NmCommand Cmds[] =
    {
        { NmScope::INTERFACE, NmCmd::IF_EDIT },
        { NmScope::INTERFACE, NmCmd::IF_GET },
        { NmScope::INTERFACE, NmCmd::IF_RENAME },
        { NmScope::INTERFACE, NmCmd::IP_ADDR_SET },
        { NmScope::INTERFACE, NmCmd::IP4_ADDR_GET },
        { NmScope::INTERFACE, NmCmd::IP6_ADDR_GET },
        { NmScope::INTERFACE, NmCmd::IP4_DHCP_ENABLE },
        { NmScope::INTERFACE, NmCmd::IP6_DHCP_ENABLE },
        { NmScope::INTERFACE, NmCmd::IP4_DHCP_DISABLE },
        { NmScope::INTERFACE, NmCmd::IP6_DHCP_DISABLE },
        { NmScope::INTERFACE, NmCmd::IP4_GET_DHCP_STATUS },
        { NmScope::INTERFACE, NmCmd::IP6_GET_DHCP_STATUS },
        { NmScope::INTERFACE, NmCmd::IP_ADDR_ADD },
        { NmScope::INTERFACE, NmCmd::IP_ADDR_REMOVE },
        { NmScope::INTERFACE, NmCmd::MTU_GET },
        { NmScope::INTERFACE, NmCmd::MTU_SET },
        { NmScope::INTERFACE, NmCmd::MAC_ADDR_GET },
        { NmScope::INTERFACE, NmCmd::MAC_ADDR_SET }
    };
//    int getIfFlags(std::string);
//    bool setIfFlags(std::string, int);
    const short MEDIA_STATUS_MAX_RETRIES = 100;
    static constexpr std::chrono::milliseconds MEDIA_STATUS_RETRY_DELAY{200};
    static inline const std::string DHCP_CLIENT_EXEC = "/sbin/dhclient";
    static constexpr int MTU_MAX = 9192;
    std::string ifName = "";
    std::shared_ptr<AddressBase> getMainIfAddr(short family);
    bool removeIfAddr(std::shared_ptr<AddressBase>);
    bool addIfAddr(std::shared_ptr<AddressGroup>);
    bool isDHCPEnabled();
    bool termDHCPClient();
    bool killDHCPClient();
    bool enableDHCP();
    static bool setMtu(const std::string& ifname, int mtu);
    static bool setBridgeMembers(const std::string& ifname, const std::vector<std::string>& members);
    static bool setLaggMembers(const std::string& ifname, const std::vector<std::string>& members);
    static bool setIfName(const std::string& ifname, const std::string& newname);
    static bool setDescription(const std::string& ifname, const std::string& description);
//    addr* getAddrFromJson(json);
public:
    NmWorkerIf();
    ~NmWorkerIf();
    NmScope getScope();
    json execCmd(NmCommandData*);
    bool isValidCmd(NmCommandData*);
    json execCmdIfEdit(NmCommandData*);
    json execCmdIfGet(NmCommandData*);
    json execCmdIfRename(NmCommandData*);
    json execCmdIpAddrSet(NmCommandData*);
    json execCmdIp4AddrGet(NmCommandData*);
    json execCmdIp6AddrGet(NmCommandData*);
    json execCmdMacAddrGet(NmCommandData*);
    json execCmdMacAddrSet(NmCommandData*);
    json execCmdIpAddrAdd(NmCommandData*);
    json execCmdIpAddrRemove(NmCommandData*);
    json execCmdMtuGet(NmCommandData*);
    json execCmdMtuSet(NmCommandData*);
    json execCmdDHCPEnable(NmCommandData*);
    json execCmdDHCPDisable(NmCommandData*);
    json execCmdDHCPGetStatus(NmCommandData*);
//    json execCmdIfEnable(nmcommand_data*);
//    json execCmdIfDisable(nmcommand_data*);
};

#endif // NMWORKERIF_H
