#include "nmworkerwpa.h"

NmWorkerWpa::NmWorkerWpa() :
    cliSockAddr(csaPrefix+std::to_string(Tool::getRandomInt())),
    srvSockAddrDir(defSsaDir)
{
    buf = new char[BUF_LEN]();
}

NmWorkerWpa::NmWorkerWpa(std::string ssad) :
    cliSockAddr(csaPrefix+std::to_string(Tool::getRandomInt())),
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
        case NmCmd::WPA_SELECT :
            return execCmdWpaSelect(pcmd);
        case NmCmd::WPA_DISCONNECT :
            return execCmdWpaDisconnect(pcmd);
        case NmCmd::WPA_REASSOC :
            return execCmdWpaReassoc(pcmd);
        case NmCmd::WPA_ADD :
            return execCmdWpaAdd(pcmd);
        case NmCmd::WPA_REMOVE :
            return execCmdWpaRemove(pcmd);
        case NmCmd::WPA_RESET :
            return execCmdWpaReset(pcmd);
        case NmCmd::WPA_SAVE :
            return execCmdWpaSave(pcmd);
        case NmCmd::WPA_ENABLE :
            return execCmdWpaEnable(pcmd);
        case NmCmd::WPA_DISABLE :
            return execCmdWpaDisable(pcmd);
        case NmCmd::WPA_SET_BSSID :
            return execCmdWpaSetBssid(pcmd);
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
    //const char DELIM_LINES = '\t';
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
        while (getline (ss, column, DELIM_FIELDS))
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
    json ret_list = {};
    std::string ifname = "";
    bool details = false;

    ifname = getStringParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
    if(ifname.empty())
        return JSON_RESULT_ERR;

    strCmd = COMMAND_LIST;
    if(!wpaCtrlCmd(strCmd, ifname))
        return JSON_RESULT_ERR;

    ret_list = getJsonFromBufTable(JSON_PARAM_NETWORKS);
    details = getBoolParamFromCommand(pcmd, JSON_PARAM_DETAILS);
    if(details)
    {
        //getSuppParams(ifname, ret_list);
        getSuppParamsAsync(ifname, ret_list);
    }

    return ret_list;
}

void NmWorkerWpa::getSuppParamsAsync(std::string ifname, json& ret_list)
{
    int total_networks = ret_list[JSON_PARAM_DATA][JSON_PARAM_NETWORKS].size();
    std::string str_id = "";
    int id_network = -1;
    std::vector<async::task<std::vector<std::tuple<std::string, std::string>>>> tasks;
    std::vector<async::task<std::vector<std::tuple<std::string, std::string>>>> result;

    for(int i=0; i<total_networks; i++)
    {
        str_id = ret_list[JSON_PARAM_DATA][JSON_PARAM_NETWORKS][i]["network id"];
        id_network = -1;
        try
        {
            id_network = std::stoi(str_id);
        }
        catch (std::exception& e)
        {
            LOG_S(ERROR) << "getSuppParamsAsync cannot convert network id " << str_id << " to integer: " << e.what();
            continue;
        }
        if(id_network<0)
            continue;
        tasks.push_back( async::spawn([=]() { return GetWpaNetworkParams(ifname, Tool::getRandomInt(), id_network); }) );
        /*
        for (auto param_name : SuppParamNames)
        {
            param_value = getNetworkParam(ifname, id_network, param_name);
            if(!param_value.empty())
                ret_list[JSON_PARAM_DATA][JSON_PARAM_NETWORKS][i][param_name] = param_value;
        }
        */
    }
    try {
        auto all_tasks = when_all(tasks.begin(), tasks.end());
        result = all_tasks.get();
    } catch (WpaCommunicationException& wce) {
        LOG_S(ERROR) << "getSuppParamsAsync cannot communicate with wpa_supplicant: " << wce.what();
    }
    for(auto &rt : result)  // Every rt.get() is params for one network
    {
        auto res = rt.get();
        str_id = "";
        for(auto &p : res)  // Find network id
        {
            if(get<0>(p) == "id")
            {
                str_id = get<1>(p);
                break;
            }
        }
        if(!str_id.empty())
        {
            try
            {
                id_network = std::stoi(str_id);
            }
            catch (std::exception& e)
            {
                continue;
            }
        }
        for(int i=0; i<total_networks; i++) // Find this network in the json to return with params inside
        {
            std::string str_iid = ret_list[JSON_PARAM_DATA][JSON_PARAM_NETWORKS][i]["network id"];
            if(str_iid==str_id)
            {
                for(auto &p : res)  // Put the params into json
                {
                    if(get<0>(p) != "id")
                    {
                        ret_list[JSON_PARAM_DATA][JSON_PARAM_NETWORKS][i][get<0>(p)] = get<1>(p);
                    }
                }
                break;
            }
        }
    }
}

std::vector<std::tuple<std::string, std::string>> NmWorkerWpa::GetWpaNetworkParams(std::string ifname, int rnd_suffix, int net_id)
{
    std::string cliSockAddr = csaPrefix + std::to_string(rnd_suffix);
    std::string srvSockAddrDir = defSsaDir;
    char buf[BUF_LEN];
    sockpp::unix_dgram_socket sock;
    int res,i;
    const std::string srv_sock = srvSockAddrDir + ifname;
    std::string strCmd = "";
    std::vector<std::tuple<std::string, std::string>> net_params;

    if (!sock.bind(sockpp::unix_address(cliSockAddr)))
    {
        std::string error = "GetWpaNetworkParams cannot connect to client socket " + cliSockAddr + " : " + sock.last_error_str();
        std::cout << error  << std::endl;
        throw WpaCommunicationException(error);
    }

    if (!sock.connect(sockpp::unix_address(srv_sock)))
    {
        std::string error = "GetWpaNetworkParams cannot connect to server socket " + srv_sock + " : " + sock.last_error_str();
        std::cout << error  << std::endl;
        sock.close();
        unlink(cliSockAddr.c_str());
        throw WpaCommunicationException(error);
    }

    for (auto &param_name : SuppParamNames)
    {
        strCmd = COMMAND_GET + " " + std::to_string(net_id) + " " + param_name;
        std::cout << "GetWpaNetworkParams sending command: " << strCmd << std::endl;
        if (sock.send(strCmd) != ssize_t(strCmd.length()))
        {
            std::string error = "GetWpaNetworkParams cannot write to socket: " + sock.last_error_str();
            LOG_S(ERROR) << error;
            continue;
        }

        for(i=0; i<=WAIT_CYCLES; i++)
        {
            memset(buf, 0, BUF_LEN*sizeof(char));
            res = sock.recv(buf, BUF_LEN, MSG_DONTWAIT);
            std::this_thread::sleep_for(WAIT_TIME);
            if(res>0)
            {
                std::cout << "GetWpaNetworkParams received data: " << buf << std::endl;
                if ( (res > 0 && buf[0] == '<') || (res > 6 && strncmp(buf, "IFNAME=", 7) == 0) )
                    /* This is an unsolicited message from wpa_supplicant, not the reply to the request */
                    /* See wpa_ctrl.c:559 */
                    continue;
                else
                    break;
            }
        }

        if(i>WAIT_CYCLES)
        {
            std::string error = "GetWpaNetworkParams: timeout waiting for data";
            LOG_S(ERROR) << error;
            continue;
        }
        std::string param_value = std::string(buf);
        net_params.push_back(std::make_tuple(param_name, param_value));
    }
    sock.close();
    unlink(cliSockAddr.c_str());

    if(net_params.size()>0)
    {
        net_params.push_back(std::make_tuple("id", std::to_string(net_id)));
    }

    return net_params;
}

void NmWorkerWpa::getSuppParams(std::string ifname, json& ret_list)
{
    int total_networks = ret_list[JSON_PARAM_DATA][JSON_PARAM_NETWORKS].size();
    std::string str_id = "";
    int id_network = -1;
    std::string param_value = "";
    for(int i=0; i<total_networks; i++)
    {
        str_id = ret_list[JSON_PARAM_DATA][JSON_PARAM_NETWORKS][i]["network id"];
        id_network = -1;
        try
        {
            id_network = std::stoi(str_id);
        }
        catch (std::exception& e)
        {
            LOG_S(ERROR) << "getSuppParams cannot convert network id " << str_id << " to integer: " << e.what();
            continue;
        }
        if(id_network<0)
            continue;
        for (auto param_name : SuppParamNames)
        {
            param_value = getNetworkParam(ifname, id_network, param_name);
            if(!param_value.empty())
                ret_list[JSON_PARAM_DATA][JSON_PARAM_NETWORKS][i][param_name] = param_value;
        }
    }
}

std::string NmWorkerWpa::getNetworkParam(std::string ifname, int id, std::string param_name)
{
    std::string strCmd = COMMAND_GET + " " + std::to_string(id) + " " + param_name;
    std::string result = "";
    if(!wpaCtrlCmd(strCmd, ifname))
        return result;

    if(strncmp(buf, RESULT_FAIL.c_str(), RESULT_FAIL.length())==0 )
    {
        LOG_S(ERROR) << "getNetworkParam cannot get " << param_name << " for network " << id << " : " << buf;
        return result;
    }

    result = std::string(buf);
    return result;
}

json NmWorkerWpa::execCmdWpaScan(NmCommandData* pcmd)
{
    int res,i;
    std::unique_ptr<WpaSocket> psock = nullptr;
    bool need_reset = false;
    short retry = 0;
    short SCAN_MAX_RETRIES = 3;

    std::string ifname = getStringParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
    if(ifname.empty())
        return JSON_RESULT_ERR;

    const std::string srv_sock = srvSockAddrDir + ifname;

    try {
        psock = std::make_unique<WpaSocket>(cliSockAddr, srv_sock);
    }  catch (std::exception& e) {
        LOG_S(ERROR) << "Cannot create WPA socket: " << cliSockAddr << " / " << srv_sock;
        return JSON_RESULT_ERR;
    }

    LOG_S(INFO) << "execCmdWpaScan sending command: " << COMMAND_ATTACH;
    if (psock->sockSend(COMMAND_ATTACH) != ssize_t(COMMAND_ATTACH.length()))
    {
        LOG_S(ERROR) << "execCmdWpaScan cannot write to socket: " << psock->sockLastError();
        return JSON_RESULT_ERR;
    }

send_scan:
    if(retry>SCAN_MAX_RETRIES)
    {
        LOG_S(ERROR) << "Too many retries to scan networks in execCmdWpaScan";
        return JSON_RESULT_ERR;
    }

    if(!wpaCtrlCmd(psock.get(), COMMAND_SCAN))
    {
        LOG_S(ERROR) << "execCmdWpaScan got error sending " << COMMAND_SCAN;
        return JSON_RESULT_ERR;
    }

    if( strncmp(buf, RESULT_FAIL_BUSY.c_str(), RESULT_FAIL_BUSY.length())==0 )
    {
        need_reset = true;
        goto send_reset;
    }

    if( strncmp(buf, "OK", 2)!=0 )
    {
        // Strange situation, if it happens - we need to understand why (normally we get FAIL-BUSY or OK)
        LOG_S(ERROR) << "execCmdWpaScan got unexpected data: " << buf;
        return JSON_RESULT_ERR;
    }

    std::this_thread::sleep_for(WAIT_TIME);
    for(i=0; i<=2*WAIT_CYCLES; i++)
    {
        memset(buf, 0, BUF_LEN*sizeof(char));
        res = psock->sockReceiveDontwait(buf, BUF_LEN);
        std::this_thread::sleep_for(WAIT_TIME);
        if(res>0)
        {
            LOG_S(INFO) << "execCmdWpaScan received data: " << buf;

            if ( (res>(int)RESULT_FAIL_BUSY.length()) && (strstr(buf, RESULT_FAIL_BUSY.c_str())!=nullptr) )
            {
                need_reset = true;
                break;
            }

            if ( (res>(int)RESULT_REJECT_SCAN.length()) && (buf[0]=='<') && (strstr(buf, RESULT_REJECT_SCAN.c_str())!=nullptr) )
            {
                need_reset = true;
                break;
            }

            if ( (res>(int)RESULT_FAILED_SCAN.length()) && (buf[0]=='<') && (strstr(buf, RESULT_FAILED_SCAN.c_str())!=nullptr) )
            {
                need_reset = true;
                break;
            }

            if ( (res>(int)RESULT_SCAN_RESULTS.length()) && (buf[0]=='<') && (strstr(buf, RESULT_SCAN_RESULTS.c_str())!=nullptr) )
                break;
        }
    }

    if( !need_reset )    // OK, anyway, try to get scan results
    {
        if(!wpaCtrlCmd(psock.get(), COMMAND_SCAN_RESULTS))
        {
            LOG_S(ERROR) << "execCmdWpaScan got error sending " << COMMAND_SCAN_RESULTS;
            return JSON_RESULT_ERR;
        }
        else
            return getJsonFromBufTable(JSON_PARAM_NETWORKS);
    }

    if(i==2*WAIT_CYCLES+1)  // Strange error - unexpected answer
    {
        LOG_S(ERROR) << "execCmdWpaScan got unexpected data: " << buf;
        return JSON_RESULT_ERR;
    }

send_reset:
    if(!wpaCtrlCmd(psock.get(), COMMAND_FLUSH))
    {
        LOG_S(ERROR) << "execCmdWpaScan got error sending " << COMMAND_FLUSH;
        return JSON_RESULT_ERR;
    }

    if(!wpaCtrlCmd(psock.get(), COMMAND_RECONFIGURE))
    {
        LOG_S(ERROR) << "execCmdWpaScan got error sending " << COMMAND_RECONFIGURE;
        return JSON_RESULT_ERR;
    }

    std::this_thread::sleep_for(WAIT_RECONFIGURE_TIME);

    for(i=0; i<=2*WAIT_CYCLES; i++)     // Probably, we get scan results here
    {
        memset(buf, 0, BUF_LEN*sizeof(char));
        res = psock->sockReceiveDontwait(buf, BUF_LEN);
        std::this_thread::sleep_for(WAIT_TIME);
        if(res>0)
        {
            LOG_S(INFO) << "execCmdWpaScan received data: " << buf;
            if ( (res>(int)RESULT_SCAN_RESULTS.length()) && (buf[0]=='<') && (strstr(buf, RESULT_SCAN_RESULTS.c_str())!=nullptr) )
                break;
        }
    }

    if(i<2*WAIT_CYCLES+1)
    {
        if(!wpaCtrlCmd(psock.get(), COMMAND_SCAN_RESULTS))
        {
            LOG_S(ERROR) << "execCmdWpaScan got error sending " << COMMAND_SCAN_RESULTS;
            return JSON_RESULT_ERR;
        }
        else
            return getJsonFromBufTable(JSON_PARAM_NETWORKS);
    }

    // No results, so try to rescan...
    retry++;
    goto send_scan;
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

bool NmWorkerWpa::wpaCtrlCmd(WpaSocket* psock, const std::string strCmd)
{
    int res,i;

    LOG_S(INFO) << "wpaCtrlCmd sending command: " << strCmd;
    if (psock->sockSend(strCmd) != ssize_t(strCmd.length()))
    {
        LOG_S(ERROR) << "wpaCtrlCmd cannot write to socket: " << psock->sockLastError();
        return false;
    }

    for(i=0; i<=WAIT_CYCLES; i++)
    {
        memset(buf, 0, BUF_LEN*sizeof(char));
        res = psock->sockReceiveDontwait(buf, BUF_LEN);
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
    if(i<=WAIT_CYCLES)
        return true;
    else
        return false;
}

json NmWorkerWpa::wpaConnectCmd(std::string ifname, int id)
{
    std::unique_ptr<WpaSocket> psock = nullptr;
    const std::string srv_sock = srvSockAddrDir + ifname;
    const std::string netid = std::to_string(id);
    int id_connected = -1;
    std::string str_result = JSON_PARAM_SUCC;
    std::string str_found = "";
    bool idFound = false;
    bool status_found = false;

    if(!enableNetwork(ifname, id))
        return JSON_RESULT_ERR;

    if(!wpaCtrlCmd(COMMAND_STATUS, ifname))
    {
        LOG_S(ERROR) << "wpaConnectCmd got error sending " << COMMAND_STATUS;
        return JSON_RESULT_ERR;
    }

    str_found = searchLineInBuf(RESULT_COMPLETED, true);
    if(!str_found.empty())
    {
        str_found = searchLineInBuf("id=", true);
        if(str_found.empty())
        {
            LOG_S(ERROR) << "wpaConnectCmd cannot get currently connected network id from wpa_supplicant status: " << buf;
            return JSON_RESULT_ERR;
        }
        else
        {
            std::string strid = str_found.substr(3, str_found.length()-3);
            try
            {
                id_connected = std::stoi(strid);
            }
            catch (std::exception& e)
            {
                LOG_S(ERROR) << "wpaConnectCmd cannot get currently connected network id from wpa_supplicant: " << strid;
                return JSON_RESULT_ERR;
            }
        }
        if(id_connected<0)
        {
            LOG_S(ERROR) << "wpaConnectCmd cannot get currently connected network id from wpa_supplicant";
            return JSON_RESULT_ERR;
        }
        if(!setNetworkParam(ifname, id_connected, PARAM_PRIORITY, std::to_string(HIGH_PRIORITY), false))
        {
            LOG_S(ERROR) << "wpaConnectCmd cannot set lower priority for network " << id_connected;
            return JSON_RESULT_ERR;
        }
    }

    if(!setNetworkParam(ifname, id, PARAM_PRIORITY, std::to_string(MAX_PRIORITY), false))
    {
        LOG_S(ERROR) << "wpaConnectCmd cannot set maximum priority for network " << id;
        return JSON_RESULT_ERR;
    }

    if(!wpaCtrlCmd(COMMAND_DISCONNECT, ifname))
    {
        LOG_S(ERROR) << "wpaConnectCmd cannot disconnect current newrork";
        return JSON_RESULT_ERR;
    }

    if(!(strncmp(buf, "OK", 2)==0) )
    {
        LOG_S(ERROR) << "wpaConnectCmd received bad status trying to disconnect current newrork: " << buf;
        return JSON_RESULT_ERR;
    }

    if(!wpaCtrlCmd(COMMAND_REASSOCIATE, "Associated with", ifname))
    {
        LOG_S(WARNING) << "wpaConnectCmd did not receive CTRL-EVENT-CONNECTED, trying to connect";
        return JSON_RESULT_ERR;
    }

    if(!wpaCtrlCmd(COMMAND_STATUS, ifname))
    {
        LOG_S(ERROR) << "wpaConnectCmd got error sending " << COMMAND_STATUS;
        return JSON_RESULT_ERR;
    }

    str_found = searchLineInBuf("id="+netid+"\n", true);
    if(str_found.empty())
        str_result = JSON_PARAM_ERR;
    else
    {
        idFound = true;
    }
    str_found = searchLineInBuf(RESULT_COMPLETED, true);
    if( !str_found.empty() && idFound )
    {
        str_result = JSON_PARAM_SUCC;
        return getJsonFromBufLines(str_result);
    }
    else
    {
        str_result = JSON_PARAM_ERR;
//************ Workaround for badly working drivers - reset interface status (ifconfig wlan0 down ; ifconfig wlan0 up)
        int curflags = Tool::getIfFlags(ifname);
        int oldflags = 0;
        int cmdflag=IFF_UP;
        if(curflags==0)
        {
            return JSON_RESULT_ERR;
        }
        oldflags = curflags;
        curflags &= ~cmdflag;
        if(!Tool::setIfFlags(ifname, curflags))
        {
            return JSON_RESULT_ERR;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));
        if(!Tool::setIfFlags(ifname, oldflags))
        {
            return JSON_RESULT_ERR;
        }
        status_found = false;
        for(int i=0; i<WAIT_CYCLES; i++)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));
            if(!wpaCtrlCmd(COMMAND_STATUS, ifname))
            {
                LOG_S(ERROR) << "wpaConnectCmd got error sending " << COMMAND_STATUS;
                return JSON_RESULT_ERR;
            }
            str_found = searchLineInBuf(RESULT_COMPLETED, true);
            if( !str_found.empty() )
            {
                status_found = true;
                break;
            }
        }
        if(!status_found)
            return getJsonFromBufLines(JSON_PARAM_ERR);

        str_found = searchLineInBuf("id="+netid+"\n", true);
        if(str_found.empty())
        {
            str_result = JSON_PARAM_ERR;
        }
        else
        {
            str_result = JSON_PARAM_SUCC;
        }
    }
    return getJsonFromBufLines(str_result);
}

json NmWorkerWpa::wpaSelectCmd(std::string ifname, int id)
{
    std::unique_ptr<WpaSocket> psock = nullptr;
    const std::string srv_sock = srvSockAddrDir + ifname;
    const std::string netid = std::to_string(id);
    std::string strCmd = COMMAND_SELECT + " " + netid;
    std::string str_found = "";
    int res,i;
    //const int MAXLINE = 32;
    //char line[MAXLINE];
//    char* ptr1 = nullptr;
    //char* ptr2 = nullptr;
    std::string ideq;
    std::string conn_status;
    //sockpp::unix_dgram_socket sock;
    json jret = {};
    std::string strResult = JSON_PARAM_SUCC;
    //bool idFound = false;
    //wpaCtrlCmd(strCmd, "CTRL-EVENT-CONNECTED", ifname))

    try {
        psock = std::make_unique<WpaSocket>(cliSockAddr, srv_sock);
    }  catch (std::exception& e) {
        LOG_S(ERROR) << "Cannot create WPA socket: " << cliSockAddr << " / " << srv_sock;
        return JSON_RESULT_ERR;
    }

    LOG_S(INFO) << "wpaSelectCmd sending command: " << COMMAND_ATTACH;
    if (psock->sockSend(COMMAND_ATTACH) != ssize_t(COMMAND_ATTACH.length()))
    {
        LOG_S(ERROR) << "wpaSelectCmd cannot write to socket: " << psock->sockLastError();
        return JSON_RESULT_ERR;
    }

    if(!wpaCtrlCmd(psock.get(), strCmd))
    {
        LOG_S(ERROR) << "wpaSelectCmd got error sending " << strCmd;
        return JSON_RESULT_ERR;
    }

    if( strncmp(buf, RESULT_FAIL_BUSY.c_str(), RESULT_FAIL_BUSY.length())==0 )
    {
        return JSON_RESULT_ERR;
    }

    if( strncmp(buf, "OK", 2)!=0 )
    {
        // Strange situation, if it happens - we need to understand why (normally we get FAIL-BUSY or OK)
        LOG_S(ERROR) << "wpaSelectCmd got unexpected data: " << buf;
        return JSON_RESULT_ERR;
    }

//
// <3>CTRL-EVENT-NETWORK-NOT-FOUND
// <3>CTRL-EVENT-SSID-TEMP-DISABLED id=28 ssid="AEX-Peter" auth_failures=1 duration=10 reason=WRONG_KEY
// <3>WPA: 4-Way Handshake failed - pre-shared key may be incorrect
// <3>Association request to the driver failed
//
//  Wait for result of command execution - it should be RESULT_CONNECTED in case of success
    for(i=0; i<=2*WAIT_CYCLES; i++)
    {
        memset(buf, 0, BUF_LEN*sizeof(char));
        res = psock->sockReceiveDontwait(buf, BUF_LEN);
        std::this_thread::sleep_for(WAIT_CONNECT_TIME);
        if(res>0)
        {
            LOG_S(INFO) << "wpaSelectCmd received data: " << buf;
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
            if ( (res>(int)RESULT_DRIVER_REQUEST_FAILED.length()) && (buf[0]=='<') && (strstr(buf, RESULT_DRIVER_REQUEST_FAILED.c_str())!=nullptr) )
            {
                conn_status = RESULT_DRIVER_REQUEST_FAILED;
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

    if(i==2*WAIT_CYCLES+1)
    {
        return JSON_RESULT_ERR;
    }

    if(conn_status == RESULT_CONNECTED)
//  Are we really connected to the requested network?
    {
        if(!wpaCtrlCmd(psock.get(), COMMAND_STATUS))
        {
            LOG_S(ERROR) << "wpaSelectCmd got error sending " << COMMAND_STATUS;
            return JSON_RESULT_ERR;
        }
        str_found = searchLineInBuf("id="+netid+"\n", true);
        if(!str_found.empty())
        {
            str_found = searchLineInBuf(RESULT_COMPLETED, true);
            if(!str_found.empty())
            {
                return getJsonFromBufLines(JSON_PARAM_SUCC);
            }
        }
        return getJsonFromBufLines(JSON_PARAM_ERR);
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
    else if(conn_status == RESULT_DRIVER_REQUEST_FAILED)
//  Driver request failed
    {
        return JSON_RESULT_DRV_REQ_FAILED;
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

bool NmWorkerWpa::wpaCtrlCmd(WpaSocket* psock, std::string strCmd, std::string strWait)
{
    int res,i;

    LOG_S(INFO) << "wpaCtrlCmd sending command: " << COMMAND_ATTACH;
    if (psock->sockSend(COMMAND_ATTACH) != ssize_t(COMMAND_ATTACH.length()))
    {
        LOG_S(ERROR) << "wpaCtrlCmd cannot write to socket: " << psock->sockLastError();
        return false;
    }

    LOG_S(INFO) << "wpaCtrlCmd sending command: " << strCmd;
    if (psock->sockSend(strCmd) != ssize_t(strCmd.length()))
    {
        LOG_S(ERROR) << "wpaCtrlCmd cannot write to socket: " << psock->sockLastError();
        return false;
    }

    for(i=0; i<=WAIT_CYCLES; i++)
    {
        memset(buf, 0, BUF_LEN*sizeof(char));
        res = psock->sockReceiveDontwait(buf, BUF_LEN);
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

    if( (res>8) && (strncmp(buf, RESULT_FAIL_BUSY.c_str(), RESULT_FAIL_BUSY.length() )==0) )
    {
        return false;
    }

    if( !((res>1) && (strncmp(buf, "OK", 2)==0)) )
    {
        return false;
    }

    for(i=0; i<=2*WAIT_CYCLES; i++)
    {
        memset(buf, 0, BUF_LEN*sizeof(char));
        res = psock->sockReceiveDontwait(buf, BUF_LEN);
        std::this_thread::sleep_for(WAIT_TIME);
        if(res>0)
        {
            LOG_S(INFO) << "wpaCtrlCmd received data: " << buf;
            if ( (res>(int)strWait.length()) && (buf[0]=='<') && (strstr(buf, strWait.c_str())!=nullptr) )
                break;
        }
    }
    if(i==2*WAIT_CYCLES+1)
        return false;

    return true;
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

    if( (res>8) && (strncmp(buf, RESULT_FAIL_BUSY.c_str(), RESULT_FAIL_BUSY.length())==0) )
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
    //char* ptr1;
    std::vector<std::string> columns;
    std::stringstream ss;
    std::string column;
    std::string line;
    std::string str_found = "";
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

    std::string profile = getStringParamFromCommand(pcmd, JSON_PARAM_PROFILE);
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

    if( profile.empty() || (profile==JSON_DATA_WPA_PROFILE_WPA_PSK) || (profile==JSON_DATA_WPA_PROFILE_WPA2_PSK) )
    {
        if(!psk.empty())
        {
            if(!setNetworkParam(ifname, new_id, "psk", psk, true))
            {
                removeNetwork(ifname, new_id);
                return JSON_RESULT_ERR;
            }
        }
    }
    else if(profile==JSON_DATA_WPA_PROFILE_WEP)
    {
        if(!psk.empty())
        {
            if(!setNetworkParam(ifname, new_id, "wep_key0", psk, true))
            {
                removeNetwork(ifname, new_id);
                return JSON_RESULT_ERR;
            }
            /***** This parameter set to 0 is ignored by wpa_supplicant
            if(!setNetworkParam(ifname, new_id, "wep_tx_keyidx", "0", false))
            {
                removeNetwork(ifname, new_id);
                return JSON_RESULT_ERR;
            }
            */
            if(!setNetworkParam(ifname, new_id, "key_mgmt", "NONE", false))
            {
                removeNetwork(ifname, new_id);
                return JSON_RESULT_ERR;
            }
        }
        else
        {
            LOG_S(ERROR) << "Error in execCmdWpaAdd - WEP profile needs PSK to use as network key";
            removeNetwork(ifname, new_id);
            return JSON_RESULT_ERR;
        }
    }
    else if(profile==JSON_DATA_WPA_PROFILE_OPEN)
    {
        if(!setNetworkParam(ifname, new_id, "key_mgmt", "NONE", false))
        {
            removeNetwork(ifname, new_id);
            return JSON_RESULT_ERR;
        }
    }

    if( (profile==JSON_DATA_WPA_PROFILE_WPA_PSK) || (profile==JSON_DATA_WPA_PROFILE_WPA2_PSK) )
    {
        if(!setNetworkParam(ifname, new_id, "key_mgmt", "WPA-PSK", false))
        {
            removeNetwork(ifname, new_id);
            return JSON_RESULT_ERR;
        }
    }

    if(profile==JSON_DATA_WPA_PROFILE_WPA_PSK)
    {
        if(!setNetworkParam(ifname, new_id, "proto", "WPA", false))
        {
            removeNetwork(ifname, new_id);
            return JSON_RESULT_ERR;
        }
    }

    if(profile==JSON_DATA_WPA_PROFILE_WPA2_PSK)
    {
        if(!setNetworkParam(ifname, new_id, "proto", "RSN", false))
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
        str_found = searchLineInBuf(bssid, false);
    }
    else
    {
        str_found = searchLineInBuf(ssid, false);
    }
    if(str_found.empty())
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

    ss = std::stringstream(str_found);
    j=0;
    jline = {};
    while (getline (ss, column, DELIM_FIELDS))
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

std::string NmWorkerWpa::searchLineInBuf(const std::string mask, bool at_start)
{
    std::stringstream bufss(buf);
    std::string line = "";
    while(std::getline(bufss, line, '\n'))
    {
        std::string::size_type pos = line.find(mask);
        if(pos != std::string::npos)
        {
            if(at_start && (pos!=0))
                continue;
            else
                return line;
        }
    }
    return "";
}

/*
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
*/

bool NmWorkerWpa::setNetworkParam(std::string ifname, int id, std::string param_name, std::string param, bool quotes)
{
    std::string strCmd;
    if(quotes)
        strCmd = COMMAND_SET + " " + std::to_string(id) + " " + param_name + " \"" + param + "\"";
    else
        strCmd = COMMAND_SET + " " + std::to_string(id) + " " + param_name + " " + param;
    if(!wpaCtrlCmd(strCmd, ifname))
        return false;
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
    bool net_found = false;
    char* ptr1 = nullptr;
    char* ptr2 = nullptr;
    int id = -1;

    std::string ifname = getStringParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
    if(ifname.empty())
        return JSON_RESULT_ERR;

    id = getIntParamFromCommand(pcmd, JSON_PARAM_NETID);
    if( id >= 0 )
    {
        netid = std::to_string(id);
    }
    else
    {
        ssid = getStringParamFromCommand(pcmd, JSON_PARAM_SSID);
        bssid = getStringParamFromCommand(pcmd, JSON_PARAM_BSSID);
        if(ssid.empty() && bssid.empty())
        {
            LOG_S(ERROR) << "NETID or SSID or BSSID is required to remove network";
            return JSON_RESULT_ERR;
        }
        else
        {
            id = getNetId(ifname, ssid, bssid);
            if( id < 0)
            {
                LOG_S(ERROR) << "Cannot get NETID of network to remove";
                return JSON_RESULT_ERR;
            }
            net_found = true;
            netid = std::to_string(id);
        }
    }

    if(!net_found)
    {
        std::string strCmd = COMMAND_LIST;
        if(!wpaCtrlCmd(strCmd, ifname))
            return JSON_RESULT_ERR;

        std::string str_found = searchLineInBuf(netid+DELIM_FIELDS, true);
        if(str_found.empty())
        {
            LOG_S(ERROR) << "execCmdWpaRemove cannot find network " << netid << " in running configuration";
            return JSON_RESULT_ERR;
        }
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
    std::string str_found = "";
    std::string field = "";
    std::string strResult = JSON_PARAM_SUCC;
    //bool idFound = false;
    const int MAXLINE = 32;
    char* ptr1 = nullptr;
    //char* ptr2 = nullptr;
    std::string netid = "";
    int id = -1;

    char line[MAXLINE];
    memset(line, 0, MAXLINE*sizeof(char));

    std::string ifname = getStringParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
    if(ifname.empty())
        return JSON_RESULT_ERR;

    id = getIntParamFromCommand(pcmd, JSON_PARAM_NETID);
    if(id >= 0)
    {
        netid = std::to_string(id);
    }
    else
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

    str_found = searchLineInBuf(RESULT_COMPLETED, true);
    if(!str_found.empty())
    {
        if(!netid.empty())
        {
            str_found = searchLineInBuf("id="+netid+"\n", true);
            if(!str_found.empty())
            {
                LOG_S(WARNING) << "Already connected to network " << netid;
                return getJsonFromBufLines(JSON_PARAM_SUCC);
            }
        }
        else if(!bssid.empty())
        {
            str_found = searchLineInBuf("bssid="+bssid, true);
            if(!str_found.empty())
            {
                LOG_S(WARNING) << "Already connected to network " << bssid;
                return getJsonFromBufLines(JSON_PARAM_SUCC);
            }
        }
        else if(!ssid.empty())
        {
            str_found = searchLineInBuf("ssid="+ssid, true);
            if( (ptr1!=nullptr) && (str_found==("ssid="+ssid)) )
            {
                LOG_S(WARNING) << "Already connected to network " << ssid;
                return getJsonFromBufLines(JSON_PARAM_SUCC);
            }
        }
    }

    if(netid.empty())
    {
        id = getNetId(ifname, ssid, bssid);
    }
    if( id >= 0 )
    {
        return wpaConnectCmd(ifname, id);
    }
    else
    {
        return JSON_RESULT_ERR;
    }
}

int NmWorkerWpa::getNetId(std::string ifname, std::string ssid, std::string bssid)
{
    int netid = -1;
    std::string str_found = "";
    std::string field = "";

    std::string strCmd = COMMAND_LIST;
    if(!wpaCtrlCmd(strCmd, ifname))
        return JSON_RESULT_ERR;

    if(!bssid.empty())
    {
        str_found = searchLineInBuf(bssid+DELIM_FIELDS, false);
        if(str_found.empty())
        {
            LOG_S(ERROR) << "getNetId cannot find network " << bssid << " in running configuration";
            return netid;
        }
    }
    else if(!ssid.empty())
    {
        str_found = searchLineInBuf(ssid+DELIM_FIELDS, false);
        if(str_found.empty())
        {
            LOG_S(ERROR) << "getNetId cannot find network " << ssid << " in running configuration";
            return netid;
        }
    }

    std::stringstream bufss(str_found);
    std::getline(bufss, field, DELIM_FIELDS);
    if(!field.empty())
    {
        try {
            netid = std::stoi(field);
        } catch (std::exception& e) {
            LOG_S(ERROR) << "getNetId cannot convert NETID " << field << " to integer: " << e.what();
        }

        if( netid < 0 )
        {
            LOG_S(ERROR) << "getNetId received incorrect data from wpa daemon - cannot find network ID in: " << str_found;
        }
    }
    else
    {
        LOG_S(ERROR) << "getNetId received incorrect data from wpa daemon - cannot find network ID in: " << str_found;
    }
    return netid;
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
    //char line[MAXLINE];
    //char* ptr2 = nullptr;
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

    std::string str_found = searchLineInBuf(RESULT_COMPLETED, true);
    if(str_found.empty())
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

json NmWorkerWpa::execCmdWpaReset(NmCommandData* pcmd)
{
    std::string ifname = getStringParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
    if(ifname.empty())
        return JSON_RESULT_ERR;

    return resetWpaStatus(ifname);
}

json NmWorkerWpa::resetWpaStatus(std::string ifname)
{
    std::string strCmd = COMMAND_FLUSH;

    if(!wpaCtrlCmd(strCmd, ifname))
    {
        LOG_S(WARNING) << "execCmdWpaReset did not receive OK for " << strCmd << " command";
        return JSON_RESULT_ERR;
    }

    strCmd = COMMAND_RECONFIGURE;
    if(!wpaCtrlCmd(strCmd, ifname))
    {
        LOG_S(WARNING) << "execCmdWpaReset did not receive OK for " << strCmd << " command";
        return JSON_RESULT_ERR;
    }

    return JSON_RESULT_SUCCESS;
}

json NmWorkerWpa::execCmdWpaSave(NmCommandData* pcmd)
{
    std::string strCmd = COMMAND_SAVE;
    std::string ifname = getStringParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
    if(ifname.empty())
        return JSON_RESULT_ERR;

    if(!wpaCtrlCmd(strCmd, ifname))
    {
        LOG_S(WARNING) << "execCmdWpaSave did not receive OK for " << strCmd << " command";
        return JSON_RESULT_ERR;
    }

    return JSON_RESULT_SUCCESS;
}

json NmWorkerWpa::execCmdWpaEnable(NmCommandData* pcmd)
{
    std::string ifname = getStringParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
    if(ifname.empty())
        return JSON_RESULT_ERR;

    int id = -1;
    id = getIntParamFromCommand(pcmd, JSON_PARAM_NETID);
    if(id<0)
        return JSON_RESULT_ERR;

    if(enableNetwork(ifname, id))
        return JSON_RESULT_SUCCESS;
    else
        return JSON_RESULT_ERR;
}

bool NmWorkerWpa::enableNetwork(std::string ifname, int id)
{
    std::string netid = std::to_string(id);
    if(netid.empty())
        return false;

    std::string strCmd = COMMAND_ENABLE + " " + netid;
    if(!wpaCtrlCmd(strCmd, ifname))
    {
        LOG_S(WARNING) << "execCmdWpaEnable did not receive OK for " << strCmd << " command";
        return false;
    }

    return true;
}

json NmWorkerWpa::execCmdWpaDisable(NmCommandData* pcmd)
{
    std::string netid = "";
    std::string ifname = getStringParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
    if(ifname.empty())
        return JSON_RESULT_ERR;

    int id = -1;
    id = getIntParamFromCommand(pcmd, JSON_PARAM_NETID);
    if(id>=0)
        netid = std::to_string(id);

    if(netid.empty())
        return JSON_RESULT_ERR;

    std::string strCmd = COMMAND_DISABLE + " " + netid;

    if(!wpaCtrlCmd(strCmd, ifname))
    {
        LOG_S(WARNING) << "execCmdWpaDisable did not receive OK for " << strCmd << " command";
        return JSON_RESULT_ERR;
    }

    return JSON_RESULT_SUCCESS;
}

json NmWorkerWpa::execCmdWpaSetBssid(NmCommandData* pcmd)
{
    std::string netid = "";
    std::string ifname = getStringParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
    if(ifname.empty())
        return JSON_RESULT_ERR;

    std::string bssid = getStringParamFromCommand(pcmd, JSON_PARAM_BSSID);
    if(bssid.empty())
        return JSON_RESULT_ERR;

    int id = -1;
    id = getIntParamFromCommand(pcmd, JSON_PARAM_NETID);
    if(id>=0)
        netid = std::to_string(id);

    if(netid.empty())
        return JSON_RESULT_ERR;


    std::string strCmd = COMMAND_SET_BSSID + " " + netid + " " + bssid;

    if(!wpaCtrlCmd(strCmd, ifname))
    {
        LOG_S(WARNING) << "execCmdWpaSetBssid did not receive OK for " << strCmd << " command";
        return JSON_RESULT_ERR;
    }

    return JSON_RESULT_SUCCESS;
}

json NmWorkerWpa::execCmdWpaSelect(NmCommandData* pcmd)
{
    std::string ssid = "";
    std::string bssid = "";
    std::string strSearch = "";
    std::string strResult = JSON_PARAM_SUCC;
    //bool idFound = false;
    //bool isConnected = false;
    const int MAXLINE = 32;
    char* ptr1 = nullptr;
    char* ptr2 = nullptr;
    std::string netid = "";
    std::string str_found = "";
    int id = -1;

    char line[MAXLINE];
    memset(line, 0, MAXLINE*sizeof(char));

    std::string ifname = getStringParamFromCommand(pcmd, JSON_PARAM_IF_NAME);
    if(ifname.empty())
        return JSON_RESULT_ERR;

    id = getIntParamFromCommand(pcmd, JSON_PARAM_NETID);
    if( id >= 0 )
    {
        netid = std::to_string(id);
    }
    else
    {
        ssid = getStringParamFromCommand(pcmd, JSON_PARAM_SSID);
        bssid = getStringParamFromCommand(pcmd, JSON_PARAM_BSSID);
        if(ssid.empty() && bssid.empty())
        {
            LOG_S(ERROR) << "NETID or SSID or BSSID is required to select network";
            return JSON_RESULT_ERR;
        }
    }

    std::string strCmd = COMMAND_STATUS;
    if(!wpaCtrlCmd(strCmd, ifname))
        return JSON_RESULT_ERR;

    str_found = searchLineInBuf(RESULT_COMPLETED, true);
    if(!str_found.empty())
    {
        if(!netid.empty())
        {
            strSearch = "id="+netid+"\n";
            str_found = searchLineInBuf("id="+netid+"\n", true);
            if(!str_found.empty())
            {
                LOG_S(WARNING) << "Already connected to network " << netid;
                return getJsonFromBufLines(JSON_PARAM_SUCC);
            }
        }
        else if(!bssid.empty())
        {
            str_found = searchLineInBuf("bssid="+bssid, true);
            if(!str_found.empty())
            {
                LOG_S(WARNING) << "Already connected to network " << bssid;
                return getJsonFromBufLines(JSON_PARAM_SUCC);
            }

        }
        else if(!ssid.empty())
        {
            str_found = searchLineInBuf("ssid="+ssid, true);
            if( (!str_found.empty()) && (str_found==("ssid="+ssid)) )
            {
                LOG_S(WARNING) << "Already connected to network " << ssid;
                return getJsonFromBufLines(JSON_PARAM_SUCC);
            }
        }
    }

    if(netid.empty())
    {
        id = getNetId(ifname, ssid, bssid);
    }
    if( id >= 0 )
    {
        return wpaSelectCmd(ifname, id);
    }
    else
    {
        return JSON_RESULT_ERR;
    }
}
