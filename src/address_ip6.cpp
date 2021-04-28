#include "address_ip6.h"

address_ip6::address_ip6(const struct sockaddr_in6* psa)
{
    sock_addr = new struct sockaddr_storage;
    memset(sock_addr,0,sizeof(struct sockaddr_storage));
    memcpy(sock_addr, psa, sizeof(struct sockaddr_in6));
    setIpAddr6();
}

address_ip6::address_ip6(std::string str_addr)
{
    const char* chr_ptr = str_addr.c_str();
    if(inet_pton(family, chr_ptr, &ip_addr6) == 1)
    {
        strAddr = str_addr;
    }
    else
    {
        throw nmExcept;
    }
    sock_addr = new struct sockaddr_storage;
    memset(sock_addr,0,sizeof(struct sockaddr_storage));
    ((struct sockaddr*)sock_addr)->sa_family = family;
    ((struct sockaddr_in6*)sock_addr)->sin6_len = sizeof(struct sockaddr_in6);
    memcpy(&(((struct sockaddr_in6*)sock_addr)->sin6_addr.s6_addr), &(ip_addr6.s6_addr), sizeof(in6_addr::s6_addr));
}

address_ip6::~address_ip6()
{
    delete sock_addr;
}

void address_ip6::setIpAddr6()
{
    void* addr_ptr = 0;
    char address[INET6_ADDRSTRLEN];

    if (((struct sockaddr*)sock_addr)->sa_family == family)
    {
        addr_ptr = &((struct sockaddr_in6*) sock_addr)->sin6_addr;
        memcpy(&ip_addr6, addr_ptr, sizeof(struct in6_addr));
    }
    else
    {
        throw nmExcept;
    }

    if(inet_ntop(family, addr_ptr, address, sizeof(address)) != nullptr)
    {
        strAddr = std::string(address);
    }
    else
    {
        throw nmExcept;
    }
}

std::string address_ip6::getStrAddr() const
{
    return strAddr;
}

const struct sockaddr_storage* address_ip6::getSockAddr() const
{
    return sock_addr;
}

short address_ip6::getFamily() const
{
    return family;
}

bool address_ip6::operator==(const address_base& addr)
{
    if(family != addr.getFamily())
        return false;
    if(ip_addr6.s6_addr != ((struct sockaddr_in6*)addr.getSockAddr())->sin6_addr.s6_addr)
        return false;
    else
        return true;
}

bool address_ip6::operator!=(const address_base& addr)
{
    if(family == addr.getFamily())
        return false;
    if(ip_addr6.s6_addr == ((struct sockaddr_in6*)addr.getSockAddr())->sin6_addr.s6_addr)
        return false;
    else
        return true;
}
