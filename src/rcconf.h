#ifndef RCCONF_H
#define RCCONF_H

#include <string>
#include <stdlib.h>
#include <map>
#include <memory>
#include <filesystem>
#include <cstdio>
#include <arpa/inet.h>
//#define LOGURU_WITH_STREAMS 1
#include "loguru/loguru.hpp"
#include "json/json.hpp"
#include "inifile/inifile.h"
#include "nmjsonconst.h"
#include "addressgroup.h"
#include "tool.h"

using json = nlohmann::json;

class RcConf
{
protected:
    static inline const std::string IFCONFIG_KEY_PREFIX = "ifconfig_";
    static inline const std::string DEFAULT_ROUTE_KEY = "defaultrouter";
    static inline const std::string ROUTES_KEY = "static_routes";
    static inline const std::string ROUTE_KEY_PREFIX = "route_";
    static inline const std::string INET_ADDR = "inet";
    static inline const std::string INET_MASK = "netmask";
    static inline const std::string DHCP_SUFFIX = "DHCP";
    static inline const std::string ALIASES_SUFFIX = "aliases";
    static inline const std::string ALIAS_SUFFIX = "alias";
    static inline const std::string FILE_VERSIONS_DELIMITER = ".";
    static inline const std::string IPV4_MASK_HOST = "255.255.255.255";
    static inline const std::string ROUTE_PREFIX_HOST = "-host";
    static inline const std::string ROUTE_PREFIX_NET = "-net";
    // sscanf() format strings used by getIpConfFromString()/getRouteConfFromString(),
    // named after what they match rather than left as anonymous locals.
    // %u (not %d) matches the unsigned int* arguments passed at each call site.
    static inline const std::string IF_FORMAT_CIDR = "inet %3u.%3u.%3u.%3u/%2u";                       // "inet 127.0.0.251/24"
    // These two pairs share a "netmask"/"-netmask" prefix and differ only in how the
    // trailing token is read, so an out-of-range dotted mask can otherwise be accepted
    // as a partial match by the hex form (sscanf doesn't require consuming the whole
    // string). The trailing %n + a consumed-length check at the call site rejects that.
    static inline const std::string IF_FORMAT_NETMASK_DOTTED = "inet %3u.%3u.%3u.%3u netmask %3u.%3u.%3u.%3u%n"; // "inet 127.0.0.251 netmask 255.255.255.0"
    static inline const std::string IF_FORMAT_NETMASK_HEX = "inet %3u.%3u.%3u.%3u netmask %10x%n";        // "inet 127.0.0.251 netmask 0xffffff00"
    static inline const std::string RT_FORMAT_NET_CIDR = ROUTE_PREFIX_NET + " %3u.%3u.%3u.%3u/%2u %3u.%3u.%3u.%3u"; // "-net 192.168.32.0/24 192.168.213.252"
    static inline const std::string RT_FORMAT_NET_DOTTED = ROUTE_PREFIX_NET + " %3u.%3u.%3u.%3u -netmask %3u.%3u.%3u.%3u %3u.%3u.%3u.%3u%n"; // "-net 192.168.32.0 -netmask 255.255.255.0 192.168.213.252"
    static inline const std::string RT_FORMAT_NET_HEX = ROUTE_PREFIX_NET + " %3u.%3u.%3u.%3u -netmask %10x %3u.%3u.%3u.%3u%n"; // "-net 192.168.32.0 -netmask 0xffffff00 192.168.213.252"
    static inline const std::string RT_FORMAT_HOST = ROUTE_PREFIX_HOST + " %3u.%3u.%3u.%3u %3u.%3u.%3u.%3u"; // "-host 191.1.200.4 192.168.213.254"
    std::string rcFileName;
    std::unique_ptr<CIniFile> rcIniFile;
    std::string getStrInetMaskFromPrefix(int);
    static std::string ipv4FromOctets(unsigned int a, unsigned int b, unsigned int c, unsigned int d);
    static bool validOctets(unsigned int a, unsigned int b, unsigned int c, unsigned int d);
    // "em0_101" (rc.conf key form) <-> "em0.101" (display form) for VLAN sub-interfaces.
    // Each returns its argument unchanged if it doesn't look like a VLAN name (no
    // trailing "_"/"." followed by a positive number), so they're safe to call
    // unconditionally and stay each other's exact inverse.
    static std::string vlanConfigNameToDisplay(const std::string& ifname);
    static std::string vlanDisplayNameToConfig(const std::string& ifname);
    json getIpConfFromString(std::string);
    json getRouteConfFromString(std::string);
    short nBackups;
public:
    RcConf(std::string, short);
    ~RcConf();
    bool iniLoad();
    bool iniSave();
    bool rotateRcConfFile();
    json getRcIpConfig();
    bool setRcIpConfig(json);
};

#endif // RCCONF_H
