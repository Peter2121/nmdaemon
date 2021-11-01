#ifndef IPADDR_H
#define IPADDR_H

#include <ifaddrs.h>
#include <net/if.h>
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
    BCAST, PPP, LINK, LOOPBACK, ROUTE
};

class addr
{
protected:
    ipaddr_type ipType;
    address_base* ipAddress;
    std::shared_ptr<address_base> spIpAddress;
    bool memAddr;
    address_base* ipMask;    // subnet mask or nothing
    std::shared_ptr<address_base> spIpMask;
    bool memMask;
    address_base* ipData;    // network broadcast or gateway or nothing
    std::shared_ptr<address_base> spIpData;
    bool memData;
    bool isAddrUp;
    nmexception nmExcept;
    bool isValidIp4() const;
    bool isValidIp6() const;
public:
    addr(struct ifaddrs*);
    addr(address_base* addr, address_base* mask, address_base* data=nullptr, ipaddr_type type=ipaddr_type::BCAST, bool up=false, bool mm=false);
    addr(std::shared_ptr<address_base> addr, std::shared_ptr<address_base> mask, std::shared_ptr<address_base> data=nullptr, ipaddr_type type=ipaddr_type::BCAST, bool up=false);
    ~addr();
    const address_base* getAddrAB() const;
    const address_base* getMaskAB() const;
    const address_base* getDataAB() const;
    const std::string getAddrString() const;
    const nlohmann::json getAddrJson() const;
    bool isUp() const;
    short getFamily() const;
//    void setAddr(address_base*, bool);
//    void setMask(address_base*, bool);
//    void setData(address_base*, bool);
    void setData(std::shared_ptr<address_base>);
    bool isValidIp() const;
};

#endif // IPADDR_H
