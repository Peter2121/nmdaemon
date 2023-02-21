#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/jail.h>
#include <sys/socket.h>
#include <sys/sysctl.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <err.h>
#include <errno.h>
#include <jail.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <vector>
#include "json/json.hpp"
#include "loguru/loguru.hpp"
#include "nmjsonconst.h"

#define INET
#define INET6

#ifndef JAILPARAM_H
#define JAILPARAM_H

class JailParam {
public:
    JailParam();
    ~JailParam() {}

    void Print();
    int GetJailParams(int jflags);
    nlohmann::json GetJailJson();
    static void InitRequestParams();

    static int Lastjid;

protected:
    bool Ip4Ok;
    bool Ip6Ok;
    int Id;
    std::string Name;
    std::vector<std::string> Ipv4Addresses;
    std::vector<std::string> Ipv6Addresses;
    std::string HostName;
    std::string Path;
    static std::vector<jailparam> Params;
    static constexpr unsigned int JP_USER = 0x01000000;
    static constexpr unsigned int JP_OPT = 0x02000000;
    static inline const std::string ParamNames[] = { "jid", "name", "host.hostname", "path" };
    static inline const std::string ParamIpv4 = "ip4.addr";
    static inline const std::string ParamIpv6 = "ip6.addr";
    static inline const std::string ParamLastJid = "lastjid";

    void PutIpAddrToList(int af_family, struct jailparam *param);
    static int AddParam(const char *name, void *value, size_t valuelen, struct jailparam *source, unsigned flags);

};

#endif // JAILPARAM_H
