#include "addressip6.h"

AddressIp6::AddressIp6(const struct sockaddr_in6* psa)
{
    sock_addr = new struct sockaddr_storage;
    memset(sock_addr,0,sizeof(struct sockaddr_storage));
    memcpy(sock_addr, psa, sizeof(struct sockaddr_in6));
    setIpAddr6();
}

AddressIp6::AddressIp6(std::string str_addr)
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
    ((struct sockaddr_in6*)sock_addr)->sin6_family = family;
    ((struct sockaddr_in6*)sock_addr)->sin6_len = sizeof(struct sockaddr_in6);
    memcpy(&(((struct sockaddr_in6*)sock_addr)->sin6_addr.s6_addr), &(ip_addr6.s6_addr), sizeof(in6_addr::s6_addr));
}

AddressIp6::~AddressIp6()
{
    delete sock_addr;
}

void AddressIp6::setIpAddr6()
{
    void* addr_ptr = 0;
    char address[INET6_ADDRSTRLEN];

    if (((struct sockaddr_in6*)sock_addr)->sin6_family == family)
    {
        addr_ptr = &((struct sockaddr_in6*)sock_addr)->sin6_addr;
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

std::string AddressIp6::getStrAddr() const
{
    return strAddr;
}

const struct sockaddr_storage* AddressIp6::getSockAddr() const
{
    return sock_addr;
}

short AddressIp6::getFamily() const
{
    return family;
}

/*
struct in6_addr {
        u_int8_t  s6_addr[16];  // IPv6 address
}
struct sockaddr_in6 {
       u_char           sin6_len;      // length of this structure
       u_char           sin6_family;   // AF_INET6
       u_int16m_t       sin6_port;     // Transport layer port
       u_int32m_t       sin6_flowinfo; // IPv6 flow information
       struct in6_addr  sin6_addr;     // IPv6 address
}
*/

bool AddressIp6::operator==(const AddressBase& addr)
{
    if(family != addr.getFamily())
        return false;
    const void *p1 = &(((struct sockaddr_in6*)addr.getSockAddr())->sin6_addr.s6_addr);
    const void *p2 = &(ip_addr6.s6_addr);
    if(memcmp(p1,p2,sizeof(struct in6_addr))!=0)
//    if(ip_addr6.s6_addr != ((struct sockaddr_in6*)addr.getSockAddr())->sin6_addr.s6_addr)
        return false;
    else
        return true;
}

bool AddressIp6::operator!=(const AddressBase& addr)
{
    if(family == addr.getFamily())
        return false;
    const void *p1 = &(((struct sockaddr_in6*)addr.getSockAddr())->sin6_addr.s6_addr);
    const void *p2 = &(ip_addr6.s6_addr);
    if(memcmp(p1,p2,sizeof(struct in6_addr))==0)
//    if(ip_addr6.s6_addr == ((struct sockaddr_in6*)addr.getSockAddr())->sin6_addr.s6_addr)
        return false;
    else
        return true;
}
