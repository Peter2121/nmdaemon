#include "addressip4.h"

AddressIp4::AddressIp4(const struct sockaddr_in* psa)
{
    sock_addr = new struct sockaddr_storage;
    memset(sock_addr,0,sizeof(struct sockaddr_storage));
    memcpy(sock_addr, psa, sizeof(struct sockaddr_in));
    setIpAddr();
}

AddressIp4::AddressIp4(std::string str_addr)
{
    const char* chr_ptr = str_addr.c_str();
    if(inet_pton(family, chr_ptr, &ip_addr) == 1)
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
    ((struct sockaddr_in*)sock_addr)->sin_len = sizeof(struct sockaddr_in);
    ((struct sockaddr_in*)sock_addr)->sin_addr.s_addr = ip_addr.s_addr;
}

AddressIp4::~AddressIp4()
{
    delete sock_addr;
}

void AddressIp4::setIpAddr()
{
    void* addr_ptr = 0;
    char address[INET_ADDRSTRLEN];

    if (((struct sockaddr*)sock_addr)->sa_family == family)
    {
        addr_ptr = &((struct sockaddr_in*) sock_addr)->sin_addr;
        memcpy(&ip_addr, addr_ptr, sizeof(struct in_addr));
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

std::string AddressIp4::getStrAddr() const
{
    return strAddr;
}

const struct sockaddr_storage* AddressIp4::getSockAddr() const
{
    return sock_addr;
}

short AddressIp4::getFamily() const
{
    return family;
}

bool AddressIp4::operator==(const AddressBase& addr)
{
    if(family != addr.getFamily())
        return false;
    if(ip_addr.s_addr != ((struct sockaddr_in*)addr.getSockAddr())->sin_addr.s_addr)
        return false;
    else
        return true;
}

bool AddressIp4::operator!=(const AddressBase& addr)
{
    if(family == addr.getFamily())
        return false;
    if(ip_addr.s_addr == ((struct sockaddr_in*)addr.getSockAddr())->sin_addr.s_addr)
        return false;
    else
        return true;
}
