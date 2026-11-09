#ifndef INTERFACE_H
#define INTERFACE_H

#include <string>
#include <exception>
#include <memory>
#include <algorithm>
#include <map>
//#define LOGURU_WITH_STREAMS 1
#include "loguru/loguru.hpp"
#include "addressgroup.h"
#include "mediastatus.h"
#include "jailparam.h"

class Interface
{
protected:
    std::string strName;
    std::vector<std::shared_ptr<AddressGroup>> spVectAddrs;
    bool hasIPv4;
    bool hasIPv6;
    bool isIfUp;
    bool isIfRunning;
    bool isDhcpEnabled;
    MediaStatus ifStatus;
    std::string mediaDesc;
    std::string driverName;
    std::string description;
    std::string hardwareAddress;
    bool isBridge;
    std::vector<std::string> bridgeMembers;
    bool isLagg;
    std::vector<std::string> laggMembers;
    std::string laggProto;
    std::string laggMaster;
    // Driver-reported on/off status of individual IFCAP_* capabilities (e.g.
    // JSON_PARAM_TXCSUM_IPV4), keyed by the JSON field name that reports it.
    // Only capabilities the driver actually supports are present here, so new
    // ones can be added later (see Tool::getIfInfoJson()) just by adding an
    // entry to the {capbit, JSON key} table there -- no other code changes.
    std::map<std::string, bool> capStatus;
public:
    Interface(std::string);
    Interface() : Interface("") {}
    void setName(std::string);
    void addAddress(struct ifaddrs*);
    void addAddress(std::shared_ptr<AddressGroup>);
    std::string getName() const;
    const nlohmann::json getIfJson() const;
    const nlohmann::json getIfString() const;
    MediaStatus getStatus() const;
    void setStatus(MediaStatus);
    void setDhcpStatus(bool);
    bool getDhcpStatus() const;
    void setBridgeMembers(const std::vector<std::string>&);
    void setLaggMembers(const std::vector<std::string>&);
    void setLaggProto(const std::string&);
    void setLaggMaster(const std::string&);
    void setCapStatus(const std::string& jsonKey, bool enabled);
    ~Interface();
    const std::string &getMediaDesc() const;
    void setMediaDesc(const std::string &newMediaDesc);
    const std::string &getDriverName() const;
    void setDriverName(const std::string &newDriverName);
    const std::string &getDescription() const;
    void setDescription(const std::string &newDescription);
    void setHardwareAddress(const std::string &newHardwareAddress);
    void findPrimaryAndJailAddress();
};

#endif // INTERFACE_H
