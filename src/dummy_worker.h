#ifndef DUMMY_WORKER_H
#define DUMMY_WORKER_H

#include "nmworker.h"

class dummy_worker : public NmWorker
{
protected:
    static constexpr NmCommand Cmds[] =
    {
        { NmScope::DUMMY, NmCmd::TEST }
    };
public:
    dummy_worker();
    ~dummy_worker();
    NmScope getScope();
    json execCmd(nmcommand_data*);
    bool isValidCmd(nmcommand_data*);
};

#endif // DUMMY_WORKER_H
