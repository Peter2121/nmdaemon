#ifndef ADDRESSIP6_H
#define ADDRESSIP6_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "addressbase.h"

class AddressIp6 : public AddressBase
{
protected:
    struct in6_addr ip_addr6;
    const short family = AF_INET6;
    struct sockaddr_storage* sock_addr;
    std::string strAddr;
    nmexception nmExcept;
    void setIpAddr6();
public:
    AddressIp6(const struct sockaddr_in6*);
    AddressIp6(std::string);
    AddressIp6() : AddressIp6("0:0:0:0:0:0:0:0") {}
    ~AddressIp6();
    std::string getStrAddr() const;
    const struct sockaddr_storage* getSockAddr() const;
    short getFamily() const;
    bool operator==(const AddressBase&);
    bool operator!=(const AddressBase&);
};

#endif // ADDRESSIP6_H
