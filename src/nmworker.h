#ifndef NMWORKER_H
#define NMWORKER_H

#include "nmcommand_data.h"

class NmWorker
{
public:
    virtual ~NmWorker() {}
    virtual NmScope getScope() = 0;
    virtual json execCmd(nmcommand_data*) = 0;
    virtual bool isValidCmd(nmcommand_data*) = 0;
};

#endif // NMWORKER_H
