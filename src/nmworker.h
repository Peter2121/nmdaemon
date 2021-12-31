#ifndef NMWORKER_H
#define NMWORKER_H

#include "nmcommanddata.h"

class NmWorker
{
public:
    virtual ~NmWorker() {}
    virtual NmScope getScope() = 0;
    virtual json execCmd(NmCommandData*) = 0;
    virtual bool isValidCmd(NmCommandData*) = 0;
};

#endif // NMWORKER_H
