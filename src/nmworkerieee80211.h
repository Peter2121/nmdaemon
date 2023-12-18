#ifndef NMWORKERIEEE80211_H
#define NMWORKERIEEE80211_H

#include <net/if.h>
#include <cstdint>
#include <thread>
#include <stddef.h>
#include <lib80211/lib80211_regdomain.h>
#include <lib80211/lib80211_regdomain.h>
//#include <lib80211/lib80211_ioctl.h>
#include <net80211/ieee80211_ioctl.h>
#include <net80211/ieee80211_freebsd.h>
#include <net/ethernet.h>
#include <net/route.h>

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

// ifconfig code is used here (ifieee80211.c)
#define LE_READ_4(p)				\
    ((u_int32_t)					\
     ((((const u_int8_t *)(p))[0]      ) |		\
      (((const u_int8_t *)(p))[1] <<  8) |		\
      (((const u_int8_t *)(p))[2] << 16) |		\
      (((const u_int8_t *)(p))[3] << 24)))

static __inline bool iswpaoui(const u_int8_t *frm)
{
    return frm[1] > 3 && LE_READ_4(frm+2) == ((WPA_OUI_TYPE<<24)|WPA_OUI);
}

static __inline bool iswmeinfo(const u_int8_t *frm)
{
    return frm[1] > 5 && LE_READ_4(frm+2) == ((WME_OUI_TYPE<<24)|WME_OUI) &&
        frm[6] == WME_INFO_OUI_SUBTYPE;
}

static __inline bool iswmeparam(const u_int8_t *frm)
{
    return frm[1] > 5 && LE_READ_4(frm+2) == ((WME_OUI_TYPE<<24)|WME_OUI) &&
        frm[6] == WME_PARAM_OUI_SUBTYPE;
}

static __inline bool isatherosoui(const u_int8_t *frm)
{
    return frm[1] > 3 && LE_READ_4(frm+2) == ((ATH_OUI_TYPE<<24)|ATH_OUI);
}

static __inline bool istdmaoui(const uint8_t *frm)
{
    return frm[1] > 3 && LE_READ_4(frm+2) == ((TDMA_OUI_TYPE<<24)|TDMA_OUI);
}

static __inline bool iswpsoui(const uint8_t *frm)
{
    return frm[1] > 3 && LE_READ_4(frm+2) == ((WPS_OUI_TYPE<<24)|WPA_OUI);
}

/*
 * This worker use lib80211 to get the information directly from kernel using ioctl
*/
class NmWorkerIeee80211 : public NmWorkerBase
{
protected:
    static constexpr NmCommand Cmds[] =
    {
        { NmScope::WIFI, NmCmd::WIFI_SCAN },
        { NmScope::WIFI, NmCmd::WIFI_SCAN_RESULTS },
        { NmScope::WIFI, NmCmd::WIFI_STATUS }
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
    std::string getCaps(int capinfo);
    std::string getIEs(const u_int8_t *vp, int ielen);
public:
    NmWorkerIeee80211();
    ~NmWorkerIeee80211();
    NmScope getScope();
    json execCmd(NmCommandData*);
    bool isValidCmd(NmCommandData*);
    json execCmdScan(NmCommandData*);
    json execCmdScanResults(NmCommandData*);
    json execCmdStatus(NmCommandData*);
};

#endif // NMWORKERIEEE80211_H
