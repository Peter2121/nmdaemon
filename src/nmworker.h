#ifndef NMWORKER_H
#define NMWORKER_H

#include "nmcommand_data.h"

class nmworker
{
public:
    virtual ~nmworker() {}
    virtual nmscope getScope() = 0;
    virtual json execCmd(nmcommand_data*) = 0;
    virtual bool isValidCmd(nmcommand_data*) = 0;
};

#endif // NMWORKER_H
