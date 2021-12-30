#ifndef ADDRESS_H
#define ADDRESS_H

#include <string>
#include "nmexception.h"

class AddressBase
{
public:
    virtual ~AddressBase() {}
    virtual std::string getStrAddr() const = 0;
    virtual const struct sockaddr_storage* getSockAddr() const = 0;
    virtual short getFamily() const = 0;
    virtual bool operator==(const AddressBase&) = 0;
    virtual bool operator!=(const AddressBase&) = 0;
};

#endif // ADDRESS_H
