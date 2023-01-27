#ifndef INTERFACE_H
#define INTERFACE_H

#include <string>
#include <exception>
#include <memory>
//#define LOGURU_WITH_STREAMS 1
#include "loguru/loguru.hpp"
#include "addressgroup.h"
#include "mediastatus.h"

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
    ~Interface();
    const std::string &getMediaDesc() const;
    void setMediaDesc(const std::string &newMediaDesc);
};

#endif // INTERFACE_H
