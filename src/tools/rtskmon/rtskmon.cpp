#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
// Must come after <cstdint>: this header uses uint8_t/uint64_t etc. without
// including it itself, relying on a kernel build context having already
// pulled them in (it's otherwise plain userspace-visible, not _KERNEL-gated).
#include <net80211/ieee80211_freebsd.h>

struct FlagName
{
    long bit;
    const char* name;
};

static const std::vector<FlagName> RTF_TABLE =
{
    { RTF_UP, "UP" }, { RTF_GATEWAY, "GATEWAY" }, { RTF_HOST, "HOST" }, { RTF_REJECT, "REJECT" },
    { RTF_DYNAMIC, "DYNAMIC" }, { RTF_MODIFIED, "MODIFIED" }, { RTF_DONE, "DONE" },
    { RTF_XRESOLVE, "XRESOLVE" }, { RTF_LLINFO, "LLINFO" }, { RTF_STATIC, "STATIC" },
    { RTF_BLACKHOLE, "BLACKHOLE" }, { RTF_PROTO2, "PROTO2" }, { RTF_PROTO1, "PROTO1" },
    { RTF_PROTO3, "PROTO3" }, { RTF_FIXEDMTU, "FIXEDMTU" }, { RTF_PINNED, "PINNED" },
    { RTF_LOCAL, "LOCAL" }, { RTF_BROADCAST, "BROADCAST" }, { RTF_MULTICAST, "MULTICAST" },
    { RTF_STICKY, "STICKY" }, { RTF_GWFLAG_COMPAT, "GWFLAG_COMPAT" }
};

static const std::vector<FlagName> IFF_TABLE =
{
    { IFF_UP, "UP" }, { IFF_BROADCAST, "BROADCAST" }, { IFF_DEBUG, "DEBUG" }, { IFF_LOOPBACK, "LOOPBACK" },
    { IFF_POINTOPOINT, "POINTOPOINT" }, { IFF_DRV_RUNNING, "RUNNING" }, { IFF_NOARP, "NOARP" },
    { IFF_PROMISC, "PROMISC" }, { IFF_ALLMULTI, "ALLMULTI" }, { IFF_DRV_OACTIVE, "OACTIVE" },
    { IFF_SIMPLEX, "SIMPLEX" }, { IFF_MULTICAST, "MULTICAST" }, { IFF_CANTCONFIG, "CANTCONFIG" },
    { IFF_PPROMISC, "PPROMISC" }, { IFF_MONITOR, "MONITOR" }, { IFF_STATICARP, "STATICARP" },
    { IFF_STICKYARP, "STICKYARP" }, { IFF_DYING, "DYING" }, { IFF_RENAMING, "RENAMING" }
};

static const std::vector<FlagName> RTA_TABLE =
{
    { RTA_DST, "DST" }, { RTA_GATEWAY, "GATEWAY" }, { RTA_NETMASK, "NETMASK" }, { RTA_GENMASK, "GENMASK" },
    { RTA_IFP, "IFP" }, { RTA_IFA, "IFA" }, { RTA_AUTHOR, "AUTHOR" }, { RTA_BRD, "BRD" }
};

static std::string decodeFlags(long flags, const std::vector<FlagName>& table)
{
    std::string out;
    for(const auto& f : table)
    {
        if(flags & f.bit)
        {
            if(!out.empty())
                out += ",";
            out += f.name;
        }
    }
    return out.empty() ? "-" : out;
}

static std::string rtmTypeName(u_char type)
{
    switch(type)
    {
        case RTM_ADD:        return "RTM_ADD";
        case RTM_DELETE:     return "RTM_DELETE";
        case RTM_CHANGE:     return "RTM_CHANGE";
        case RTM_GET:        return "RTM_GET";
        case RTM_LOSING:     return "RTM_LOSING";
        case RTM_REDIRECT:   return "RTM_REDIRECT";
        case RTM_MISS:       return "RTM_MISS";
        case RTM_LOCK:       return "RTM_LOCK";
        case RTM_RESOLVE:    return "RTM_RESOLVE";
        case RTM_NEWADDR:    return "RTM_NEWADDR";
        case RTM_DELADDR:    return "RTM_DELADDR";
        case RTM_IFINFO:     return "RTM_IFINFO";
        case RTM_NEWMADDR:   return "RTM_NEWMADDR";
        case RTM_DELMADDR:   return "RTM_DELMADDR";
        case RTM_IFANNOUNCE: return "RTM_IFANNOUNCE";
        case RTM_IEEE80211:  return "RTM_IEEE80211";
        case RTM_IPFWLOG:    return "RTM_IPFWLOG";
        default:
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "UNKNOWN(0x%02x)", type);
            return buf;
        }
    }
}

static std::string macToString(const uint8_t mac[6])
{
    char buf[18];
    snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return buf;
}

static std::string ieee80211EventName(int what)
{
    switch(what)
    {
        case RTM_IEEE80211_ASSOC:    return "ASSOC";
        case RTM_IEEE80211_REASSOC:  return "REASSOC";
        case RTM_IEEE80211_DISASSOC: return "DISASSOC";
        case RTM_IEEE80211_JOIN:     return "JOIN";
        case RTM_IEEE80211_LEAVE:    return "LEAVE";
        case RTM_IEEE80211_SCAN:     return "SCAN";
        case RTM_IEEE80211_REPLAY:   return "REPLAY";
        case RTM_IEEE80211_MICHAEL:  return "MICHAEL";
        case RTM_IEEE80211_REJOIN:   return "REJOIN";
        case RTM_IEEE80211_WDS:      return "WDS";
        case RTM_IEEE80211_CSA:      return "CSA";
        case RTM_IEEE80211_RADAR:    return "RADAR";
        case RTM_IEEE80211_CAC:      return "CAC";
        case RTM_IEEE80211_DEAUTH:   return "DEAUTH";
        case RTM_IEEE80211_AUTH:     return "AUTH";
        case RTM_IEEE80211_COUNTRY:  return "COUNTRY";
        case RTM_IEEE80211_RADIO:    return "RADIO";
        default:
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "UNKNOWN(%d)", what);
            return buf;
        }
    }
}

// Mirrors enum ieee80211_notify_cac_event (sys/net80211/ieee80211_proto.h,
// a _KERNEL-only header) - redefined locally rather than pulled in, since
// that header isn't meant to be included from userspace.
static std::string cacEventName(uint8_t type)
{
    switch(type)
    {
        case 0:  return "START";
        case 1:  return "STOP";
        case 2:  return "RADAR";
        case 3:  return "EXPIRE";
        default: return "UNKNOWN(" + std::to_string(type) + ")";
    }
}

static std::string linkStateName(uint8_t state)
{
    switch(state)
    {
        case LINK_STATE_UNKNOWN: return "UNKNOWN";
        case LINK_STATE_DOWN:    return "DOWN";
        case LINK_STATE_UP:      return "UP";
        default:                 return "?";
    }
}

static std::string ifIndexToName(u_short index)
{
    char ifname[IFNAMSIZ] = {0};
    if(index > 0 && if_indextoname(index, ifname) != nullptr)
        return ifname;
    return "?";
}

static std::string sockaddrToString(const struct sockaddr* sa)
{
    char buf[128];

    switch(sa->sa_family)
    {
        case AF_INET:
        {
            const struct sockaddr_in* sin = reinterpret_cast<const struct sockaddr_in*>(sa);
            return inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf)) ? buf : "(invalid AF_INET)";
        }
        case AF_INET6:
        {
            const struct sockaddr_in6* sin6 = reinterpret_cast<const struct sockaddr_in6*>(sa);
            return inet_ntop(AF_INET6, &sin6->sin6_addr, buf, sizeof(buf)) ? buf : "(invalid AF_INET6)";
        }
        case AF_LINK:
        {
            const struct sockaddr_dl* sdl = reinterpret_cast<const struct sockaddr_dl*>(sa);
            std::string out = (sdl->sdl_nlen > 0) ? ("if=" + std::string(sdl->sdl_data, sdl->sdl_nlen))
                                                   : ("if=" + ifIndexToName(sdl->sdl_index));
            if(sdl->sdl_alen > 0)
            {
                char* mac = link_ntoa(sdl);
                if(mac != nullptr)
                    out += " addr=" + std::string(mac);
            }
            return out;
        }
        default:
            return "(family " + std::to_string(sa->sa_family) + ")";
    }
}

// Walks the sockaddr array that follows a routing message's fixed-size
// header, printing each one present according to addrsMask (RTA_*/RTAX_*
// bit per slot). Every routing-socket message carrying addresses uses this
// same encoding (see SA_SIZE in net/route.h), just with a different header
// size ahead of it.
static void printAddrs(const char* msgBase, size_t headerSize, long addrsMask, size_t msgLen)
{
    const char* p = msgBase + headerSize;
    const char* end = msgBase + msgLen;

    for(int i = 0; i < RTAX_MAX && p < end; i++)
    {
        if(!(addrsMask & (1 << i)))
            continue;

        const struct sockaddr* sa = reinterpret_cast<const struct sockaddr*>(p);
        std::string label;
        for(const auto& f : RTA_TABLE)
        {
            if(f.bit == (1 << i))
            {
                label = f.name;
                break;
            }
        }
        if(label.empty())
            label = "RTAX" + std::to_string(i);

        printf("    %-10s %s\n", label.c_str(), sockaddrToString(sa).c_str());

        size_t sz = SA_SIZE(sa);
        if(p + sz > end)
            break;
        p += sz;
    }
}

static void printTimestamp()
{
    char timebuf[16];
    time_t now = time(nullptr);
    struct tm tmnow;
    localtime_r(&now, &tmnow);
    strftime(timebuf, sizeof(timebuf), "%H:%M:%S", &tmnow);
    printf("[%s] ", timebuf);
}

static void decodeMessage(const char* buf, size_t n)
{
    if(n < 4)
    {
        printTimestamp();
        printf("short message (%zu bytes), ignored\n\n", n);
        return;
    }

    u_short msglen;
    memcpy(&msglen, buf, sizeof(msglen));
    u_char version = buf[2];
    u_char type = buf[3];

    printTimestamp();
    printf("%s  len=%u ver=%u\n", rtmTypeName(type).c_str(), msglen, version);

    switch(type)
    {
        case RTM_ADD:
        case RTM_DELETE:
        case RTM_CHANGE:
        case RTM_GET:
        case RTM_LOSING:
        case RTM_REDIRECT:
        case RTM_MISS:
        case RTM_LOCK:
        case RTM_RESOLVE:
        case RTM_IPFWLOG:
        {
            if(n < sizeof(struct rt_msghdr))
            {
                printf("  (truncated rt_msghdr)\n\n");
                return;
            }
            const struct rt_msghdr* rtm = reinterpret_cast<const struct rt_msghdr*>(buf);
            printf("  index=%u (%s) pid=%d seq=%d errno=%d\n",
                   rtm->rtm_index, ifIndexToName(rtm->rtm_index).c_str(),
                   (int)rtm->rtm_pid, rtm->rtm_seq, rtm->rtm_errno);
            printf("  flags=%s\n", decodeFlags(rtm->rtm_flags, RTF_TABLE).c_str());
            printf("  addrs=%s\n", decodeFlags(rtm->rtm_addrs, RTA_TABLE).c_str());
            printAddrs(buf, sizeof(struct rt_msghdr), rtm->rtm_addrs, n);
            break;
        }
        case RTM_IFINFO:
        {
            if(n < sizeof(struct if_msghdr))
            {
                printf("  (truncated if_msghdr)\n\n");
                return;
            }
            const struct if_msghdr* ifm = reinterpret_cast<const struct if_msghdr*>(buf);
            printf("  index=%u (%s)\n", ifm->ifm_index, ifIndexToName(ifm->ifm_index).c_str());
            printf("  flags=%s\n", decodeFlags(ifm->ifm_flags, IFF_TABLE).c_str());
            printf("  mtu=%u type=%u link_state=%s\n",
                   ifm->ifm_data.ifi_mtu, ifm->ifm_data.ifi_type,
                   linkStateName(ifm->ifm_data.ifi_link_state).c_str());
            printAddrs(buf, sizeof(struct if_msghdr), ifm->ifm_addrs, n);
            break;
        }
        case RTM_NEWADDR:
        case RTM_DELADDR:
        {
            if(n < sizeof(struct ifa_msghdr))
            {
                printf("  (truncated ifa_msghdr)\n\n");
                return;
            }
            const struct ifa_msghdr* ifam = reinterpret_cast<const struct ifa_msghdr*>(buf);
            printf("  index=%u (%s) metric=%d\n",
                   ifam->ifam_index, ifIndexToName(ifam->ifam_index).c_str(), ifam->ifam_metric);
            printf("  flags=%s\n", decodeFlags(ifam->ifam_flags, IFF_TABLE).c_str());
            printf("  addrs=%s\n", decodeFlags(ifam->ifam_addrs, RTA_TABLE).c_str());
            printAddrs(buf, sizeof(struct ifa_msghdr), ifam->ifam_addrs, n);
            break;
        }
        case RTM_NEWMADDR:
        case RTM_DELMADDR:
        {
            if(n < sizeof(struct ifma_msghdr))
            {
                printf("  (truncated ifma_msghdr)\n\n");
                return;
            }
            const struct ifma_msghdr* ifmam = reinterpret_cast<const struct ifma_msghdr*>(buf);
            printf("  index=%u (%s)\n", ifmam->ifmam_index, ifIndexToName(ifmam->ifmam_index).c_str());
            printf("  flags=%s\n", decodeFlags(ifmam->ifmam_flags, IFF_TABLE).c_str());
            printf("  addrs=%s\n", decodeFlags(ifmam->ifmam_addrs, RTA_TABLE).c_str());
            printAddrs(buf, sizeof(struct ifma_msghdr), ifmam->ifmam_addrs, n);
            break;
        }
        case RTM_IFANNOUNCE:
        {
            if(n < sizeof(struct if_announcemsghdr))
            {
                printf("  (truncated if_announcemsghdr)\n\n");
                return;
            }
            const struct if_announcemsghdr* ifan = reinterpret_cast<const struct if_announcemsghdr*>(buf);
            printf("  index=%u name=%s what=%s\n", ifan->ifan_index, ifan->ifan_name,
                   ifan->ifan_what == IFAN_ARRIVAL ? "ARRIVAL" :
                   (ifan->ifan_what == IFAN_DEPARTURE ? "DEPARTURE" : "UNKNOWN"));
            break;
        }
        case RTM_IEEE80211:
        {
            // Piggybacks on if_announcemsghdr (ifan_what carries the
            // RTM_IEEE80211_* sub-event instead of IFAN_ARRIVAL/DEPARTURE),
            // with an event-specific struct appended right after it - see
            // rt_ieee80211msg()/ieee80211_notify_*() in sys/net80211 and
            // sys/net/rtsock.c (verified against the FreeBSD 15.1 source
            // tree, matching this box's installed version).
            if(n < sizeof(struct if_announcemsghdr))
            {
                printf("  (truncated if_announcemsghdr)\n\n");
                return;
            }
            const struct if_announcemsghdr* ifan = reinterpret_cast<const struct if_announcemsghdr*>(buf);
            const char* payload = buf + sizeof(struct if_announcemsghdr);
            size_t payloadLen = n - sizeof(struct if_announcemsghdr);

            printf("  index=%u name=%s event=%s\n", ifan->ifan_index, ifan->ifan_name,
                   ieee80211EventName(ifan->ifan_what).c_str());

            switch(ifan->ifan_what)
            {
                case RTM_IEEE80211_ASSOC:
                case RTM_IEEE80211_REASSOC:
                case RTM_IEEE80211_JOIN:
                case RTM_IEEE80211_REJOIN:
                case RTM_IEEE80211_LEAVE:
                case RTM_IEEE80211_WDS:
                case RTM_IEEE80211_DEAUTH:
                case RTM_IEEE80211_AUTH:
                {
                    // ieee80211_join_event/leave_event/wds_event/auth_event/
                    // deauth_event are all the same {iev_addr[6]} layout -
                    // notify_macaddr() in ieee80211_freebsd.c builds every
                    // one of them identically regardless of which named
                    // struct the header declares for that event.
                    if(payloadLen < sizeof(struct ieee80211_join_event))
                    {
                        printf("  (truncated station-address event payload)\n");
                        break;
                    }
                    const auto* iev = reinterpret_cast<const struct ieee80211_join_event*>(payload);
                    printf("  addr=%s\n", macToString(iev->iev_addr).c_str());
                    break;
                }
                case RTM_IEEE80211_DISASSOC:
                case RTM_IEEE80211_SCAN:
                    // No payload for these two (rt_ieee80211msg() called with data_len=0).
                    break;
                case RTM_IEEE80211_REPLAY:
                {
                    if(payloadLen < sizeof(struct ieee80211_replay_event))
                    {
                        printf("  (truncated replay event payload)\n");
                        break;
                    }
                    const auto* iev = reinterpret_cast<const struct ieee80211_replay_event*>(payload);
                    printf("  src=%s dst=%s cipher=%u keyix=%u key_rsc=%llu frame_rsc=%llu\n",
                           macToString(iev->iev_src).c_str(), macToString(iev->iev_dst).c_str(),
                           iev->iev_cipher, iev->iev_keyix,
                           (unsigned long long)iev->iev_keyrsc, (unsigned long long)iev->iev_rsc);
                    break;
                }
                case RTM_IEEE80211_MICHAEL:
                {
                    if(payloadLen < sizeof(struct ieee80211_michael_event))
                    {
                        printf("  (truncated michael event payload)\n");
                        break;
                    }
                    const auto* iev = reinterpret_cast<const struct ieee80211_michael_event*>(payload);
                    printf("  src=%s dst=%s cipher=%u keyix=%u\n",
                           macToString(iev->iev_src).c_str(), macToString(iev->iev_dst).c_str(),
                           iev->iev_cipher, iev->iev_keyix);
                    break;
                }
                case RTM_IEEE80211_CSA:
                {
                    if(payloadLen < sizeof(struct ieee80211_csa_event))
                    {
                        printf("  (truncated csa event payload)\n");
                        break;
                    }
                    const auto* iev = reinterpret_cast<const struct ieee80211_csa_event*>(payload);
                    printf("  flags=0x%x freq=%u ieee_chan=%u mode=%u count=%u\n",
                           iev->iev_flags, iev->iev_freq, iev->iev_ieee, iev->iev_mode, iev->iev_count);
                    break;
                }
                case RTM_IEEE80211_RADAR:
                {
                    if(payloadLen < sizeof(struct ieee80211_radar_event))
                    {
                        printf("  (truncated radar event payload)\n");
                        break;
                    }
                    const auto* iev = reinterpret_cast<const struct ieee80211_radar_event*>(payload);
                    printf("  flags=0x%x freq=%u ieee_chan=%u\n", iev->iev_flags, iev->iev_freq, iev->iev_ieee);
                    break;
                }
                case RTM_IEEE80211_CAC:
                {
                    if(payloadLen < sizeof(struct ieee80211_cac_event))
                    {
                        printf("  (truncated cac event payload)\n");
                        break;
                    }
                    const auto* iev = reinterpret_cast<const struct ieee80211_cac_event*>(payload);
                    printf("  flags=0x%x freq=%u ieee_chan=%u type=%s\n",
                           iev->iev_flags, iev->iev_freq, iev->iev_ieee, cacEventName(iev->iev_type).c_str());
                    break;
                }
                case RTM_IEEE80211_COUNTRY:
                {
                    if(payloadLen < sizeof(struct ieee80211_country_event))
                    {
                        printf("  (truncated country event payload)\n");
                        break;
                    }
                    const auto* iev = reinterpret_cast<const struct ieee80211_country_event*>(payload);
                    printf("  addr=%s cc=%c%c\n", macToString(iev->iev_addr).c_str(), iev->iev_cc[0], iev->iev_cc[1]);
                    break;
                }
                case RTM_IEEE80211_RADIO:
                {
                    if(payloadLen < sizeof(struct ieee80211_radio_event))
                    {
                        printf("  (truncated radio event payload)\n");
                        break;
                    }
                    const auto* iev = reinterpret_cast<const struct ieee80211_radio_event*>(payload);
                    printf("  state=%s\n", iev->iev_state ? "ON" : "OFF");
                    break;
                }
                default:
                    printf("  (unknown ieee80211 sub-event, %zu bytes of payload not decoded)\n", payloadLen);
                    break;
            }
            break;
        }
        default:
            printf("  (unhandled message type, %zu bytes total)\n", n);
            break;
    }

    printf("\n");
    fflush(stdout);
}

int main()
{
    int sock = socket(PF_ROUTE, SOCK_RAW, 0);
    if(sock < 0)
    {
        perror("socket(PF_ROUTE, SOCK_RAW, 0)");
        return 1;
    }

    printf("rtskmon: listening on routing socket (pid=%d)\n\n", getpid());
    fflush(stdout);

    std::vector<char> buf(4096);

    while(true)
    {
        ssize_t n = read(sock, buf.data(), buf.size());
        if(n < 0)
        {
            if(errno == EINTR)
                continue;
            perror("read");
            break;
        }
        if(n == 0)
            continue;
        decodeMessage(buf.data(), (size_t)n);
    }

    close(sock);
    return 0;
}
