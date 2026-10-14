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

// Hides PSK / WEP keys / passwords before a request or a wpa_supplicant command is logged
std::string redactSecrets(const std::string& text);

#endif // NMCOMMANDDATA_H
