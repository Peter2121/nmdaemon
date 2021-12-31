#ifndef NMWORKERDUMMY_H
#define NMWORKERDUMMY_H

#include "nmworkerbase.h"

class NmWorkerDummy : public NmWorkerBase
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
