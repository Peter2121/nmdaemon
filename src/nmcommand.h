#ifndef NMCOMMAND_H
#define NMCOMMAND_H

enum class NmScope
{
    NONE, DUMMY, SYSTEM, INTERFACE, ROUTE, WPA
};

enum class NmCmd
{
    NONE, TEST,
//  system_worker
    IF_ADD, IF_REMOVE, IF_LIST,
    IF_ENABLE, IF_DISABLE,
    RCCONF_READ, RCCONF_WRITE,
//  if_worker
    IP4_ADDR_GET, IP6_ADDR_GET, // Returns only primary IP
    IP_ADDR_SET, IP4_DHCP_ENABLE, IP6_DHCP_ENABLE,
    IP_ADDR_ADD, IP_ADDR_REMOVE,
    MTU_GET, MTU_SET, MAC_ADDR_GET, MAC_ADDR_SET,
//  route_worker
    RT_GET, RT_DEF_GET, RT_DEF6_GET, RT_SET, RT_DEF_SET, RT_DEL, RT_DEF_DEL, RT_LIST, RT_LIST6,
//  wifi_worker
    WPA_LIST_IF, WPA_LIST, WPA_SCAN, WPA_STATUS, WPA_SETPSK, WPA_CONNECT, WPA_DISCONNECT, WPA_REASSOC, WPA_ADD, WPA_REMOVE
};

struct NmCommand
{
    NmScope scope;
    NmCmd cmd;
};

#endif // NMCOMMAND_H
