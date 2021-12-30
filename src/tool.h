#ifndef TOOL_H
#define TOOL_H

#include <vector>
#include <sys/sockio.h>
#include <kvm.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/user.h>
#include <sys/sysctl.h>
//#define LOGURU_WITH_STREAMS 1
#include "loguru/loguru.hpp"
#include "json/json.hpp"
#include "magic_enum/magic_enum.hpp"
#include "sockpp/socket.h"
#include "sockpp/version.h"
#include "nmcommand.h"
#include "nmjsonconst.h"

using json = nlohmann::json;

class AddressGroup;

class tool
{
protected:
    static inline const std::string DHCP_CLIENT_PROCESS = "dhclient";
public:
    static std::shared_ptr<AddressGroup> getAddrFromJson(json);
    static int getIfFlags(std::string);
    static bool setIfFlags(std::string, int);
    static std::string getIfPrimaryAddr4(std::string);
//    static std::string getIfPrimaryAddr6(std::string);
    static bool isValidGw4(uint32_t, uint32_t, uint32_t);
    static bool isValidBcast4(uint32_t, uint32_t, uint32_t);
    static std::vector<std::tuple<int, std::string, std::string>> getActiveProcesses();
    static bool isDHCPEnabled(std::string);
    static bool termDHCPClient(std::string, short sig=SIGTERM);
    static int getDHCPClientPid(std::string);
};

#endif // TOOL_H
