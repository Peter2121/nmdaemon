#include "dummy_worker.h"

dummy_worker::dummy_worker()
{ }

dummy_worker::~dummy_worker()
{ }

NmScope dummy_worker::getScope()
{
    return NmScope::DUMMY;
}

json dummy_worker::execCmd(nmcommand_data*)
{
    return { { JSON_PARAM_RESULT, JSON_PARAM_SUCC } };
}

bool dummy_worker::isValidCmd(nmcommand_data* pcmd)
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

