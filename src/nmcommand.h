#ifndef NMCOMMAND_H
#define NMCOMMAND_H

enum class NmScope
{
    NONE, DUMMY, SYSTEM, INTERFACE, ROUTE, WPA, WIFI
};

enum class NmCmd
{
    NONE, TEST,
//  Cross-cutting: supported by multiple scopes' workers (see each worker's
//  Cmds[]), not tied to one section below.
    SUBSCRIBE, UNSUBSCRIBE,
//  Cross-cutting notification-only EVENT values: sourced from more than one
//  worker's own PfRouteMonitor (each decoding the same kernel message type
//  independently, for its own scope's subscribers - see each worker's
//  routeMonitor member), never dispatched as a command, no case for any of
//  these exists in any worker's execCmd(). Exist because nmdaemon cannot
//  reliably tell whether a given PF_ROUTE message was caused by one of its
//  own commands or by something external; these are reported uniformly
//  regardless of cause, distinct from the command-triggered EVENT values
//  (IF_EDIT/IF_ADD/IF_ENABLE/etc.) so a client can tell the two sources
//  apart.
    IF_INFO_CHANGED,    // RTM_IFINFO: flags/MTU/link state changed (which field, if any in particular, is NOT knowable from this alone) - SYSTEM (IF_ENABLE/DISABLE) and INTERFACE (MTU_SET/MAC_ADDR_SET) scopes both watch for this
    IF_ARRIVED,         // RTM_IFANNOUNCE / IFAN_ARRIVAL - SYSTEM (IF_ADD) and INTERFACE (IF_RENAME) scopes both watch for this
    IF_DEPARTED,        // RTM_IFANNOUNCE / IFAN_DEPARTURE - ditto (IF_REMOVE / IF_RENAME)
//  system_worker
    IF_ADD, IF_REMOVE, IF_LIST,
    IF_ENABLE, IF_DISABLE,
    RCCONF_READ, RCCONF_WRITE,
    JAIL_LIST, RESOLVCONF_READ, RESOLVCONF_WRITE,
//  if_worker
    IF_EDIT, IF_GET, IF_RENAME,
    IP4_ADDR_GET, IP6_ADDR_GET, // Returns only primary IP
    IP_ADDR_SET,
    IP4_DHCP_ENABLE, IP6_DHCP_ENABLE,
    IP4_DHCP_DISABLE, IP6_DHCP_DISABLE,
    IP4_GET_DHCP_STATUS, IP6_GET_DHCP_STATUS,
    IP_ADDR_ADD, IP_ADDR_REMOVE,
    MTU_GET, MTU_SET, MAC_ADDR_GET, MAC_ADDR_SET,
//  Notification-only, same rationale as the cross-cutting block above, but
//  INTERFACE-only: only IP_ADDR_SET/ADD/REMOVE relate to interface
//  addresses, no SYSTEM-scope command does.
    IF_ADDR_ADDED,      // RTM_NEWADDR
    IF_ADDR_REMOVED,    // RTM_DELADDR
//  route_worker
    RT_GET, RT_DEF_GET, RT_DEF6_GET, RT_SET, RT_DEF_SET, RT_DEL, RT_DEF_DEL, RT_LIST, RT_LIST6,
//  Notification-only, same rationale as the cross-cutting block above, but
//  ROUTE-only: sourced from NmWorkerRoute's own PfRouteMonitor, decoding
//  the same RTM_ADD/DELETE/CHANGE messages RT_SET/RT_DEL/RT_DEF_SET/
//  RT_DEF_DEL themselves cause (plus anything external that causes them).
    RT_ADDED,   // RTM_ADD
    RT_REMOVED, // RTM_DELETE
    RT_CHANGED, // RTM_CHANGE (e.g. setStaticRoute()'s own ADD->CHANGE fallback on EEXIST)
//  wpa_worker
    WPA_LIST_IF, WPA_LIST, WPA_LIST_ASYNC, WPA_LIST_RESULTS, WPA_SCAN, WPA_SCAN_ASYNC, WPA_SCAN_RESULTS, WPA_STATUS, WPA_SETPSK, WPA_SETPARAM, WPA_CONNECT,
    WPA_CONNECT_ASYNC,
    WPA_DISCONNECT, WPA_REASSOC, WPA_ADD, WPA_REMOVE, WPA_RESET, WPA_SAVE, WPA_ENABLE, WPA_DISABLE, WPA_SET_BSSID, WPA_SELECT,
//  IEEE80211 worker
    WIFI_SCAN, WIFI_SCAN_RESULTS, WIFI_STATUS,
//  Notification-only EVENT values for WIFI-scope subscribers, one per
//  RTM_IEEE80211_* kernel sub-event (see NmWorkerIeee80211's routing-socket
//  monitor thread) - never dispatched as a command, no case for any of
//  these exists in any worker's execCmd().
    WIFI_STA_ASSOC, WIFI_STA_REASSOC, WIFI_STA_DISASSOC,
    WIFI_STA_JOIN, WIFI_STA_REJOIN, WIFI_STA_LEAVE,
    WIFI_SCAN_DONE, WIFI_REPLAY_FAILURE, WIFI_MICHAEL_FAILURE,
    WIFI_WDS_DISCOVER, WIFI_CSA, WIFI_RADAR, WIFI_CAC,
    WIFI_STA_DEAUTH, WIFI_STA_AUTH, WIFI_COUNTRY, WIFI_RADIO
};

struct NmCommand
{
    NmScope scope;
    NmCmd cmd;
};

#endif // NMCOMMAND_H
