#ifndef NMWORKERWPA_H
#define NMWORKERWPA_H

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
#include "wpasocket.h"

namespace fs = std::filesystem;

class NmWorkerWpa : public NmWorkerBase
{
protected:
    static constexpr NmCommand Cmds[] =
    {
        { NmScope::WPA, NmCmd::WPA_LIST_IF },
        { NmScope::WPA, NmCmd::WPA_LIST },
        { NmScope::WPA, NmCmd::WPA_SCAN },
        { NmScope::WPA, NmCmd::WPA_SCAN_RESULTS },
        { NmScope::WPA, NmCmd::WPA_STATUS },
        { NmScope::WPA, NmCmd::WPA_SETPSK },
        { NmScope::WPA, NmCmd::WPA_SETPARAM },
        { NmScope::WPA, NmCmd::WPA_CONNECT },
        { NmScope::WPA, NmCmd::WPA_DISCONNECT },
        { NmScope::WPA, NmCmd::WPA_REASSOC },
        { NmScope::WPA, NmCmd::WPA_ADD },
        { NmScope::WPA, NmCmd::WPA_REMOVE },
        { NmScope::WPA, NmCmd::WPA_RESET },
        { NmScope::WPA, NmCmd::WPA_SAVE }
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
    static inline const std::string COMMAND_FLUSH = "FLUSH";
    static inline const std::string COMMAND_RECONFIGURE = "RECONFIGURE";
    static inline const std::string COMMAND_SAVE = "SAVE_CONFIG";
    static inline const std::string RESULT_CONNECTED = "CTRL-EVENT-CONNECTED";
    static inline const std::string RESULT_NOT_FOUND = "CTRL-EVENT-NETWORK-NOT-FOUND";
    static inline const std::string RESULT_TEMP_DISABLED = "CTRL-EVENT-SSID-TEMP-DISABLED";
    static inline const std::string RESULT_SCAN_RESULTS = "CTRL-EVENT-SCAN-RESULTS";
    static inline const std::string RESULT_WRONG_KEY = "reason=WRONG_KEY";
    static inline const std::string RESULT_CONN_FAILED = "reason=CONN_FAILED";
    static inline const std::string RESULT_HANDSHAKE_FAILED = "4-Way Handshake failed";
    static inline const std::string RESULT_DRIVER_REQUEST_FAILED = "Association request to the driver failed";
    static inline const std::string RESULT_COMPLETED = "wpa_state=COMPLETED";
    static inline const std::string RESULT_REJECT_SCAN = "<3>Reject scan trigger since one is already pending";
    static inline const std::string RESULT_FAILED_SCAN = "<4>Failed to initiate AP scan";
    static inline const std::string RESULT_FAIL_BUSY = "FAIL-BUSY";

    static constexpr int BUF_LEN = 4096;
    static constexpr std::chrono::milliseconds WAIT_TIME = std::chrono::milliseconds(200);
    static constexpr std::chrono::milliseconds WAIT_RECONFIGURE_TIME = std::chrono::milliseconds(3000);
    static constexpr std::chrono::milliseconds WAIT_CONNECT_TIME = std::chrono::milliseconds(1000);
    static constexpr int WAIT_CYCLES = 10;

    char* buf = nullptr;
    const std::string csaPrefix = "/tmp/nmd_wpaw.XXXXX";
    const std::string defSsaDir = "/var/run/wpa_supplicant/";
    std::string cliSockAddr;
    std::string srvSockAddrDir;
    bool wpaCtrlCmd(WpaSocket*, const std::string);
    bool wpaCtrlCmd(WpaSocket*, const std::string, const std::string);
    bool wpaCtrlCmd(const std::string, const std::string);
    bool wpaCtrlCmd(const std::string, const std::string, const std::string);
    json wpaConnectCmd(int, std::string);
    bool isValidWpaIf(std::string);
    json getJsonFromBufTable(std::string);
    json getJsonFromBufLines(std::string);
    std::string getStringParamFromCommand(NmCommandData*, std::string);
    bool getBoolParamFromCommand(NmCommandData*, std::string);
    int getIntParamFromCommand(NmCommandData*, std::string);
    bool setNetworkParam(std::string, int, std::string, std::string, bool);
    char* searchLineInBuf(const char* mask);
    bool removeNetwork(std::string, int);
    json resetWpaStatus(std::string);
    json saveWpaConfig(std::string);
public:
    NmWorkerWpa();
    NmWorkerWpa(std::string);
    ~NmWorkerWpa();
    NmScope getScope();
    json execCmd(NmCommandData*);
    bool isValidCmd(NmCommandData*);
    json execCmdWpaListIf(NmCommandData*);
    json execCmdWpaList(NmCommandData*);
    json execCmdWpaScan(NmCommandData*);
    json execCmdWpaScanResults(NmCommandData*);
    json execCmdWpaStatus(NmCommandData*);
    json execCmdWpaSetPsk(NmCommandData*);
    json execCmdWpaSetNetParam(NmCommandData*);
    json execCmdWpaConnect(NmCommandData*);
    json execCmdWpaDisconnect(NmCommandData*);
    json execCmdWpaReassoc(NmCommandData*);
    json execCmdWpaAdd(NmCommandData*);
    json execCmdWpaRemove(NmCommandData*);
    json execCmdWpaReset(NmCommandData*);
    json execCmdWpaSave(NmCommandData*);
};


#endif // NMWORKERWPA_H
