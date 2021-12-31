#ifndef ADDR_LINK_H
#define ADDR_LINK_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if_dl.h>
#include <arpa/inet.h>
#include "addressbase.h"

class AddressLink : public AddressBase
{
protected:
    const short family = AF_LINK;
    struct sockaddr_storage* sock_addr;
    std::string strAddr;
    NmException nmExcept;
    void setLinkAddr();
public:
    AddressLink(const struct sockaddr_dl*);
    AddressLink(std::string);
    AddressLink() : AddressLink("00:00:00:00:00:00") {}
    ~AddressLink();
    std::string getStrAddr() const;
    const struct sockaddr_storage* getSockAddr() const;
    short getFamily() const;
    bool operator==(const AddressBase&);
    bool operator!=(const AddressBase&);
};

#endif // ADDR_LINK_H
