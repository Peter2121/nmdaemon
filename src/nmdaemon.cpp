#include "nmdaemon.h"

std::mutex NmDaemon::work_access;

NmDaemon::NmDaemon(std::vector<NmWorkerBase*> wrks) : req_access(), requests()
{
    running_sock_receiver = false;
    running_dispatcher = false;
    stop_dispatcher = false;
    stop_receiver = false;
    workers = wrks;
}

void NmDaemon::shutdown()
{
    stop_receiver = true;
    stop_dispatcher = true;
}

void NmDaemon::sock_receiver(sockpp::unix_socket sockin)
{
    ssize_t nread=0;
    ssize_t nwrite=0;
    char buf[NM_MAXBUF];
    std::string strBuf;
    bool res;

    if(running_sock_receiver == true)
    {
        LOG_S(ERROR) << "Socket receiver is already running";
        return;
    }
    running_sock_receiver = true;
    logPeerCredentials(sockin);

    std::thread dispatch_thread(&NmDaemon::dispatcher, this, sockin.clone());

    while (true)
    {
        memset(buf, 0, sizeof(buf));
        nread = sockin.read(buf, sizeof(buf)-1);
        if(nread <= 0)
            break;
        strBuf = std::string(buf);
        //NmCommandData* request = new NmCommandData(strBuf);
        NmCommandData* request = nullptr;
        res = false;
        try
        {
            request = new NmCommandData(strBuf);
            if(request->isValid())
            {
                for(auto worker : workers)
                {
                    res = worker->isValidCmd(request);
                    if(res) break;
                }
            }
        }
        catch(std::exception& e)
        {
            LOG_S(WARNING) << "Exception parsing request '" << Tool::hideSecrets(strBuf) << "': " << e.what();
            res = false;
        }

        if(res)
        {
            LOG_S(INFO) << "Received valid request: " << Tool::hideSecrets(strBuf);
            std::lock_guard<std::mutex> lk(req_access);
            requests.emplace(request);
        }
        else
        {
            delete request;
            LOG_S(INFO) << "Received invalid request: " << Tool::hideSecrets(strBuf);
            json jsResult = { { JSON_PARAM_RESULT, JSON_PARAM_ERR }, { JSON_PARAM_ERR, JSON_DATA_ERR_INVALID_REQUEST } };
            std::string strResult = jsResult.dump();
            strlcpy(buf, strResult.c_str(), sizeof(buf));
            {
                std::lock_guard<std::mutex> lk(sock_access_write);
                LOG_S(INFO) << "Writing to socket: " << (char*)buf;
                nwrite = sockin.write_n(buf, sizeof(buf));
                if(nwrite != (ssize_t)sizeof(buf))
                    LOG_S(WARNING) << "Error writing to socket: wrote " << nwrite << " bytes, expected " << sizeof(buf) << "bytes";
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        if(stop_receiver)
            break;
    }
    sockin.close();
    LOG_S(INFO) << "Connection closed";
    stop_dispatcher = true;
    if(dispatch_thread.joinable())
        dispatch_thread.join();
    LOG_S(INFO) << "Receiver stopped";
}

void NmDaemon::dispatcher(sockpp::unix_socket sockout)
{
    json jsonRes;
    std::string strResult;
    char buf[NM_MAXBUF];
    const json jsonInternalErr = { { JSON_PARAM_RESULT, JSON_PARAM_ERR }, { JSON_PARAM_ERR, JSON_DATA_ERR_INTERNAL_ERROR } };

    if(running_dispatcher == true)
    {
        LOG_S(ERROR) << "Dispatcher is already running";
        return;
    }
    running_dispatcher = true;

    while(true)
    {
        NmCommandData* request = nullptr;
        {
            std::lock_guard<std::mutex> lk(req_access);
            if(!requests.empty())
            {
                request = requests.front();
                requests.pop();
            }
        }
        if(request != nullptr)
        {
            jsonRes = jsonInternalErr;
            try {
                for(auto worker : workers)
                {
                    if(worker->getScope() == request->getCommand().scope)
                    {
                        std::lock_guard<std::mutex> lk(work_access);
                        LOG_S(INFO) << "Worker choosen: " << magic_enum::enum_name(worker->getScope());
                        jsonRes = worker->execCmd(request);
                        break;
                    }
                }
            }
            catch(std::exception& e)
            {
                LOG_S(ERROR) << "Exception in worker: " << e.what();
                jsonRes = jsonInternalErr;
            }
            delete request;
            strResult = jsonRes.dump();
            LOG_S(INFO) << "Got result from worker: " << strResult;
            if(strResult.length() >= NM_MAXBUF)
            {
                LOG_S(ERROR) << "No place for result, rebuild with increased NM_MAXBUF value";
                jsonRes = jsonInternalErr;
                strResult = jsonRes.dump();
            }
            memset(&buf, 0, NM_MAXBUF*sizeof(char));
            strlcpy(buf, strResult.c_str(), NM_MAXBUF);
            {
                std::lock_guard<std::mutex> lk(sock_access_write);
                LOG_S(INFO) << "Writing to socket: " << (char*)buf;
                sockout.write_n(buf, NM_MAXBUF*sizeof(char));
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if(stop_dispatcher)
            break;
    }
    LOG_S(INFO) << "Dispatcher stopped";
}

void NmDaemon::logPeerCredentials(const sockpp::unix_socket& sock)
{
    struct xucred xuc;
    socklen_t len = sizeof(xuc);
    memset(&xuc, 0, sizeof(xuc));
    if(getsockopt(sock.handle(), SOL_LOCAL, LOCAL_PEERCRED, &xuc, &len) == 0 && xuc.cr_version == XUCRED_VERSION)
    {
        LOG_S(INFO) << "Connection from uid " << xuc.cr_uid << " gid " << (xuc.cr_ngroups > 0 ? (long)xuc.cr_groups[0] : -1L);
    }
    else
    {
        LOG_S(WARNING) << "Cannot get connected peer credentials: " << strerror(errno);
    }
}