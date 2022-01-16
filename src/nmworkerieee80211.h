#ifndef NMWORKERIEEE80211_H
#define NMWORKERIEEE80211_H

#include <net/if.h>
#include <cstdint>
#include <stddef.h>
#include <lib80211/lib80211_regdomain.h>
#include <lib80211/lib80211_regdomain.h>
//#include <lib80211/lib80211_ioctl.h>
#include <net80211/ieee80211_ioctl.h>
#include <net/ethernet.h>

#include "loguru/loguru.hpp"
#include "sockpp/socket.h"
#include "sockpp/version.h"
#include "nmworkerbase.h"
#include "tool.h"

// The "native" header <lib80211/lib80211_ioctl.h> does not declare these function as "C"
extern "C" int lib80211_get80211(int s, const char *name, int type, void *data, int len);
extern "C" int lib80211_get80211len(int s, const char *name, int type, void *data, int len, int *plen);
extern "C" int lib80211_get80211val(int s, const char *name, int type, int *val);
extern "C" int lib80211_set80211(int s, const char *name, int type, int val, int len, void *data);

/*
 * This worker use lib80211 to get the information directly from kernel using ioctl
*/
class NmWorkerIeee80211 : public NmWorkerBase
{
protected:
    static constexpr NmCommand Cmds[] =
    {
        { NmScope::WIFI, NmCmd::NET_LIST },
        { NmScope::WIFI, NmCmd::IF_STATUS }
    };
    static constexpr short MAXCHAN = 1536;
    struct ieee80211req_chaninfo *ChanInfo;
    std::unique_ptr<struct ieee80211_channel> Chan;
    std::unique_ptr<struct ifmediareq> IfMr;
    int HtConf;
    bool getChanInfo(int, std::string);
    int copyEssid(char buf[], size_t bufsize, const u_int8_t *essid, size_t essid_len);
    bool mapFreq(struct ieee80211_channel *chan, int freq, unsigned int flags);
    int promoteChan(unsigned int i);
    int canPromote(unsigned int i, unsigned int from, unsigned int to);
    int getMaxRate(const uint8_t rates[15], uint8_t nrates);
public:
    NmWorkerIeee80211();
    ~NmWorkerIeee80211();
    NmScope getScope();
    json execCmd(NmCommandData*);
    bool isValidCmd(NmCommandData*);
    json execCmdNetList(NmCommandData*);
    json execCmdIfStatus(NmCommandData*);
};

#endif // NMWORKERIEEE80211_H
