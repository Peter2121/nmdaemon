#ifndef IPADDR_H
#define IPADDR_H

#include <ifaddrs.h>
#include <net/if.h>
#include <net/route.h>
#include <memory>
#include "json/json.hpp"
#include "magic_enum/magic_enum.hpp"
#include "nmjsonconst.h"
#include "addressbase.h"
#include "addressip4.h"
#include "addressip6.h"
#include "addresslink.h"
#include "nmexception.h"
#include "tool.h"

enum class AddressGroupType
{
    BCAST, PPP, LINK, LOOPBACK, ROUTE, UNKNOWN
};

class AddressGroup
{
protected:
    AddressGroupType ipType;
    std::shared_ptr<AddressBase> spIpAddress;
    std::shared_ptr<AddressBase> spIpMask;
    std::shared_ptr<AddressBase> spIpData;
    int flags;
    bool isAddrUp;
    bool isAddrRunning;
    bool isAddrPrimary;
    NmException nmExcept;
    bool isValidIp4() const;
    bool isValidIp6() const;
public:
    AddressGroup(struct ifaddrs*);
    AddressGroup(std::shared_ptr<AddressBase> addr, std::shared_ptr<AddressBase> mask, std::shared_ptr<AddressBase> data=nullptr, AddressGroupType type=AddressGroupType::BCAST, bool up=false, int fl=0, bool primary=false, bool running=false);
    AddressGroup();
    ~AddressGroup();
    const std::shared_ptr<AddressBase> getAddr() const;
    const std::shared_ptr<AddressBase> getMask() const;
    const std::shared_ptr<AddressBase> getData() const;
    const AddressBase* getAddrAB() const;
    const AddressBase* getMaskAB() const;
    const AddressBase* getDataAB() const;
    const std::string getAddrString() const;
    const nlohmann::json getAddrJson() const;
    bool isUp() const;
    bool isRunning() const;
    bool isPrimary() const;
    short getFamily() const;
    void setAddr(std::shared_ptr<AddressBase>);
    void setMask(std::shared_ptr<AddressBase>);
    void setData(std::shared_ptr<AddressBase>);
    void setType(AddressGroupType);
    bool isValidIp() const;
    int getFlags() const;
    void setFlags(int);
    const std::vector<std::string> getFlagsRoute() const;
};

#endif // IPADDR_H
