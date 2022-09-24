#include "nmdaemon.h"

NmDaemon::NmDaemon(std::vector<NmWorkerBase*> wrks) : req_access(), work_access(), requests()
{
    running_sock_receiver = false;
    running_dispatcher = false;
    stop_dispatcher = false;
    workers = wrks;
}

void NmDaemon::shutdown()
{
    stop_receiver = true;
    stop_dispatcher = true;
}

void NmDaemon::sock_receiver(sockpp::unix_socket sockin)
{
    unsigned long nread=0;
    unsigned long nwrite=0;
    char buf[NM_MAXBUF];
    std::string strBuf;
    bool res;

    if(running_sock_receiver == true)
    {
        LOG_S(ERROR) << "Socket receiver is already running";
        return;
    }
    running_sock_receiver = true;

    std::thread dispatch_thread(&NmDaemon::dispatcher, this, sockin.clone());

    std::unique_lock<std::mutex> ulmut_req(req_access);
    req_access.unlock();

    std::unique_lock<std::mutex> ulmut_sock(sock_access_write);
    sock_access_write.unlock();

    std::unique_lock<std::mutex> ulmut_work(work_access);
    work_access.unlock();
/*
    while (true)
    {
        nread = sockin.read(buf, sizeof(buf));
        if(nread <= 0)
            break;
        strBuf = "Received invalid request";
        sockin.write_n(strBuf.c_str(), sizeof(buf));
    }
*/
    memset(buf, 0, sizeof(buf));
    while (true)
    {
        nread = sockin.read(buf, sizeof(buf));
        if(nread <= 0)
            break;
        //LOG_S(MAX) << "Received data: " << (char*)buf;
        strBuf = std::string(buf);
        NmCommandData* request = new NmCommandData(strBuf);
        res = false;
        if(request->isValid())
        {
            for(auto worker : workers)
            {
                res = worker->isValidCmd(request);
                if(res) break;
            }
            if(res)
            {
                LOG_S(INFO) << "Received valid request: " << strBuf;
                req_access.lock();
                requests.emplace(request);
                req_access.unlock();
            }
        }
        if(!res)
        {
            LOG_S(INFO) << "Received invalid request: " << strBuf;
            json jsResult = { { JSON_PARAM_RESULT, JSON_PARAM_ERR }, { JSON_PARAM_ERR, JSON_DATA_ERR_INVALID_REQUEST } };
            std::string strResult = jsResult.dump();
            strlcpy(buf,strResult.c_str(),strResult.length()+1);
            sock_access_write.lock();
            LOG_S(INFO) << "Writing to socket: " << (char*)buf;
            nwrite = sockin.write_n(buf, sizeof(buf));
            sock_access_write.unlock();
            if(nwrite != sizeof(buf))
                LOG_S(WARNING) << "Error writing to socket: wrote " << nwrite << " bytes, expected " << sizeof(buf) << "bytes";
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

    if(running_dispatcher == true)
    {
        LOG_S(ERROR) << "Dispatcher is already running";
        return;
    }
    running_dispatcher = true;

    std::unique_lock<std::mutex> ulmut_req(req_access);
    req_access.unlock();

    std::unique_lock<std::mutex> ulmut_sock(sock_access_write);
    sock_access_write.unlock();

    std::unique_lock<std::mutex> ulmut_work(work_access);
    work_access.unlock();

    while(true)
    {
        if(!requests.empty())
        {
            req_access.lock();
            NmCommandData* request = requests.front();
            requests.pop();
            req_access.unlock();
            for(auto worker : workers)
            {
                if(worker->getScope() == request->getCommand().scope)
                {
                    work_access.lock();
                    LOG_S(INFO) << "Worker choosen: " << magic_enum::enum_name(worker->getScope());
                    jsonRes = worker->execCmd(request);
                    work_access.unlock();
                    break;
                }
            }
            strResult = jsonRes.dump();
//            strResult = jsonRes.dump(2);
            LOG_S(INFO) << "Got result from worker: " << strResult;
            if(strResult.length() >= NM_MAXBUF)
            {
                LOG_S(ERROR) << "No place for result, rebuild with increased NM_MAXBUF value";
                jsonRes = { { JSON_PARAM_RESULT, JSON_PARAM_ERR }, { JSON_PARAM_ERR, JSON_DATA_ERR_INTERNAL_ERROR } };
                strResult = jsonRes.dump();
            }
            memset(&buf, 0, NM_MAXBUF*sizeof(char));
            strlcpy(buf, strResult.c_str(), NM_MAXBUF);
            sock_access_write.lock();
            //std::this_thread::sleep_for(std::chrono::milliseconds(100));
            LOG_S(INFO) << "Writing to socket: " << (char*)buf;
            sockout.write_n(buf, NM_MAXBUF*sizeof(char));
            sock_access_write.unlock();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if(stop_dispatcher)
            break;
    }
    LOG_S(INFO) << "Dispatcher stopped";
}
