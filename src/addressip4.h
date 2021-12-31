#ifndef ADDRESSIP4_H
#define ADDRESSIP4_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "addressbase.h"

class AddressIp4 : public AddressBase
{
protected:
    struct in_addr ip_addr;
    const short family = AF_INET;
    struct sockaddr_storage* sock_addr;
    std::string strAddr;
    NmException nmExcept;
    void setIpAddr();
public:
    AddressIp4(const struct sockaddr_in*);
    AddressIp4(std::string);
    AddressIp4() : AddressIp4("0.0.0.0") {}
    ~AddressIp4();
    std::string getStrAddr() const;
    const struct sockaddr_storage* getSockAddr() const;
    short getFamily() const;
    bool operator==(const AddressBase&);
    bool operator!=(const AddressBase&);
};

#endif // ADDRESSIP4_H
