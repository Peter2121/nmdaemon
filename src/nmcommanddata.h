#ifndef NMCOMMANDDATA_H
#define NMCOMMANDDATA_H

#include "json/json.hpp"
#include "magic_enum/magic_enum.hpp"
#include "nmcommand.h"
#include "nmjsonconst.h"

using json = nlohmann::json;

class NmCommandData
{
protected:
    NmCommand command;
    json json_data;
public:
    NmCommandData(std::string str_data);
    bool isValid();
    json getJsonData();
    NmCommand getCommand();
};

#endif // NMCOMMANDDATA_H
