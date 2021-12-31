#ifndef WPA_WORKER_H
#define WPA_WORKER_H

#include <unistd.h>
#include <sys/socket.h>
#include <net/route.h>
#include <filesystem>
#include <sstream>
#include <vector>
#include <algorithm>
#include <string_view>
#include <thread>
//#define LOGURU_WITH_STREAMS 1
#include "loguru/loguru.hpp"
#include "sockpp/socket.h"
#include "sockpp/unix_dgram_socket.h"
#include "sockpp/version.h"
#include "nmworkerbase.h"
#include "addressgroup.h"
#include "tool.h"

namespace fs = std::filesystem;

class wpa_worker : public NmWorkerBase
{
protected:
    static constexpr NmCommand Cmds[] =
    {
        { NmScope::WPA, NmCmd::WPA_LIST_IF },
        { NmScope::WPA, NmCmd::WPA_LIST },
        { NmScope::WPA, NmCmd::WPA_SCAN },
        { NmScope::WPA, NmCmd::WPA_STATUS },
        { NmScope::WPA, NmCmd::WPA_SETPSK },
        { NmScope::WPA, NmCmd::WPA_CONNECT },
        { NmScope::WPA, NmCmd::WPA_DISCONNECT },
        { NmScope::WPA, NmCmd::WPA_REASSOC },
        { NmScope::WPA, NmCmd::WPA_ADD },
        { NmScope::WPA, NmCmd::WPA_REMOVE }
    };

    static inline const std::string COMMAND_LIST = "LIST_NETWORKS";
    static inline const std::string COMMAND_SCAN = "SCAN";
    static inline const std::string COMMAND_SCAN_RESULTS = "SCAN_RESULTS";
    static inline const std::string COMMAND_PING = "PING";
    static inline const std::string COMMAND_STATUS = "STATUS";
    static inline const std::string COMMAND_ADD = "ADD_NETWORK";
    static inline const std::string COMMAND_SET = "SET_NETWORK";
    static inline const std::string COMMAND_SELECT = "SELECT_NETWORK";
    static inline const std::string COMMAND_REMOVE = "REMOVE_NETWORK";
    static inline const std::string COMMAND_ATTACH = "ATTACH";
    static inline const std::string COMMAND_DISCONNECT = "DISCONNECT";
    static inline const std::string COMMAND_REASSOCIATE = "REASSOCIATE";

    static constexpr int BUF_LEN = 4096;
    static constexpr std::chrono::milliseconds WAIT_TIME = std::chrono::milliseconds(200);
    static constexpr int WAIT_CYCLES = 10;

    char* buf = nullptr;
    const std::string csaPrefix = "/tmp/nmd_wpaw.XXXXX";
    const std::string defSsaDir = "/var/run/wpa_supplicant/";
    std::string cliSockAddr;
    std::string srvSockAddrDir;
    bool wpaCtrlCmd(const std::string, const std::string);
    bool wpaCtrlCmd(const std::string, const std::string, const std::string);
    bool isValidWpaIf(std::string);
    json getJsonFromBufTable(std::string);
    json getJsonFromBufLines(std::string);
    std::string getParamFromCommand(NmCommandData*, std::string);
    bool setNetworkParam(std::string, int, std::string, std::string, bool);
    char* searchLineInBuf(const char* mask);
    bool removeNetwork(std::string, int);
public:
    wpa_worker();
    wpa_worker(std::string);
    ~wpa_worker();
    NmScope getScope();
    json execCmd(NmCommandData*);
    bool isValidCmd(NmCommandData*);
    json execCmdWpaListIf(NmCommandData*);
    json execCmdWpaList(NmCommandData*);
    json execCmdWpaScan(NmCommandData*);
    json execCmdWpaStatus(NmCommandData*);
    json execCmdWpaSetPsk(NmCommandData*);
    json execCmdWpaConnect(NmCommandData*);
    json execCmdWpaDisconnect(NmCommandData*);
    json execCmdWpaReassoc(NmCommandData*);
    json execCmdWpaAdd(NmCommandData*);
    json execCmdWpaRemove(NmCommandData*);
};


#endif // WPA_WORKER_H
