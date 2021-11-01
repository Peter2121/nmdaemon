#ifndef INTERFACE_H
#define INTERFACE_H

#include <string>
#include <exception>
#include <memory>
#define LOGURU_WITH_STREAMS 1
#include "loguru/loguru.hpp"
#include "addr.h"

class interface
{
protected:
    std::string strName;
    std::vector<std::shared_ptr<addr>> spVectAddrs;
    bool hasIPv4;
    bool hasIPv6;
    bool isIfUp;
public:
    interface();
    interface(std::string);
    void setName(std::string);
    void addAddress(struct ifaddrs*);
    void addAddress(std::shared_ptr<addr>);
    std::string getName() const;
    const nlohmann::json getIfJson() const;
    const nlohmann::json getIfString() const;
    ~interface();
};

#endif // INTERFACE_H
