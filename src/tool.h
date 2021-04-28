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
#include "addr.h"

using json = nlohmann::json;

class tool
{
public:
    static addr* getAddrFromJson(json);
    static int getIfFlags(std::string ifname);
    static bool setIfFlags(std::string ifname, int setflags);
};

#endif // TOOL_H
