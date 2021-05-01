#ifndef TOOL_H
#define TOOL_H

#include <sys/sockio.h>
#define LOGURU_WITH_STREAMS 1
#include "loguru/loguru.hpp"
#include "json/json.hpp"
#include "magic_enum/magic_enum.hpp"
#include "sockpp/socket.h"
#include "sockpp/version.h"
#include "nmcommand.h"
#include "nmjsonconst.h"

using json = nlohmann::json;

class addr;

class tool
{
public:
    static addr* getAddrFromJson(json);
    static int getIfFlags(std::string);
    static bool setIfFlags(std::string, int);
    static bool isValidGw4(uint32_t, uint32_t, uint32_t);
    static bool isValidBcast4(uint32_t, uint32_t, uint32_t);
};

#endif // TOOL_H
