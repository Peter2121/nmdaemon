#include "nmworkerieee80211.h"

NmWorkerIeee80211::NmWorkerIeee80211()
{
    ChanInfo = nullptr;
    HtConf = 0;
}

NmWorkerIeee80211::~NmWorkerIeee80211()
{
    if(ChanInfo != nullptr)
        free(ChanInfo);
}

NmScope NmWorkerIeee80211::getScope()
{
    return NmScope::WIFI;
}

json NmWorkerIeee80211::execCmd(NmCommandData* pcmd)
{
    switch (pcmd->getCommand().cmd)
    {
        case NmCmd::NET_LIST :
            return execCmdNetList(pcmd);
        default :
            return { { JSON_PARAM_RESULT, JSON_PARAM_ERR }, {JSON_PARAM_ERR, JSON_DATA_ERR_INVALID_COMMAND} };
    }
}

bool NmWorkerIeee80211::isValidCmd(NmCommandData* pcmd)
{
    if( pcmd->getCommand().scope != getScope() )
        return false;

    for(auto cm : Cmds)
    {
        if ( cm.cmd == pcmd->getCommand().cmd )
            return true;
    }

    return false;
}

// ifconfig code is used here
json NmWorkerIeee80211::execCmdNetList(NmCommandData* pcmd)
{
    constexpr int BUFSIZE = 24*1024;
    std::string ifname;
    char ssid[IEEE80211_NWID_LEN+1];
    const uint8_t *cp;
    int len;
    int idlen;
    json cmd_json = {};
    json ret_list = {};
    json ret_data = {};
    json net = {};
    std::vector<json> vect_nets;

    std::unique_ptr<uint8_t[]> buf = std::make_unique<uint8_t[]>(BUFSIZE);

    try {
        cmd_json = pcmd->getJsonData();
        ifname = cmd_json[JSON_PARAM_DATA][JSON_PARAM_IF_NAME];
    } catch (std::exception& e) {
        LOG_S(ERROR) << "Exception in execCmdNetList - cannot get ifname";
        return JSON_RESULT_ERR;
    }

    sockpp::socket sock = sockpp::socket::create(AF_LOCAL, SOCK_DGRAM);

    if (lib80211_get80211len(sock.handle(), ifname.c_str(), IEEE80211_IOC_SCAN_RESULTS, (void*)&buf[0], sizeof(uint8_t[BUFSIZE]), &len) < 0)
    {
        LOG_S(ERROR) << "Error in execCmdNetList: unable to get network scan results";
        sock.close();
        return JSON_RESULT_ERR;
    }
    if (len < (int)sizeof(struct ieee80211req_scan_result))
    {
        LOG_S(ERROR) << "Error in execCmdNetList: incorrect data received from lib80211";
        sock.close();
        return JSON_RESULT_ERR;
    }

    if(!getChanInfo(sock.handle(), ifname))
    {
        LOG_S(ERROR) << "Error in execCmdNetList: unable to get channels information";
    }

    IfMr = Tool::getMediaState(ifname);
    if(IfMr==nullptr)
    {
        LOG_S(ERROR) << "Error in execCmdNetList: cannot get media state";
    }

    if (lib80211_get80211val(sock.handle(), ifname.c_str(), IEEE80211_IOC_HTCONF, &HtConf) < 0)
    {
        LOG_S(ERROR) << "Error in execCmdNetList: unable to get HtConf";
    }

    cp = (uint8_t*)&buf[0];
    do {
        const struct ieee80211req_scan_result *sr;
        const uint8_t *vp, *idp;
        int len_ssid;

        sr = (const struct ieee80211req_scan_result *) cp;
        vp = cp + sr->isr_ie_off;
        if (sr->isr_meshid_len) {
            idp = vp + sr->isr_ssid_len;
            idlen = sr->isr_meshid_len;
        } else {
            idp = vp;
            idlen = sr->isr_ssid_len;
        }
/*
        printf("%-*.*s  %s  %3d  %3dM %4d:%-4d %4d %-4.4s"
            , IEEE80211_NWID_LEN
              , len_ssid
              , ssid
            , ether_ntoa((const struct ether_addr *) sr->isr_bssid)
            , ieee80211_mhz2ieee(sr->isr_freq, sr->isr_flags)
            , getmaxrate(sr->isr_rates, sr->isr_nrates)
            , (sr->isr_rssi/2)+sr->isr_noise, sr->isr_noise
            , sr->isr_intval
            , getcaps(sr->isr_capinfo)
        );
        printies(vp + sr->isr_ssid_len + sr->isr_meshid_len, sr->isr_ie_len, 24);
*/
        memset(ssid, 0, sizeof(ssid));
        len_ssid = copyEssid(ssid, IEEE80211_NWID_LEN, idp, idlen);
        if(len_ssid>0)
            net[JSON_PARAM_SSID] = ssid;
        else
            net[JSON_PARAM_SSID] = "";
        net[JSON_PARAM_BSSID] = ether_ntoa((const struct ether_addr *) sr->isr_bssid);
        Chan = std::make_unique<struct ieee80211_channel>();
        if(mapFreq(Chan.get(), sr->isr_freq, sr->isr_flags))
        {
            net[JSON_PARAM_CHANNEL] = Chan->ic_ieee;
        }
        net[JSON_PARAM_MAX_RATE] = getMaxRate(sr->isr_rates, sr->isr_nrates);
        net[JSON_PARAM_SIGNAL] = (sr->isr_rssi/2)+sr->isr_noise;
        net[JSON_PARAM_NOISE] = sr->isr_noise;
        cp += sr->isr_len, len -= sr->isr_len;
        vect_nets.push_back(net);
    } while (len >= (int)sizeof(struct ieee80211req_scan_result));

    sock.close();
    ret_data[JSON_PARAM_NETWORKS] = vect_nets;
    ret_list[JSON_PARAM_DATA] = ret_data;
    ret_list[JSON_PARAM_RESULT] = JSON_PARAM_SUCC;
    return ret_list;;
}

// ifconfig code is used here
int NmWorkerIeee80211::copyEssid(char ssbuf[], size_t bufsize, const u_int8_t *essid, size_t essid_len)
{
    const u_int8_t *p;
    size_t maxlen;
    u_int i;

    if (essid_len > bufsize)
        maxlen = bufsize;
    else
        maxlen = essid_len;
    /* determine printable or not */
    for (i = 0, p = essid; i < maxlen; i++, p++) {
        if (*p < ' ' || *p > 0x7e)
            break;
    }
    if (i != maxlen) {		/* not printable, print as hex */
        if (bufsize < 3)
            return 0;
        strlcpy(ssbuf, "0x", bufsize);
        bufsize -= 2;
        p = essid;
        for (i = 0; i < maxlen && bufsize >= 2; i++) {
            sprintf(&ssbuf[2+2*i], "%02x", p[i]);
            bufsize -= 2;
        }
        if (i != essid_len)
            memcpy(&ssbuf[2+2*i-3], "...", 3);
    } else {			/* printable, truncate as needed */
        memcpy(ssbuf, essid, maxlen);
        if (maxlen != essid_len)
            memcpy(&ssbuf[maxlen-3], "...", 3);
    }
    return maxlen;
}

// ifconfig code is used here
bool NmWorkerIeee80211::mapFreq(struct ieee80211_channel *chan, int freq, unsigned int flags)
{
    u_int i;
    for (i = 0; i < ChanInfo->ic_nchans; i++) {
        const struct ieee80211_channel *c = &ChanInfo->ic_chans[i];

        if (c->ic_freq == freq && (c->ic_flags & flags) == flags) {
            if (flags == 0) {
                /* when ambiguous promote to ``best'' */
                c = &ChanInfo->ic_chans[promoteChan(i)];
            }
            *chan = *c;
            return true;
        }
    }
    return false;
}

// ifconfig code is used here
int NmWorkerIeee80211::promoteChan(unsigned int i)
{
    /*
     * Query the current mode of the interface in case it's
     * constrained (e.g. to 11a).  We must do this carefully
     * as there may be a pending ifmedia request in which case
     * asking the kernel will give us the wrong answer.  This
     * is an unfortunate side-effect of the way ifconfig is
     * structure for modularity (yech).
     *
     * NB: ifmr is actually setup in getchaninfo (above); we
     *     assume it's called coincident with to this call so
     *     we have a ``current setting''; otherwise we must pass
     *     the socket descriptor down to here so we can make
     *     the ifmedia_getstate call ourselves.
     */
    int chanmode = IfMr.get() != nullptr ? IFM_MODE(IfMr->ifm_current) : IFM_AUTO;

    /* when ambiguous promote to ``best'' */
    /* NB: we abitrarily pick HT40+ over HT40- */
    if (chanmode != IFM_IEEE80211_11B)
        i = canPromote(i, IEEE80211_CHAN_B, IEEE80211_CHAN_G);
    if (chanmode != IFM_IEEE80211_11G && (HtConf & 1)) {
        i = canPromote(i, IEEE80211_CHAN_G,
            IEEE80211_CHAN_G | IEEE80211_CHAN_HT20);
        if (HtConf & 2) {
            i = canPromote(i, IEEE80211_CHAN_G,
                IEEE80211_CHAN_G | IEEE80211_CHAN_HT40D);
            i = canPromote(i, IEEE80211_CHAN_G,
                IEEE80211_CHAN_G | IEEE80211_CHAN_HT40U);
        }
    }
    if (chanmode != IFM_IEEE80211_11A && (HtConf & 1)) {
        i = canPromote(i, IEEE80211_CHAN_A,
            IEEE80211_CHAN_A | IEEE80211_CHAN_HT20);
        if (HtConf & 2) {
            i = canPromote(i, IEEE80211_CHAN_A,
                IEEE80211_CHAN_A | IEEE80211_CHAN_HT40D);
            i = canPromote(i, IEEE80211_CHAN_A,
                IEEE80211_CHAN_A | IEEE80211_CHAN_HT40U);
        }
    }
    return i;
}

// ifconfig code is used here
int NmWorkerIeee80211::canPromote(unsigned int i, unsigned int from, unsigned int to)
{
    const struct ieee80211_channel *fc = &ChanInfo->ic_chans[i];
    u_int j;

    if ((fc->ic_flags & from) != from)
        return i;
    /* NB: quick check exploiting ordering of chans w/ same frequency */
    if (i+1 < ChanInfo->ic_nchans &&
        ChanInfo->ic_chans[i+1].ic_freq == fc->ic_freq &&
        (ChanInfo->ic_chans[i+1].ic_flags & to) == to)
        return i+1;
    /* brute force search in case channel list is not ordered */
    for (j = 0; j < ChanInfo->ic_nchans; j++) {
        const struct ieee80211_channel *tc = &ChanInfo->ic_chans[j];
        if (j != i &&
            tc->ic_freq == fc->ic_freq && (tc->ic_flags & to) == to)
        return j;
    }
    return i;
}

// ifconfig code is used here
bool NmWorkerIeee80211::getChanInfo(int s, std::string ifname)
{
    if (ChanInfo != nullptr)
    {
        free(ChanInfo);
    }
    ChanInfo = (struct ieee80211req_chaninfo *)malloc(IEEE80211_CHANINFO_SIZE(MAXCHAN));
    if (ChanInfo == nullptr)
    {
        LOG_S(ERROR) << "Error in getChanInfo: cannot allocate memory";
        return false;
    }
    if (lib80211_get80211(s, ifname.c_str(), IEEE80211_IOC_CHANINFO, ChanInfo, IEEE80211_CHANINFO_SIZE(MAXCHAN)) < 0)
    {
        LOG_S(ERROR) << "Error in getChanInfo: error calling lib80211_get80211 function";
        return false;
    }
//    gethtconf(s);
//    getvhtconf(s);
    return true;
}

// ifconfig code is used here
int NmWorkerIeee80211::getMaxRate(const uint8_t rates[15], uint8_t nrates)
{
    int i, maxrate = -1;

    for (i = 0; i < nrates; i++) {
        int rate = rates[i] & IEEE80211_RATE_VAL;
        if (rate > maxrate)
            maxrate = rate;
    }
    return maxrate / 2;
}

