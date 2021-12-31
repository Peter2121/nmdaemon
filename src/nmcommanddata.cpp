#include "nmcommanddata.h"

NmCommandData::NmCommandData(std::string str_data)
{
    json_data = json::parse(str_data, nullptr, false);
    if(!json_data.is_discarded())
    {
        if (json_data.contains(JSON_PARAM_SCOPE))
        {
            std::string strScope = json_data[JSON_PARAM_SCOPE];
            auto enumscope = magic_enum::enum_cast<NmScope>(strScope);
            if (enumscope.has_value())
                command.scope = enumscope.value();
            else
                command.scope = NmScope::NONE;
        }
        else
            command.scope = NmScope::NONE;
        if (json_data.contains(JSON_PARAM_CMD))
        {
            std::string strCmd = json_data[JSON_PARAM_CMD];
            auto enumcmd = magic_enum::enum_cast<NmCmd>(strCmd);
            if (enumcmd.has_value())
                command.cmd = enumcmd.value();
            else
                command.cmd = NmCmd::NONE;
        }
        else
            command.cmd = NmCmd::NONE;
    }
    else
    {
        command.scope = NmScope::NONE;
        command.cmd = NmCmd::NONE;
    }
}

bool NmCommandData::isValid()
{
    if(json_data.is_discarded())
        return false;
    if(command.scope == NmScope::NONE)
        return false;
    if(command.cmd == NmCmd::NONE)
        return false;

    return true;
}

json NmCommandData::getJsonData()
{
    return json_data;
}

NmCommand NmCommandData::getCommand()
{
    return command;
}
