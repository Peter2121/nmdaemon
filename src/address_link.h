#ifndef ADDR_LINK_H
#define ADDR_LINK_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if_dl.h>
#include <arpa/inet.h>
#include "address_base.h"

class address_link : public address_base
{
protected:
    const short family = AF_LINK;
    struct sockaddr_storage* sock_addr;
    std::string strAddr;
    void setLinkAddr();
public:
    address_link(const struct sockaddr_dl*);
    address_link(std::string);
    ~address_link();
    std::string getStrAddr() const;
    const struct sockaddr_storage* getSockAddr() const;
    short getFamily() const;
    bool operator==(const address_base&);
    bool operator!=(const address_base&);
};

#endif // ADDR_LINK_H
