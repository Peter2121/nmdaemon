#ifndef TOOL_H
#define TOOL_H

#include <vector>
#include <fstream>
#include <random>
#include <sys/sockio.h>
#include <kvm.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/user.h>
#include <sys/sysctl.h>
#include <net/if_media.h>
#include <net/if.h>
#include <net/if_mib.h>
#include <net/if_dl.h>
#include <net/ethernet.h>
#include <sys/linker.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <string.h>
#include <cctype>
#include <stdexcept>
//#define LOGURU_WITH_STREAMS 1
#include "loguru/loguru.hpp"
#include "json/json.hpp"
#include "magic_enum/magic_enum.hpp"
#include "sockpp/socket.h"
#include "sockpp/version.h"
#include "nmcommand.h"
#include "nmjsonconst.h"
#include "mediastatus.h"
#include "jailparam.h"
//#include "mediadesc.h"

using json = nlohmann::json;

class AddressGroup;

#define	IFM_OPMODE(x) \
    ((x) & (IFM_IEEE80211_ADHOC | IFM_IEEE80211_HOSTAP | \
     IFM_IEEE80211_IBSS | IFM_IEEE80211_WDS | IFM_IEEE80211_MONITOR | \
     IFM_IEEE80211_MBSS))
#define	IFM_IEEE80211_STA	0

class Tool
{
protected:
    static inline const std::string DHCP_CLIENT_PROCESS = "dhclient";
    static inline const std::string DHCP_CLIENT_LEASES_FILE = "/var/db/dhclient.leases.";
    static inline const std::string MEDIA_BL_PREFIXES[] =
    {
        "tun",
        "tap",
        "lo"
    };
public:
    static inline const std::string BRIDGE_IF_PREFIX = "bridge";
    static inline const std::string LAGG_IF_PREFIX = "lagg";
    static std::string classifyIfType(const std::string& ifname); // maps a clone interface's name prefix to its JSON_PARAM_IF_TYPE_* value; empty if none match (e.g. a physical/non-clone interface)
    inline static std::random_device RANDOM_DEVICE;
    inline static std::mt19937 RANDOM_MT;
    inline static std::uniform_int_distribution<int> RND_DISTR;
    static void initRandomGenerator();
    static int getRandomInt();
    static std::shared_ptr<AddressGroup> getAddrFromJson(json);
    static int getIfFlags(std::string);
    static bool setIfFlags(std::string, int);
    static bool setIfCap(const std::string& ifname, int capbit, bool enable); // enable/disable one IFCAP_* capability (e.g. IFCAP_TXCSUM), no-op error if unsupported by the driver
    static bool getIfCaps(const std::string& ifname, int& curcap, int& reqcap); // curcap: currently enabled; reqcap: supported by the driver (both bitmasks of IFCAP_*)
    static bool clonedIfAddMember(const std::string& brname, const std::string& memberName);
    static bool bridgeRemoveMember(const std::string& brname, const std::string& memberName);
    static bool bridgeGetMembers(const std::string& brname, std::vector<std::string>& members);
    static bool clonedIfDestroy(const std::string& ifname);
    static bool laggAddPort(const std::string& laggname, const std::string& portname);
    static bool laggRemovePort(const std::string& laggname, const std::string& portname);
    static bool laggGetPorts(const std::string& laggname, std::vector<std::string>& ports);
    static bool laggGetMaster(const std::string& laggname, std::string& master); // empty on success if no port currently has the MASTER flag
    static bool laggSetProto(const std::string& laggname, int proto); // proto is a lagg_proto (net/if_lagg.h) value
    static bool laggGetProto(const std::string& laggname, int& proto);
    static bool laggSetRRLimit(const std::string& laggname, int limit);
    static bool laggProtoFromName(const std::string& name, int& proto); // maps a JSON_PARAM_LAGG_PROTO_* string to its lagg_proto value
    static std::string laggProtoName(int proto); // reverse of laggProtoFromName; empty string if unknown
    static std::vector<json> getIfInfoJson(const std::string& ifname = std::string()); // ifname empty => all interfaces, otherwise at most one match
    static bool loadIfKldModule(const std::string& prefix); // loads kernel module "if_<prefix>" (e.g. "if_epair") if not already loaded
    static std::string getIfPrimaryAddr4(std::string);
    static std::string getIfPrimaryAddr6(std::string);
    static std::string getIfMacAddr(std::string);
    static bool setIfMacAddr(const std::string& ifname, const std::string& macaddr);
    static std::string getIfHwAddr(const std::string& ifname); // permanent/burned-in address, as reported by SIOCGHWADDR (ifconfig's "hwaddr")
    static bool isValidGw4(uint32_t, uint32_t, uint32_t);
    static bool isValidBcast4(uint32_t, uint32_t, uint32_t);
    static std::vector<std::tuple<int, std::string, std::string>> getActiveProcesses();
    static bool isDHCPEnabled(std::string);
    static bool termDHCPClient(std::string, short sig=SIGTERM);
    static int getDHCPClientPid(std::string);
    static MediaStatus getMediaStatus(std::string);
    static std::string getMediaDesc(std::string);
    static std::string getIfDriverName(const std::string&); // original driver-assigned name (e.g. "igb0"), survives interface rename
    static std::string getIfDescription(const std::string&);
    static std::unique_ptr<struct ifmediareq> getMediaState(std::string);
    static std::string getDescWord(int ifmw, int print_toptype);
    static bool isMediaStatusSupported(std::string);
    static std::vector<JailParam> getJails();
    static std::string getLastDHCPLeaseAddress(const std::string);
    static std::vector<std::string> splitString(const std::string str, const char delim='\n');
    static std::string leftTrimString(const std::string str, const char delim=' ');
    static std::string rightTrimString(const std::string str, const char delim=' ');
    static std::string trimString(const std::string str, const char delim=' ');
    static bool rotateConfigFile(const std::string filename, const int num_versions, const std::string delim=".");
    static bool isValidIfName(const std::string&);      // letters, digits, '.', '_', '-' ; shorter than IFNAMSIZ
    static bool isValidIpAddr(const std::string&);      // inet_pton IPv4 or IPv6
    static bool isValidName(const std::string&);        // letters, digits, '_' (route names)
    static bool isValidRcWord(const std::string&);      // letters, digits, space, '.', '_', '-' (rc.conf keywords)
    static bool isValidDomainName(const std::string&);  // letters, digits, '.', '-'
    static bool isValidWpaParamName(const std::string&);// lowercase letters, digits, '_'
    static std::string hideSecrets(const std::string& text);
};

#endif // TOOL_H
