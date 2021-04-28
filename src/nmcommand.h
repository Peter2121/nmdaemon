#ifndef NMCOMMAND_H
#define NMCOMMAND_H

enum class nmscope
{
    NONE, DUMMY, SYSTEM, INTERFACE, ROUTE, WPA
};

enum class nmcmd
{
    NONE, TEST,
//  system_worker
    IF_ADD, IF_REMOVE, IF_LIST,
    IF_ENABLE, IF_DISABLE,
//  if_worker
    IP_ADDR_SET, IP4_DHCP_ENABLE, IP6_DHCP_ENABLE,
    IP_ADDR_ADD, IP_ADDR_REMOVE,
    MTU_GET, MTU_SET, MAC_ADDR_GET, MAC_ADDR_SET,
//  route_worker
    RT_GET, RT_DEF_GET, RT_DEF6_GET, RT_SET, RT_DEF_SET, RT_REMOVE, RT_DEF_REMOVE, RT_LIST,
//  wifi_worker
    WPA_LIST_IF, WPA_LIST, WPA_SCAN, WPA_STATUS, WPA_SETPSK, WPA_CONNECT, WPA_DISCONNECT, WPA_REASSOC, WPA_ADD, WPA_REMOVE
};

struct nmcommand
{
    nmscope scope;
    nmcmd cmd;
};

#endif // NMCOMMAND_H
