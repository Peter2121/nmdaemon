#ifndef NMWORKERBASE_H
#define NMWORKERBASE_H

#include "nmcommanddata.h"

class NmWorkerBase
{
public:
    virtual ~NmWorkerBase() {}
    virtual NmScope getScope() = 0;
    virtual json execCmd(NmCommandData*) = 0;
    virtual bool isValidCmd(NmCommandData*) = 0;
};

#endif // NMWORKERBASE_H
