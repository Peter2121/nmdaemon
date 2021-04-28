#ifndef ADDRESS_H
#define ADDRESS_H

#include <string>
#include "nmexception.h"

class address_base
{
public:
    virtual ~address_base() {}
    virtual std::string getStrAddr() const = 0;
    virtual const struct sockaddr_storage* getSockAddr() const = 0;
    virtual short getFamily() const = 0;
    virtual bool operator==(const address_base&) = 0;
    virtual bool operator!=(const address_base&) = 0;
};

#endif // ADDRESS_H
