#ifndef SYS_WORKER_H
#define SYS_WORKER_H

#include <ifaddrs.h>
#include <sys/sockio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/route.h>
#include <unistd.h>
//#define LOGURU_WITH_STREAMS 1
#include "loguru/loguru.hpp"
#include "sockpp/socket.h"
#include "sockpp/version.h"
#include "nmdaemon.h"
#include "nmworkerbase.h"
#include "interface.h"
#include "tool.h"
#include "rcconf.h"
#include "resolvconf.h"
//#include "jailparam.h"

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
        { NmScope::SYSTEM, NmCmd::JAIL_LIST },
        { NmScope::SYSTEM, NmCmd::RESOLVCONF_READ },
        { NmScope::SYSTEM, NmCmd::RESOLVCONF_WRITE },
        { NmScope::SYSTEM, NmCmd::SUBSCRIBE },
        { NmScope::SYSTEM, NmCmd::UNSUBSCRIBE }
    };
    static inline const std::string CONF_SECT_SYSTEM = "SYSTEM";
    static inline const std::string CONF_KEY_RCCONF_FILE = "rcconf_file";
    static inline const std::string RCCONF_FILENAME_DEFAULT = "/etc/rc.conf";
    static inline const std::string CONF_KEY_RESOLVCONF_FILE = "resolvconf_file";
    static inline const std::string RESOLVCONF_FILENAME_DEFAULT = "/etc/resolv.conf";
    static inline const std::string CONF_KEY_RCCONF_BACKUPS = "rcconf_backups";
    static inline const std::string RCCONF_BACKUPS_DEFAULT="5";
    static inline const std::string CONF_KEY_RESOLVCONF_BACKUPS = "resolvconf_backups";
    static inline const std::string RESOLVCONF_BACKUPS_DEFAULT="5";
    std::unique_ptr<RcConf> prcConf;
    std::unique_ptr<ResolvConf> presConf;
//    int getIfFlags(std::string);
//    bool setIfFlags(std::string, int);
    // IF_LIST "FILTER" support. ifMatchesFilterCriteria() ANDs together every
    // criterion present in the filter; to support a new one (e.g. "STATUS",
    // "INTERFACE NAME"), add one more "if(filter.contains(...)) { ... matches
    // = matches && ...; }" block there -- applyIfListFilter() and MODE
    // (INCLUDE/EXCLUDE) handling stay unchanged.
    static bool ifMatchesFilterCriteria(const json& ifJson, const json& filter);
    static bool applyIfListFilter(std::vector<json>& ifList, const json& filter);

    // Decodes one routing-socket message into the EVENT/DATA this worker's
    // publishEvent() should fire. Returns false (nothing published) for a
    // message type this worker doesn't care about, a truncated message, or
    // one whose interface can't be resolved/no longer exists.
    bool decodeSystemEvent(const char* buf, size_t n, NmCmd& outCmd, json& outData);
    // Long-lived PF_ROUTE listener for SYSTEM-scope kernel events
    // (RTM_IFINFO/RTM_IFANNOUNCE), independent of whatever command nmdaemon
    // itself may have issued. Runs on its own independent PF_ROUTE socket,
    // decoding the same kernel message types NmWorkerIf's own monitor also
    // watches for - each worker pays attention for its own scope's
    // subscribers, there is no shared listener between them.
    PfRouteMonitor routeMonitor;
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
    json execCmdResolvConfRead(NmCommandData*);
    json execCmdResolvConfWrite(NmCommandData*);
    json execCmdIfAdd(NmCommandData*);
    json execCmdIfRemove(NmCommandData*);
};

#endif // SYS_WORKER_H
