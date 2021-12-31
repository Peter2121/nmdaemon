#ifndef NMEXCEPTION_H
#define NMEXCEPTION_H

#include <exception>

class NmException : public std::exception
{
public:
    NmException();
    virtual const char* what() const throw()
    {
      return "Exception in nmdaemon";
    }
};

#endif // NMEXCEPTION_H
