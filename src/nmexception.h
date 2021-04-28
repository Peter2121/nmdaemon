#ifndef NMEXCEPTION_H
#define NMEXCEPTION_H

#include <exception>

class nmexception : public std::exception
{
public:
    nmexception();
    virtual const char* what() const throw()
    {
      return "Exception in nmdaemon";
    }
};

#endif // NMEXCEPTION_H
