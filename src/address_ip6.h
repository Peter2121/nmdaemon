#ifndef ADDRESS_IP6_H
#define ADDRESS_IP6_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "address_base.h"

class address_ip6 : public address_base
{
protected:
    struct in6_addr ip_addr6;
    const short family = AF_INET6;
    struct sockaddr_storage* sock_addr;
    std::string strAddr;
    nmexception nmExcept;
    void setIpAddr6();
public:
    address_ip6(const struct sockaddr_in6*);
    address_ip6(std::string);
    ~address_ip6();
    std::string getStrAddr() const;
    const struct sockaddr_storage* getSockAddr() const;
    short getFamily() const;
    bool operator==(const address_base&);
    bool operator!=(const address_base&);
};

#endif // ADDRESS_IP6_H
