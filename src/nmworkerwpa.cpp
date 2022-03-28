#include "nmworkerwpa.h"

NmWorkerWpa::NmWorkerWpa() :
    cliSockAddr(mktemp((char*)csaPrefix.c_str())),
    srvSockAddrDir(defSsaDir)
{
    buf = new char[BUF_LEN]();
}

NmWorkerWpa::NmWorkerWpa(std::string ssad) :
    cliSockAddr(mktemp((char*)csaPrefix.c_str())),
    srvSockAddrDir()
{
    buf = new char[BUF_LEN]();
    if(ssad.compare(ssad.length()-1,1,"/")==0)
        srvSockAddrDir = ssad;
    else
        srvSockAddrDir = ssad +"/";
}

NmWorkerWpa::~NmWorkerWpa()
{
    delete buf;
}

NmScope NmWorkerWpa::getScope()
{
    return NmScope::WPA;
}

json NmWorkerWpa::execCmd(NmCommandData* pcmd)
{
    switch (pcmd->getCommand().cmd)
    {
        case NmCmd::WPA_LIST_IF :
            return execCmdWpaListIf(pcmd);
        case NmCmd::WPA_LIST :
            return execCmdWpaList(pcmd);
        case NmCmd::WPA_SCAN :
            return execCmdWpaScan(pcmd);
        case NmCmd::WPA_SCAN_RESULTS :
            return execCmdWpaScanResults(pcmd);
        case NmCmd::WPA_STATUS :
            return execCmdWpaStatus(pcmd);
        case NmCmd::WPA_SETPSK :
            return execCmdWpaSetPsk(pcmd);
        case NmCmd::WPA_SETPARAM :
            return execCmdWpaSetNetParam(pcmd);
        case NmCmd::WPA_CONNECT :
            return execCmdWpaConnect(pcmd);
        case NmCmd::WPA_DISCONNECT :
            return execCmdWpaDisconnect(pcmd);
        case NmCmd::WPA_REASSOC :
            return execCmdWpaReassoc(pcmd);
        case NmCmd::WPA_ADD :
            return execCmdWpaAdd(pcmd);
        case NmCmd::WPA_REMOVE :
            return execCmdWpaRemove(pcmd);
        default :
            return { { JSON_PARAM_RESULT, JSON_PARAM_ERR }, {JSON_PARAM_ERR, JSON_DATA_ERR_INVALID_COMMAND} };
    }
}

bool NmWorkerWpa::isValidCmd(NmCommandData* pcmd)
{
    if( pcmd->getCommand().scope != getScope() )
        return false;

    for(auto cm : Cmds)
    {
        if ( cm.cmd == pcmd->getCommand().cmd )
            return true;
    }

    return false;
}

bool NmWorkerWpa::isValidWpaIf(std::string ifn)
{
    const std::string pong = "PONG";
    std::string strCmd = COMMAND_PING;
    if(!wpaCtrlCmd(strCmd, ifn))
        return false;
    std::stringstream ss(buf);
    std::string resp;
    std::getline(ss, resp, '\n');
    if(pong!=resp)
        return false;
    return true;
}

json NmWorkerWpa::execCmdWpaListIf(NmCommandData*)
{
    std::vector<std::string> ifs;
    nlohmann::json retIfListJson;
    nlohmann::json json_ifs;
    for (const auto& entry : fs::directory_iterator(srvSockAddrDir, fs::directory_options::skip_permission_denied))
    {
        if( entry.is_socket() && isValidWpaIf(entry.path().filename()) )
            ifs.push_back(entry.path().filename());
    }
    if(ifs.size()==0)
        return JSON_RESULT_NOTFOUND;
    retIfListJson[JSON_PARAM_RESULT] = JSON_PARAM_SUCC;
    json_ifs[JSON_PARAM_INTERFACES] = ifs;
    retIfListJson[JSON_PARAM_DATA] = json_ifs;
    return retIfListJson;
}

json NmWorkerWpa::getJsonFromBufLines(std::string result)
{
    const char DELIM = '=';
    std::stringstream ss(buf);
    std::stringstream ssline;
    std::string line;
    std::string key;
    std::string value;
    json jdata {};
    json jret {};
    while (getline (ss, line))
    {
        ssline = std::stringstream(line);
        getline (ssline, key, DELIM);
        getline (ssline, value, DELIM);
        jdata[key] = value;
    }
    jret[JSON_PARAM_RESULT] = result;
    jret[JSON_PARAM_DATA] = jdata;
    return jret;
}

json NmWorkerWpa::getJsonFromBufTable(std::string param_name)
{
    const char DELIM_HEAD = '/';
    const char DELIM_LINES = '\t';
    std::vector<std::string> columns;
    std::vector<std::string> lines;
    std::stringstream ss(buf);
    std::string column;
    std::string line;
    std::string_view column_view;
    int num_columns = 0;
    int i, j;
    json jline {};
    json jdata {};
    json jret {};

    std::vector<json> jlines;

    while (getline (ss, line))
    {
        lines.push_back(line);
    }
    ss = std::stringstream(lines[0]);
    while (getline (ss, column, DELIM_HEAD))
    {
        column_view = column;
        column_view.remove_prefix(std::min(column.find_first_not_of(" "), column.size()));
        column_view.remove_suffix(std::min(column.size() - column.find_last_not_of(" ") - 1, column.size()));
        columns.push_back(static_cast<std::string>(column_view));
    }
    num_columns = columns.size();
    i = 0;
    for(auto ln : lines)
    {
        if(i==0)
        {
            i++;
            continue;
        }
        ss = std::stringstream(lines[i]);
        j=0;
        jline = {};
        while (getline (ss, column, DELIM_LINES))
        {
            jline[columns[j]] = column;
            j++;
            if(j>=num_columns)
                break;
        }
        jlines.push_back(jline);
        i++;
    }
    jdata[param_name] = jlines;
    jret[JSON_PARAM_RESULT] = JSON_PARAM_SUCC;
    jret[JSON_PARAM_DATA] = jdata;
    return jret;
}

json NmWorkerWpa::execCmdWpaList(NmCommandData* pcmd)
{
    std::string strCmd;
    json cmd = {};
    std::string ifname = "";

    ifname = getStringParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
    if(ifname.empty())
        return JSON_RESULT_ERR;

    strCmd = COMMAND_LIST;
    if(!wpaCtrlCmd(strCmd, ifname))
        return JSON_RESULT_ERR;

    return getJsonFromBufTable(JSON_PARAM_NETWORKS);
}

json NmWorkerWpa::execCmdWpaScan(NmCommandData* pcmd)
{
    sockpp::unix_dgram_socket sock;
    std::string strCmd;
    json cmd = {};
    std::string ifname = "";

    ifname = getStringParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
    if(ifname.empty())
        return JSON_RESULT_ERR;
    strCmd = COMMAND_SCAN;
    if(!wpaCtrlCmd(strCmd, "CTRL-EVENT-SCAN-RESULTS", ifname))
        return JSON_RESULT_ERR;

    strCmd=COMMAND_SCAN_RESULTS;

    if(!wpaCtrlCmd(strCmd, ifname))
        return JSON_RESULT_ERR;

    return getJsonFromBufTable(JSON_PARAM_NETWORKS);
}

json NmWorkerWpa::execCmdWpaScanResults(NmCommandData* pcmd)
{
    sockpp::unix_dgram_socket sock;
    std::string strCmd;
    json cmd = {};
    std::string ifname = "";

    ifname = getStringParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
    if(ifname.empty())
        return JSON_RESULT_ERR;

    strCmd=COMMAND_SCAN_RESULTS;

    if(!wpaCtrlCmd(strCmd, ifname))
        return JSON_RESULT_ERR;

    return getJsonFromBufTable(JSON_PARAM_NETWORKS);
}

bool NmWorkerWpa::wpaCtrlCmd(const std::string strCmd, const std::string ifname)
{
    sockpp::unix_dgram_socket sock;
    int res,i;
    const std::string srv_sock = srvSockAddrDir + ifname;

    if (!sock.bind(sockpp::unix_address(cliSockAddr)))
    {
        LOG_S(ERROR) << "wpaCtrlCmd cannot connect to client socket " << cliSockAddr << " : " << sock.last_error_str();
        return false;
    }

    if (!sock.connect(sockpp::unix_address(srv_sock)))
    {
        LOG_S(ERROR) << "wpaCtrlCmd cannot connect to server socket " << srv_sock << " : " << sock.last_error_str();
        sock.close();
        unlink(cliSockAddr.c_str());
        return false;
    }

    LOG_S(INFO) << "wpaCtrlCmd sending command: " << strCmd;
    if (sock.send(strCmd) != ssize_t(strCmd.length()))
    {
        LOG_S(ERROR) << "wpaCtrlCmd cannot write to socket: " << sock.last_error_str();
        sock.close();
        unlink(cliSockAddr.c_str());
        return false;
    }

    for(i=0; i<=WAIT_CYCLES; i++)
    {
        memset(buf, 0, BUF_LEN*sizeof(char));
        res = sock.recv(buf, BUF_LEN, MSG_DONTWAIT);
        std::this_thread::sleep_for(WAIT_TIME);
        if(res>0)
        {
            LOG_S(INFO) << "wpaCtrlCmd received data: " << buf;
            if ( (res > 0 && buf[0] == '<') || (res > 6 && strncmp(buf, "IFNAME=", 7) == 0) )
                /* This is an unsolicited message from wpa_supplicant, not the reply to the request */
                /* See wpa_ctrl.c:559 */
                continue;
            else
                break;
        }
    }
    sock.close();
    unlink(cliSockAddr.c_str());
    if(i<=WAIT_CYCLES)
        return true;
    else
        return false;
}

json NmWorkerWpa::wpaConnectCmd(int id, std::string ifname)
{
    const std::string srv_sock = srvSockAddrDir + ifname;
    const std::string netid = std::to_string(id);
    std::string strCmd = COMMAND_SELECT + " " + netid;
    int res,i;
    const int MAXLINE = 32;
    char line[MAXLINE];
//    char* ptr1 = nullptr;
    char* ptr2 = nullptr;
    std::string ideq;
    std::string conn_status;
    sockpp::unix_dgram_socket sock;
    json jret = {};
    std::string strResult = JSON_PARAM_SUCC;
    bool idFound = false;
    //wpaCtrlCmd(strCmd, "CTRL-EVENT-CONNECTED", ifname))

    if (!sock.bind(sockpp::unix_address(cliSockAddr)))
    {
        LOG_S(ERROR) << "wpaCtrlCmd cannot connect to client socket " << cliSockAddr << " : " << sock.last_error_str();
        return JSON_RESULT_ERR;
    }

    if (!sock.connect(sockpp::unix_address(srv_sock)))
    {
        LOG_S(ERROR) << "wpaCtrlCmd cannot connect to server socket " << srv_sock << " : " << sock.last_error_str();
        sock.close();
        return JSON_RESULT_ERR;
    }
    LOG_S(INFO) << "wpaCtrlCmd sending command: " << COMMAND_ATTACH;
    if (sock.send(COMMAND_ATTACH) != ssize_t(COMMAND_ATTACH.length()))
    {
        LOG_S(ERROR) << "wpaCtrlCmd cannot write to socket: " << sock.last_error_str();
        sock.close();
        return JSON_RESULT_ERR;
    }

    LOG_S(INFO) << "wpaCtrlCmd sending command: " << strCmd;
    if (sock.send(strCmd) != ssize_t(strCmd.length()))
    {
        LOG_S(ERROR) << "wpaCtrlCmd cannot write to socket: " << sock.last_error_str();
        sock.close();
        return JSON_RESULT_ERR;
    }
//
// <3>CTRL-EVENT-NETWORK-NOT-FOUND
// <3>CTRL-EVENT-SSID-TEMP-DISABLED id=28 ssid="AEX-Peter" auth_failures=1 duration=10 reason=WRONG_KEY
// <3>WPA: 4-Way Handshake failed - pre-shared key may be incorrect
//
//  Wait for result of sending command - it should be "OK"
    for(i=0; i<=WAIT_CYCLES; i++)
    {
        memset(buf, 0, BUF_LEN*sizeof(char));
        res = sock.recv(buf, BUF_LEN, MSG_DONTWAIT);
        std::this_thread::sleep_for(WAIT_TIME);
        if(res>0)
        {
            LOG_S(INFO) << "wpaCtrlCmd received data: " << buf;
            if ( (res > 0 && buf[0] == '<') || (res > 6 && strncmp(buf, "IFNAME=", 7) == 0) )
                /* This is an unsolicited message from wpa_supplicant, not the reply to the request */
                /* See wpa_ctrl.c:559 */
                continue;
            else
                break;
        }
    }
    if( (i==WAIT_CYCLES+1) ||
        ((res>8) && (strncmp(buf, "FAIL-BUSY", 9)==0)) ||
        (!((res>1) && (strncmp(buf, "OK", 2)==0))) )
    {
        sock.close();
        unlink(cliSockAddr.c_str());
        return JSON_RESULT_ERR;
    }

//  Wait for result of command execution - it should be RESULT_CONNECTED in case of success
    for(i=0; i<=2*WAIT_CYCLES; i++)
    {
        memset(buf, 0, BUF_LEN*sizeof(char));
        res = sock.recv(buf, BUF_LEN, MSG_DONTWAIT);
        std::this_thread::sleep_for(WAIT_TIME);
        if(res>0)
        {
            LOG_S(INFO) << "wpaCtrlCmd received data: " << buf;
            if ( (res>(int)RESULT_CONNECTED.length()) && (buf[0]=='<') && (strstr(buf, RESULT_CONNECTED.c_str())!=nullptr) )
            {
                conn_status = RESULT_CONNECTED;
                break;
            }
            if ( (res>(int)RESULT_NOT_FOUND.length()) && (buf[0]=='<') && (strstr(buf, RESULT_NOT_FOUND.c_str())!=nullptr) )
            {
                conn_status = RESULT_NOT_FOUND;
                break;
            }
            if ( (res>(int)RESULT_HANDSHAKE_FAILED.length()) && (buf[0]=='<') && (strstr(buf, RESULT_HANDSHAKE_FAILED.c_str())!=nullptr) )
            {
                conn_status = RESULT_HANDSHAKE_FAILED;
                break;
            }
            if ( (res>(int)RESULT_TEMP_DISABLED.length()) && (buf[0]=='<') && (strstr(buf, RESULT_TEMP_DISABLED.c_str())!=nullptr) )
            {
                // Is it really the response for our request?
                ideq = "id="+netid;
                if(strstr(buf, ideq.c_str())!=nullptr)
                {
                    conn_status = RESULT_TEMP_DISABLED;
                    break;
                }
            }
        }
    }
    sock.close();
    unlink(cliSockAddr.c_str());
    if(i==2*WAIT_CYCLES+1)
        return JSON_RESULT_ERR;

    if(conn_status == RESULT_CONNECTED)
//  Are we really connected to the requested network?
    {
        if(!wpaCtrlCmd(COMMAND_STATUS, ifname))
            return JSON_RESULT_ERR;
        strncpy(line, "id=", MAXLINE);
        strncat(line, netid.c_str(), MAXLINE);
        ptr2 = searchLineInBuf(line);
        if(ptr2==nullptr)
            strResult = JSON_PARAM_ERR;
        else
        {
            idFound = true;
            ptr2[strlen(ptr2)]='\n';
        }
        strncpy(line, "wpa_state=COMPLETED", MAXLINE);
        ptr2 = searchLineInBuf(line);
        if(ptr2==nullptr)
            strResult = JSON_PARAM_ERR;
        else
            if(idFound)
                strResult = JSON_PARAM_SUCC;

        return getJsonFromBufLines(strResult);
    }
    else if(conn_status == RESULT_NOT_FOUND)
//  The network is present in WPA configuration but is not available now
    {
        return JSON_RESULT_NOTAVAIL;
    }
    else if(conn_status == RESULT_HANDSHAKE_FAILED)
//  PSK incorrect
    {
        return JSON_RESULT_AUTH_FAILED;
    }
    else if(conn_status == RESULT_TEMP_DISABLED)
    {
        if(strstr(buf, RESULT_WRONG_KEY.c_str())!=nullptr)
//          PSK incorrect
            return JSON_RESULT_AUTH_FAILED;
        else
//          Should not happen
            return JSON_RESULT_ERR;
    }
    else
        return JSON_RESULT_ERR;
}

bool NmWorkerWpa::wpaCtrlCmd(std::string strCmd, std::string strWait, std::string ifname)
{
    int res,i;
    sockpp::unix_dgram_socket sock;
    const std::string srv_sock = srvSockAddrDir + ifname;

    if (!sock.bind(sockpp::unix_address(cliSockAddr)))
    {
        LOG_S(ERROR) << "wpaCtrlCmd cannot connect to client socket " << cliSockAddr << " : " << sock.last_error_str();
        return false;
    }

    if (!sock.connect(sockpp::unix_address(srv_sock)))
    {
        LOG_S(ERROR) << "wpaCtrlCmd cannot connect to server socket " << srv_sock << " : " << sock.last_error_str();
        sock.close();
        return false;
    }

    LOG_S(INFO) << "wpaCtrlCmd sending command: " << COMMAND_ATTACH;
    if (sock.send(COMMAND_ATTACH) != ssize_t(COMMAND_ATTACH.length()))
    {
        LOG_S(ERROR) << "wpaCtrlCmd cannot write to socket: " << sock.last_error_str();
        sock.close();
        return false;
    }

    LOG_S(INFO) << "wpaCtrlCmd sending command: " << strCmd;
    if (sock.send(strCmd) != ssize_t(strCmd.length()))
    {
        LOG_S(ERROR) << "wpaCtrlCmd cannot write to socket: " << sock.last_error_str();
        sock.close();
        return false;
    }

    for(i=0; i<=WAIT_CYCLES; i++)
    {
        memset(buf, 0, BUF_LEN*sizeof(char));
        res = sock.recv(buf, BUF_LEN, MSG_DONTWAIT);
        std::this_thread::sleep_for(WAIT_TIME);
        if(res>0)
        {
            LOG_S(INFO) << "wpaCtrlCmd received data: " << buf;
            if ( (res > 0 && buf[0] == '<') || (res > 6 && strncmp(buf, "IFNAME=", 7) == 0) )
                /* This is an unsolicited message from wpa_supplicant, not the reply to the request */
                /* See wpa_ctrl.c:559 */
                continue;
            else
                break;
        }
    }
    if(i==WAIT_CYCLES+1)
        return false;

    if( (res>8) && (strncmp(buf, "FAIL-BUSY", 9)==0) )
    {
        sock.close();
        unlink(cliSockAddr.c_str());
        return false;
    }

    if( !((res>1) && (strncmp(buf, "OK", 2)==0)) )
    {
        sock.close();
        unlink(cliSockAddr.c_str());
        return false;
    }

    for(i=0; i<=2*WAIT_CYCLES; i++)
    {
        memset(buf, 0, BUF_LEN*sizeof(char));
        res = sock.recv(buf, BUF_LEN, MSG_DONTWAIT);
        std::this_thread::sleep_for(WAIT_TIME);
        if(res>0)
        {
            LOG_S(INFO) << "wpaCtrlCmd received data: " << buf;
            if ( (res>(int)strWait.length()) && (buf[0]=='<') && (strstr(buf, strWait.c_str())!=nullptr) )
                break;
        }
    }
    sock.close();
    unlink(cliSockAddr.c_str());
    if(i==2*WAIT_CYCLES+1)
        return false;

    return true;
}

json NmWorkerWpa::execCmdWpaStatus(NmCommandData* pcmd)
{
    std::string strCmd = COMMAND_STATUS;
    json cmd = {};
    std::string ifname = "";

    ifname = getStringParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
    if(ifname.empty())
        return JSON_RESULT_ERR;

    if(!wpaCtrlCmd(strCmd, ifname))
        return JSON_RESULT_ERR;

    return getJsonFromBufLines(JSON_PARAM_SUCC);
}

std::string NmWorkerWpa::getStringParamFromCommand(NmCommandData* pcmd, std::string param)
// Returns empty string if the parameter was not found
{
    std::string param_value = "";
    json cmd = {};

    try {
        cmd = pcmd->getJsonData();
        param_value = cmd[JSON_PARAM_DATA][param];
    } catch (std::exception& e) {
        LOG_S(WARNING) << "Exception in getStringParamFromCommand - cannot get parameter " << param << " from json";
    }

    return param_value;
}

bool NmWorkerWpa::getBoolParamFromCommand(NmCommandData* pcmd, std::string param)
// Returns FALSE if the parameter was not found
{
    bool param_value = false;
    json cmd = {};

    try {
        cmd = pcmd->getJsonData();
        param_value = cmd[JSON_PARAM_DATA][param];
    } catch (std::exception& e) {
        LOG_S(WARNING) << "Exception in getBoolParamFromCommand - cannot get parameter " << param << " from json";
    }

    return param_value;
}

int NmWorkerWpa::getIntParamFromCommand(NmCommandData* pcmd, std::string param)
// Returns -1 if the parameter was not found
{
    int param_value = -1;
    json cmd = {};

    try {
        cmd = pcmd->getJsonData();
        param_value = cmd[JSON_PARAM_DATA][param];
    } catch (std::exception& e) {
        LOG_S(WARNING) << "Exception in getIntParamFromCommand - cannot get parameter " << param << " from json";
    }

    return param_value;
}

/*
TODO: https://www.freebsd.org/cgi/man.cgi?wpa_supplicant.conf(5) https://wiki.netbsd.org/tutorials/how_to_use_wpa_supplicant/

WEP:
key_mgmt=NONE
wep_tx_keyidx=0 OR wep_tx_keyidx=1
wep_key0=42FEEDDEAFBABEDEAFBEEFAA55 (HEX key)
wep_key1="FreeBSDr0cks!" (ASCII key)

OPEN NETWORK:
key_mgmt=NONE

WPA-EAP:
key_mgmt=WPA-EAP
eap=PEAP OR eap=TTLS
identity="user@example.com"
password="foobar"
anonymous_identity="anonymous@example.com" OPTIONAL FOR eap=TTLS
ca_cert="/etc/cert/ca.pem"
phase1=...
phase2=...
*/
json NmWorkerWpa::execCmdWpaAdd(NmCommandData* pcmd)
{
    std::string strBuf;
    int new_id = 0;
    char* ptr1;
    const char DELIM_HEAD = '/';
    const char DELIM_LINES = '\t';
    std::vector<std::string> columns;
    std::stringstream ss;
    std::string column;
    std::string line;
    std::string_view column_view;
    int num_columns = 0;
    int j=0;
    json jline {};
    json jdata {};
    json jret {};

    std::string ifname = getStringParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
    if(ifname.empty())
        return JSON_RESULT_ERR;

    std::string ssid = getStringParamFromCommand(pcmd, JSON_PARAM_SSID);
    std::string bssid = getStringParamFromCommand(pcmd, JSON_PARAM_BSSID);
    if(ssid.empty() && bssid.empty())
    {
        LOG_S(ERROR) << "SSID or BSSID (or both) is required to add network";
        return JSON_RESULT_ERR;
    }

    std::string psk = getStringParamFromCommand(pcmd, JSON_PARAM_PSK);

    std::string strCmd = COMMAND_LIST;
    if(!wpaCtrlCmd(strCmd, ifname))
        return JSON_RESULT_ERR;

    if( !bssid.empty() && strstr(buf, bssid.c_str())!=nullptr )
    {
        LOG_S(ERROR) << "BSSID already exists in the configuration";
        return JSON_RESULT_EXISTS;
    }
    if( !ssid.empty() && strstr(buf, ssid.c_str())!=nullptr && bssid.empty() )
    {
        LOG_S(ERROR) << "SSID already exists in the configuration";
        return JSON_RESULT_EXISTS;
    }

    strCmd = COMMAND_ADD;
    if(!wpaCtrlCmd(strCmd, ifname))
        return JSON_RESULT_ERR;

    strBuf = std::string(buf);
    try {
        new_id = std::stoi(strBuf);
    } catch (std::exception& e) {
        LOG_S(ERROR) << "Cannot add network: " << e.what() << " " << strBuf;
        return JSON_RESULT_ERR;
    }

    if(!ssid.empty())
    {
        if(!setNetworkParam(ifname, new_id, "ssid", ssid, true))
        {
            removeNetwork(ifname, new_id);
            return JSON_RESULT_ERR;
        }
    }

    if(!bssid.empty())
    {
        if(!setNetworkParam(ifname, new_id, "bssid", bssid, false))
        {
            removeNetwork(ifname, new_id);
            return JSON_RESULT_ERR;
        }
    }

    if(!psk.empty())
    {
        if(!setNetworkParam(ifname, new_id, "psk", psk, true))
        {
            removeNetwork(ifname, new_id);
            return JSON_RESULT_ERR;
        }
    }

    strCmd = COMMAND_LIST;
    if(!wpaCtrlCmd(strCmd, ifname))
        return JSON_RESULT_ERR;

    if(!bssid.empty())
    {
        ptr1 = searchLineInBuf(bssid.c_str());
    }
    else
    {
        ptr1 = searchLineInBuf(ssid.c_str());
    }
    if(ptr1==nullptr)
    {
        LOG_S(ERROR) << "execCmdWpaAdd cannot find added network in running configuration";
        return JSON_RESULT_ERR;
    }

    ss = std::stringstream(buf);
    getline (ss, line);
    ss = std::stringstream(line);
    while (getline (ss, column, DELIM_HEAD))
    {
        column_view = column;
        column_view.remove_prefix(std::min(column.find_first_not_of(" "), column.size()));
        column_view.remove_suffix(std::min(column.size() - column.find_last_not_of(" ") - 1, column.size()));
        columns.push_back(static_cast<std::string>(column_view));
    }
    num_columns = columns.size();

    ss = std::stringstream(ptr1);
    j=0;
    jline = {};
    while (getline (ss, column, DELIM_LINES))
    {
        jline[columns[j]] = column;
        j++;
        if(j>=num_columns)
            break;
    }
    jdata[JSON_PARAM_NETWORK] = jline;
    jret[JSON_PARAM_RESULT] = JSON_PARAM_SUCC;
    jret[JSON_PARAM_DATA] = jdata;

    return jret;
}

char* NmWorkerWpa::searchLineInBuf(const char* mask)
{
    char endline = '\n';
    char* ptr1 = strstr(buf, mask);
    if(ptr1==nullptr)
    {
        LOG_S(WARNING) << "searchLineInBuf cannot find " << mask << " in buf";
        return ptr1;
    }
    while(true)
    {
        if(ptr1==buf)
            break;
        ptr1--;
        if(ptr1[0]==endline)
            break;
    }
    ptr1++;
    char* ptr2=ptr1+1;
    while(true)
    {
        if(ptr2[0]==0)
            break;
        if(ptr2[0]==endline)
            break;
        ptr2++;
    }
    ptr2[0] = 0;
    return ptr1;
}

bool NmWorkerWpa::setNetworkParam(std::string ifname, int id, std::string param_name, std::string param, bool quotes)
{
    std::string strCmd;
    if(quotes)
        strCmd = COMMAND_SET + " " + std::to_string(id) + " " + param_name + " \"" + param + "\"";
    else
        strCmd = COMMAND_SET + " " + std::to_string(id) + " " + param_name + " " + param;
    if(!wpaCtrlCmd(strCmd, ifname))
        return JSON_RESULT_ERR;
    if(!(strncmp(buf, "OK", 2)==0) )
    {
        LOG_S(ERROR) << "setNetworkParam cannot set " << param_name << " to " << param << " for network " << id << " : " << buf;
        return false;
    }
    return true;
}

json NmWorkerWpa::execCmdWpaRemove(NmCommandData* pcmd)
{
    std::string ssid = "";
    std::string bssid = "";
    std::string netid;
    char* ptr1 = nullptr;
    char* ptr2 = nullptr;
    int id = -1;

    std::string ifname = getStringParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
    if(ifname.empty())
        return JSON_RESULT_ERR;

    id = getIntParamFromCommand(pcmd, JSON_PARAM_NETID);
    if(id>=0)
        netid = std::to_string(id);

    if(netid.empty())
    {
        ssid = getStringParamFromCommand(pcmd, JSON_PARAM_SSID);
        bssid = getStringParamFromCommand(pcmd, JSON_PARAM_BSSID);
        if(ssid.empty() && bssid.empty())
        {
            LOG_S(ERROR) << "NETID or SSID or BSSID is required to remove network";
            return JSON_RESULT_ERR;
        }
    }

    std::string strCmd = COMMAND_LIST;
    if(!wpaCtrlCmd(strCmd, ifname))
        return JSON_RESULT_ERR;

    if(!netid.empty())
    {
        ptr2 = (char*)netid.c_str();
        ptr1 = searchLineInBuf(netid.c_str());
    }
    else if(!bssid.empty())
    {
        ptr2 = (char*)bssid.c_str();
        ptr1 = searchLineInBuf(bssid.c_str());
    }
    else if(!ssid.empty())
    {
        ptr2 = (char*)ssid.c_str();
        ptr1 = searchLineInBuf(ssid.c_str());
    }

    if(ptr1==nullptr)
    {
        LOG_S(ERROR) << "execCmdWpaRemove cannot find network " << ptr2 << " in running configuration";
        return JSON_RESULT_ERR;
    }

    if(netid.empty())
    {
        ptr2 = strstr(ptr1, "\t");
        try {
            id = std::stoi(ptr1);
        } catch (std::exception& e) {
            LOG_S(ERROR) << "execCmdWpaRemove cannot convert NETID to integer: " << e.what();
            return JSON_RESULT_ERR;
        }
        if( (ptr2==nullptr) || (id<0) )
        {
            LOG_S(ERROR) << "execCmdWpaRemove received incorrect data from wpa daemon - cannot find network ID in: " << ptr1;
            return JSON_RESULT_ERR;
        }
        ptr2[0]=0;
    }

    if(!removeNetwork(ifname, id))
        return JSON_RESULT_ERR;

    return JSON_RESULT_SUCCESS;
}

bool NmWorkerWpa::removeNetwork(std::string ifname, int idnet)
{
    std::string strCmd = COMMAND_REMOVE + " " + std::to_string(idnet);
    if(!wpaCtrlCmd(strCmd, ifname))
        return false;
    if(!(strncmp(buf, "OK", 2)==0) )
    {
        LOG_S(ERROR) << "removeNetwork cannot remove network " << idnet;
        return false;
    }
    return true;
}

json NmWorkerWpa::execCmdWpaConnect(NmCommandData* pcmd)
{
    std::string ssid = "";
    std::string bssid = "";
    std::string strSearch = "";
    std::string strResult = JSON_PARAM_SUCC;
    bool idFound = false;
    bool isConnected = false;
    const int MAXLINE = 32;
    char* ptr1 = nullptr;
    char* ptr2 = nullptr;
    std::string netid;
    int id = -1;

    char line[MAXLINE];
    memset(line, 0, MAXLINE*sizeof(char));

    std::string ifname = getStringParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
    if(ifname.empty())
        return JSON_RESULT_ERR;

    id = getIntParamFromCommand(pcmd, JSON_PARAM_NETID);
    if(id>=0)
        netid = std::to_string(id);

    if(netid.empty())
    {
        ssid = getStringParamFromCommand(pcmd, JSON_PARAM_SSID);
        bssid = getStringParamFromCommand(pcmd, JSON_PARAM_BSSID);
        if(ssid.empty() && bssid.empty())
        {
            LOG_S(ERROR) << "NETID or SSID or BSSID is required to connect to network";
            return JSON_RESULT_ERR;
        }
    }

    std::string strCmd = COMMAND_STATUS;
    if(!wpaCtrlCmd(strCmd, ifname))
        return JSON_RESULT_ERR;
    ptr1 = searchLineInBuf("wpa_state=COMPLETED");
    if(ptr1!=nullptr)
    {
        isConnected = true;
        ptr1[strlen(ptr1)]='\n';
        if(!netid.empty())
        {
            strSearch = "id="+netid+"\n";
            ptr1=searchLineInBuf(strSearch.c_str());
            if(ptr1!=nullptr)
            {
                LOG_S(WARNING) << "Already connected to network " << netid;
                ptr1[strlen(ptr1)]='\n';
                return getJsonFromBufLines(JSON_PARAM_SUCC);
            }
        }
        else if(!bssid.empty())
        {
            strSearch = "bssid="+bssid;
            ptr1=searchLineInBuf(strSearch.c_str());
            if(ptr1!=nullptr)
            {
                LOG_S(WARNING) << "Already connected to network " << bssid;
                ptr1[strlen(ptr1)]='\n';
                return getJsonFromBufLines(JSON_PARAM_SUCC);
            }

        }
        else if(!ssid.empty())
        {
            strSearch = "ssid="+ssid;
            ptr1=searchLineInBuf(strSearch.c_str());
            if( (ptr1!=nullptr) && (std::string(ptr1)==strSearch) )
            {
                LOG_S(WARNING) << "Already connected to network " << ssid;
                ptr1[strlen(ptr1)]='\n';
                return getJsonFromBufLines(JSON_PARAM_SUCC);
            }
        }
    }

    if(netid.empty())
    {
        strCmd = COMMAND_LIST;
        if(!wpaCtrlCmd(strCmd, ifname))
            return JSON_RESULT_ERR;

        if(!bssid.empty())
        {
            ptr2 = (char*)bssid.c_str();
            ptr1 = searchLineInBuf(bssid.c_str());
        }
        else if(!ssid.empty())
        {
            ptr2 = (char*)ssid.c_str();
            ptr1 = searchLineInBuf(ssid.c_str());
        }

        if(ptr1==nullptr)
        {
            LOG_S(ERROR) << "execCmdWpaConnect cannot find network " << ptr2 << " in running configuration";
            return JSON_RESULT_NOTFOUND;
        }

        ptr2 = strstr(ptr1, "\t");
        try {
            id = std::stoi(ptr1);
        } catch (std::exception& e) {
            LOG_S(ERROR) << "execCmdWpaConnect cannot convert NETID to integer: " << e.what();
            return JSON_RESULT_ERR;
        }

        if( (ptr2==nullptr) || (id<0) )
        {
            LOG_S(ERROR) << "execCmdWpaConnect received incorrect data from wpa daemon - cannot find network ID in: " << ptr1;
            return JSON_RESULT_ERR;
        }
        ptr2[0]=0;
        netid = std::to_string(id);
    }
    return wpaConnectCmd(id, ifname);
/*
    strCmd = COMMAND_SELECT + " " + netid;
    if(!wpaCtrlCmd(strCmd, RESULT_CONNECTED, ifname))
    {
        LOG_S(WARNING) << "execCmdWpaConnect did not receive CTRL-EVENT-CONNECTED, trying to refresh status...";
        strResult = JSON_PARAM_ERR;
    }
    strCmd = COMMAND_STATUS;
    if(!wpaCtrlCmd(strCmd, ifname))
        return JSON_RESULT_ERR;

    strncpy(line, "id=", MAXLINE);
    strncat(line, netid.c_str(), MAXLINE);
    ptr2 = searchLineInBuf(line);
    if(ptr2==nullptr)
        strResult = JSON_PARAM_ERR;
    else
    {
        idFound = true;
        ptr2[strlen(ptr2)]='\n';
    }
    strncpy(line, "wpa_state=COMPLETED", MAXLINE);
    ptr2 = searchLineInBuf(line);
    if(ptr2==nullptr)
        strResult = JSON_PARAM_ERR;
    else
        if(idFound)
            strResult = JSON_PARAM_SUCC;

    return getJsonFromBufLines(strResult);
*/
}

json NmWorkerWpa::execCmdWpaDisconnect(NmCommandData* pcmd)
{
    std::string ifname = getStringParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
    if(ifname.empty())
        return JSON_RESULT_ERR;

    std::string strCmd = COMMAND_DISCONNECT;
    if(!wpaCtrlCmd(strCmd, ifname))
        return JSON_RESULT_ERR;

    if(!(strncmp(buf, "OK", 2)==0) )
        return JSON_RESULT_ERR;

    strCmd = COMMAND_STATUS;
    if(!wpaCtrlCmd(strCmd, ifname))
        return JSON_RESULT_ERR;

    return getJsonFromBufLines(JSON_PARAM_SUCC);
}

json NmWorkerWpa::execCmdWpaReassoc(NmCommandData* pcmd)
{
    const int MAXLINE = 32;
    char line[MAXLINE];
    char* ptr2 = nullptr;
    std::string strResult = "";

    std::string ifname = getStringParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
    if(ifname.empty())
        return JSON_RESULT_ERR;

    std::string strCmd = COMMAND_REASSOCIATE;

    if(!wpaCtrlCmd(strCmd, "CTRL-EVENT-CONNECTED", ifname))
    {
        LOG_S(WARNING) << "execCmdWpaReassoc did not receive CTRL-EVENT-CONNECTED, trying to refresh status...";
        strResult = JSON_PARAM_ERR;
    }

    strCmd = COMMAND_STATUS;
    if(!wpaCtrlCmd(strCmd, ifname))
        return JSON_RESULT_ERR;

    strncpy(line, "wpa_state=COMPLETED", MAXLINE);
    ptr2 = searchLineInBuf(line);
    if(ptr2==nullptr)
        strResult = JSON_PARAM_ERR;
    else
        strResult = JSON_PARAM_SUCC;

    return getJsonFromBufLines(strResult);
}

json NmWorkerWpa::execCmdWpaSetPsk(NmCommandData* pcmd)
{
    int id = -1;

    std::string ifname = getStringParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
    if(ifname.empty())
        return JSON_RESULT_ERR;

    std::string psk = getStringParamFromCommand(pcmd, JSON_PARAM_PSK);
    if(psk.empty())
        return JSON_RESULT_ERR;

    id = getIntParamFromCommand(pcmd, JSON_PARAM_NETID);
    if(id<0)
        return JSON_RESULT_ERR;

    if(!setNetworkParam(ifname, id, "psk", psk, true))
    {
        return JSON_RESULT_ERR;
    }

    return JSON_RESULT_SUCCESS;
}

json NmWorkerWpa::execCmdWpaSetNetParam(NmCommandData* pcmd)
{
    int id = -1;

    std::string ifname = getStringParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
    if(ifname.empty())
        return JSON_RESULT_ERR;

    std::string param_name = getStringParamFromCommand(pcmd, JSON_PARAM_NET_PARAM_NAME);
    if(param_name.empty())
        return JSON_RESULT_ERR;

    std::string param_value = getStringParamFromCommand(pcmd, JSON_PARAM_NET_PARAM_VALUE);
    if(param_value.empty())
        return JSON_RESULT_ERR;

    bool param_quotes = getBoolParamFromCommand(pcmd, JSON_PARAM_NET_PARAM_QUOTES);

    id = getIntParamFromCommand(pcmd, JSON_PARAM_NETID);
    if(id<0)
        return JSON_RESULT_ERR;

    if(!setNetworkParam(ifname, id, param_name, param_value, param_quotes))
    {
        return JSON_RESULT_ERR;
    }

    return JSON_RESULT_SUCCESS;
}
