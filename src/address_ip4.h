#ifndef ADDRESS_IP4_H
#define ADDRESS_IP4_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "address_base.h"

class address_ip4 : public address_base
{
protected:
    struct in_addr ip_addr;
    const short family = AF_INET;
    struct sockaddr_storage* sock_addr;
    std::string strAddr;
    nmexception nmExcept;
    void setIpAddr();
public:
    address_ip4(const struct sockaddr_in*);
    address_ip4(std::string);
    ~address_ip4();
    std::string getStrAddr() const;
    const struct sockaddr_storage* getSockAddr() const;
    short getFamily() const;
    bool operator==(const address_base&);
    bool operator!=(const address_base&);
};

#endif // ADDRESS_IP4_H
