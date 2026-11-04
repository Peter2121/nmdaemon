#ifndef INTERFACE_H
#define INTERFACE_H

#include <string>
#include <exception>
#include <memory>
#include <algorithm>
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
    bool isBridge;
    std::vector<std::string> bridgeMembers;
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
    ~Interface();
    const std::string &getMediaDesc() const;
    void setMediaDesc(const std::string &newMediaDesc);
    const std::string &getDriverName() const;
    void setDriverName(const std::string &newDriverName);
    const std::string &getDescription() const;
    void setDescription(const std::string &newDescription);
    void findPrimaryAndJailAddress();
};

#endif // INTERFACE_H
