#include "nmworkerdummy.h"

NmWorkerDummy::NmWorkerDummy()
{ }

NmWorkerDummy::~NmWorkerDummy()
{ }

NmScope NmWorkerDummy::getScope()
{
    return NmScope::DUMMY;
}

json NmWorkerDummy::execCmd(NmCommandData*)
{
    return { { JSON_PARAM_RESULT, JSON_PARAM_SUCC } };
}

bool NmWorkerDummy::isValidCmd(NmCommandData* pcmd)
{
    if( pcmd->getCommand().scope != getScope() )
        return false;
    int cmd_size = sizeof(Cmds) / sizeof(Cmds[0]);
    for(int i=0; i<cmd_size; i++)
    {
        if(pcmd->getCommand().cmd == Cmds[i].cmd)
            return true;
    }
    return false;
}

