#include "address_link.h"

address_link::address_link(const struct sockaddr_dl* psa)
{
    sock_addr = new struct sockaddr_storage;
    memset(sock_addr,0,sizeof(struct sockaddr_storage));
    memcpy(sock_addr, psa, sizeof(struct sockaddr_dl));
    setLinkAddr();
}

address_link::address_link(std::string str_addr)
{
    const char* chr_ptr = str_addr.c_str();
    sock_addr = new struct sockaddr_storage;
    memset(sock_addr, 0, sizeof(struct sockaddr_storage));

    link_addr(chr_ptr, (struct sockaddr_dl*)sock_addr); // The function does not return any diagnostic info
    strAddr = str_addr;
}

address_link::~address_link()
{
    delete sock_addr;
}

void address_link::setLinkAddr()
{
    char* addr_ptr = 0;

    if (((struct sockaddr*)sock_addr)->sa_family == family)
    {
        addr_ptr = link_ntoa((struct sockaddr_dl*)sock_addr);
        strAddr = std::string(addr_ptr);
    }
    else
    {
        throw nmExcept;
    }
}

std::string address_link::getStrAddr() const
{
    return strAddr;
}

const struct sockaddr_storage* address_link::getSockAddr() const
{
    return sock_addr;
}

short address_link::getFamily() const
{
    return family;
}

bool address_link::operator==(const address_base& addr)
{
    if(family != addr.getFamily())
        return false;
    if(strAddr != addr.getStrAddr())
        return false;
    else
        return true;
}

bool address_link::operator!=(const address_base& addr)
{
    if(family == addr.getFamily())
        return false;
    if(strAddr == addr.getStrAddr())
        return false;
    else
        return true;
}
