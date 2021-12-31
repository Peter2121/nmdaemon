#include "wpa_worker.h"

wpa_worker::wpa_worker() :
    cliSockAddr(mktemp((char*)csaPrefix.c_str())),
    srvSockAddrDir(defSsaDir)
{
    buf = new char[BUF_LEN]();
}

wpa_worker::wpa_worker(std::string ssad) :
    cliSockAddr(mktemp((char*)csaPrefix.c_str())),
    srvSockAddrDir()
{
    buf = new char[BUF_LEN]();
    if(ssad.compare(ssad.length()-1,1,"/")==0)
        srvSockAddrDir = ssad;
    else
        srvSockAddrDir = ssad +"/";
}

wpa_worker::~wpa_worker()
{
    delete buf;
}

NmScope wpa_worker::getScope()
{
    return NmScope::WPA;
}

json wpa_worker::execCmd(nmcommand_data* pcmd)
{
    switch (pcmd->getCommand().cmd)
    {
        case NmCmd::WPA_LIST_IF :
            return execCmdWpaListIf(pcmd);
        case NmCmd::WPA_LIST :
            return execCmdWpaList(pcmd);
        case NmCmd::WPA_SCAN :
            return execCmdWpaScan(pcmd);
        case NmCmd::WPA_STATUS :
            return execCmdWpaStatus(pcmd);
        case NmCmd::WPA_SETPSK :
            return execCmdWpaSetPsk(pcmd);
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

bool wpa_worker::isValidCmd(nmcommand_data* pcmd)
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

bool wpa_worker::isValidWpaIf(std::string ifn)
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

json wpa_worker::execCmdWpaListIf(nmcommand_data*)
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

json wpa_worker::getJsonFromBufLines(std::string result)
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

json wpa_worker::getJsonFromBufTable(std::string param_name)
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

json wpa_worker::execCmdWpaList(nmcommand_data* pcmd)
{
    std::string strCmd;
    json cmd = {};
    std::string ifname = "";

    ifname = getParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
    if(ifname.empty())
        return JSON_RESULT_ERR;

    strCmd = COMMAND_LIST;
    if(!wpaCtrlCmd(strCmd, ifname))
        return JSON_RESULT_ERR;

    return getJsonFromBufTable(JSON_PARAM_NETWORKS);
}

json wpa_worker::execCmdWpaScan(nmcommand_data* pcmd)
{
    sockpp::unix_dgram_socket sock;
    std::string strCmd;
    json cmd = {};
    std::string ifname = "";

    ifname = getParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
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

bool wpa_worker::wpaCtrlCmd(const std::string strCmd, const std::string ifname)
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

bool wpa_worker::wpaCtrlCmd(std::string strCmd, std::string strWait, std::string ifname)
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

json wpa_worker::execCmdWpaStatus(nmcommand_data* pcmd)
{
    std::string strCmd = COMMAND_STATUS;
    json cmd = {};
    std::string ifname = "";

    ifname = getParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
    if(ifname.empty())
        return JSON_RESULT_ERR;

    if(!wpaCtrlCmd(strCmd, ifname))
        return JSON_RESULT_ERR;

    return getJsonFromBufLines(JSON_PARAM_SUCC);
}

std::string wpa_worker::getParamFromCommand(nmcommand_data* pcmd, std::string param)
{
    std::string param_value = "";
    json cmd = {};

    try {
        cmd = pcmd->getJsonData();
        param_value = cmd[JSON_PARAM_DATA][param];
    } catch (std::exception& e) {
        LOG_S(WARNING) << "Exception in getIfNameFromCommand - cannot get parameter " << param << " from json";
    }

    return param_value;
}

json wpa_worker::execCmdWpaAdd(nmcommand_data* pcmd)
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

    std::string ifname = getParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
    if(ifname.empty())
        return JSON_RESULT_ERR;

    std::string ssid = getParamFromCommand(pcmd, JSON_PARAM_SSID);
    std::string bssid = getParamFromCommand(pcmd, JSON_PARAM_BSSID);
    if(ssid.empty() && bssid.empty())
    {
        LOG_S(ERROR) << "SSID or BSSID (or both) is required to add network";
        return JSON_RESULT_ERR;
    }

    std::string psk = getParamFromCommand(pcmd, JSON_PARAM_PSK);

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

char* wpa_worker::searchLineInBuf(const char* mask)
{
    char endline = '\n';
    char* ptr1 = strstr(buf, mask);
    if(ptr1==nullptr)
    {
        LOG_S(ERROR) << "searchLineInBuf cannot find " << mask << " in buf";
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

bool wpa_worker::setNetworkParam(std::string ifname, int id, std::string param_name, std::string param, bool quotes)
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

json wpa_worker::execCmdWpaRemove(nmcommand_data* pcmd)
{
    std::string ssid = "";
    std::string bssid = "";
    char* ptr1 = nullptr;
    char* ptr2 = nullptr;
    int idnet = 0;

    std::string ifname = getParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
    if(ifname.empty())
        return JSON_RESULT_ERR;

    std::string netid = getParamFromCommand(pcmd, JSON_PARAM_NETID);
    if(netid.empty())
    {
        ssid = getParamFromCommand(pcmd, JSON_PARAM_SSID);
        bssid = getParamFromCommand(pcmd, JSON_PARAM_BSSID);
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
            idnet = std::stoi(ptr1);
        } catch (std::exception& e) {
            LOG_S(ERROR) << "execCmdWpaRemove cannot convert NETID to integer: " << e.what();
            return JSON_RESULT_ERR;
        }
        if( (ptr2==nullptr) || (idnet<0) )
        {
            LOG_S(ERROR) << "execCmdWpaRemove received incorrect data from wpa daemon - cannot find network ID in: " << ptr1;
            return JSON_RESULT_ERR;
        }
        ptr2[0]=0;
    }

    if(!removeNetwork(ifname, idnet))
        return JSON_RESULT_ERR;

    return JSON_RESULT_SUCCESS;
}

bool wpa_worker::removeNetwork(std::string ifname, int idnet)
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

json wpa_worker::execCmdWpaConnect(nmcommand_data* pcmd)
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
    int id = -1;

    char line[MAXLINE];
    memset(line, 0, MAXLINE*sizeof(char));

    std::string ifname = getParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
    if(ifname.empty())
        return JSON_RESULT_ERR;

    std::string netid = getParamFromCommand(pcmd, JSON_PARAM_NETID);
    if(netid.empty())
    {
        ssid = getParamFromCommand(pcmd, JSON_PARAM_SSID);
        bssid = getParamFromCommand(pcmd, JSON_PARAM_BSSID);
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
            if(ptr1!=nullptr)
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
            return JSON_RESULT_ERR;
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
    strCmd = COMMAND_SELECT + " " + netid;
    if(!wpaCtrlCmd(strCmd, "CTRL-EVENT-CONNECTED", ifname))
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
}

json wpa_worker::execCmdWpaDisconnect(nmcommand_data* pcmd)
{
    std::string ifname = getParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
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

json wpa_worker::execCmdWpaReassoc(nmcommand_data* pcmd)
{
    const int MAXLINE = 32;
    char line[MAXLINE];
    char* ptr2 = nullptr;
    std::string strResult = "";

    std::string ifname = getParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
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

json wpa_worker::execCmdWpaSetPsk(nmcommand_data* pcmd)
{
    int id = -1;

    std::string ifname = getParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
    if(ifname.empty())
        return JSON_RESULT_ERR;

    std::string psk = getParamFromCommand(pcmd, JSON_PARAM_PSK);
    if(psk.empty())
        return JSON_RESULT_ERR;

    std::string netid = getParamFromCommand(pcmd, JSON_PARAM_NETID);
    if(netid.empty())
        return JSON_RESULT_ERR;

    try {
        id = std::stoi(netid.c_str());
    } catch (std::exception& e) {
        LOG_S(ERROR) << "execCmdWpaSetPsk cannot convert NETID to integer: " << e.what();
        return JSON_RESULT_ERR;
    }


    if(id>=0)
    {
        if(!setNetworkParam(ifname, id, "psk", psk, true))
        {
            return JSON_RESULT_ERR;
        }
        return JSON_RESULT_SUCCESS;
    }

    LOG_S(ERROR) << "execCmdWpaSetPsk received incorrect NETID: " << id;
    return JSON_RESULT_ERR;
}

