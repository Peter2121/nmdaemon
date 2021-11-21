#ifndef IPADDR_H
#define IPADDR_H

#include <ifaddrs.h>
#include <net/if.h>
#include <net/route.h>
#include <memory>
#include "json/json.hpp"
#include "magic_enum/magic_enum.hpp"
#include "nmjsonconst.h"
#include "address_base.h"
#include "address_ip4.h"
#include "address_ip6.h"
#include "address_link.h"
#include "nmexception.h"
#include "tool.h"

enum class ipaddr_type
{
    BCAST, PPP, LINK, LOOPBACK, ROUTE, UNKNOWN
};

class addr
{
protected:
    ipaddr_type ipType;
    std::shared_ptr<address_base> spIpAddress;
    std::shared_ptr<address_base> spIpMask;
    std::shared_ptr<address_base> spIpData;
    int flags;
    bool isAddrUp;
    nmexception nmExcept;
    bool isValidIp4() const;
    bool isValidIp6() const;
public:
    addr(struct ifaddrs*);
    addr(std::shared_ptr<address_base> addr, std::shared_ptr<address_base> mask, std::shared_ptr<address_base> data=nullptr, ipaddr_type type=ipaddr_type::BCAST, bool up=false, int fl=0);
    addr();
    ~addr();
    const std::shared_ptr<address_base> getAddr() const;
    const std::shared_ptr<address_base> getMask() const;
    const std::shared_ptr<address_base> getData() const;
    const address_base* getAddrAB() const;
    const address_base* getMaskAB() const;
    const address_base* getDataAB() const;
    const std::string getAddrString() const;
    const nlohmann::json getAddrJson() const;
    bool isUp() const;
    short getFamily() const;
    void setData(std::shared_ptr<address_base>);
    bool isValidIp() const;
    int getFlags() const;
    void setFlags(int);
    const std::vector<std::string> getFlagsRoute() const;
};

#endif // IPADDR_H
