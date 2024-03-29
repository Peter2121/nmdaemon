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
#include <string.h>
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
    inline static std::random_device RANDOM_DEVICE;
    inline static std::mt19937 RANDOM_MT;
    inline static std::uniform_int_distribution<int> RND_DISTR;
    static void initRandomGenerator();
    static int getRandomInt();
    static std::shared_ptr<AddressGroup> getAddrFromJson(json);
    static int getIfFlags(std::string);
    static bool setIfFlags(std::string, int);
    static std::string getIfPrimaryAddr4(std::string);
//    static std::string getIfPrimaryAddr6(std::string);
    static bool isValidGw4(uint32_t, uint32_t, uint32_t);
    static bool isValidBcast4(uint32_t, uint32_t, uint32_t);
    static std::vector<std::tuple<int, std::string, std::string>> getActiveProcesses();
    static bool isDHCPEnabled(std::string);
    static bool termDHCPClient(std::string, short sig=SIGTERM);
    static int getDHCPClientPid(std::string);
    static MediaStatus getMediaStatus(std::string);
    static std::string getMediaDesc(std::string);
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
};

#endif // TOOL_H
