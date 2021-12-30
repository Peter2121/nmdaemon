#ifndef ADDR_LINK_H
#define ADDR_LINK_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if_dl.h>
#include <arpa/inet.h>
#include "addressbase.h"

class address_link : public AddressBase
{
protected:
    const short family = AF_LINK;
    struct sockaddr_storage* sock_addr;
    std::string strAddr;
    nmexception nmExcept;
    void setLinkAddr();
public:
    address_link(const struct sockaddr_dl*);
    address_link(std::string);
    address_link() : address_link("00:00:00:00:00:00") {}
    ~address_link();
    std::string getStrAddr() const;
    const struct sockaddr_storage* getSockAddr() const;
    short getFamily() const;
    bool operator==(const AddressBase&);
    bool operator!=(const AddressBase&);
};

#endif // ADDR_LINK_H
