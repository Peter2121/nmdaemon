#ifndef ADDRESS_IP4_H
#define ADDRESS_IP4_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "addressbase.h"

class address_ip4 : public AddressBase
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
    address_ip4() : address_ip4("0.0.0.0") {}
    ~address_ip4();
    std::string getStrAddr() const;
    const struct sockaddr_storage* getSockAddr() const;
    short getFamily() const;
    bool operator==(const AddressBase&);
    bool operator!=(const AddressBase&);
};

#endif // ADDRESS_IP4_H
