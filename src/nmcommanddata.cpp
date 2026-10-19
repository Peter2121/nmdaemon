#include "nmcommanddata.h"

NmCommandData::NmCommandData(std::string str_data)
{
    command.scope = NmScope::NONE;
    command.cmd = NmCmd::NONE;
    json_data = json::parse(str_data, nullptr, false);
    if(json_data.is_discarded() || !json_data.is_object())
        return;

    if (json_data.contains(JSON_PARAM_SCOPE) && json_data[JSON_PARAM_SCOPE].is_string())
    {
        auto enumscope = magic_enum::enum_cast<NmScope>(json_data[JSON_PARAM_SCOPE].get<std::string>());
        if (enumscope.has_value())
            command.scope = enumscope.value();
    }

    if (json_data.contains(JSON_PARAM_CMD) && json_data[JSON_PARAM_CMD].is_string())
    {
        auto enumcmd = magic_enum::enum_cast<NmCmd>(json_data[JSON_PARAM_CMD].get<std::string>());
        if (enumcmd.has_value())
            command.cmd = enumcmd.value();
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
