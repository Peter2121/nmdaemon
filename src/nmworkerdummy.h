#ifndef NMWORKERDUMMY_H
#define NMWORKERDUMMY_H

#include "nmworker.h"

class NmWorkerDummy : public NmWorker
{
protected:
    static constexpr NmCommand Cmds[] =
    {
        { NmScope::DUMMY, NmCmd::TEST }
    };
public:
    NmWorkerDummy();
    ~NmWorkerDummy();
    NmScope getScope();
    json execCmd(NmCommandData*);
    bool isValidCmd(NmCommandData*);
};

#endif // NMWORKERDUMMY_H
