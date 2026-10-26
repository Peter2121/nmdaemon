#include "nmworkerbase.h"

#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#include <array>
#include <filesystem>
#include <regex>
#include <string_view>
#include <thread>

#include "loguru/loguru.hpp"
#include "sockpp/unix_connector.h"

namespace
{
    constexpr std::size_t ASYNC_NOTIFY_MAXBUF = 512;

    // Hardening for the client callback socket named in an async request.
    // These will move to the config file later; for now they are fixed here.
    constexpr std::size_t MAX_REQUEST_ID_LENGTH = 32;
    constexpr std::array<std::string_view, 2> ALLOWED_CLIENT_SOCKET_DIRS = { "/tmp", "/var/tmp" };
    constexpr bool permitRootOwnedClientSocket = false;

    bool isValidRequestId(const std::string& request_id)
    {
        static const std::regex requestIdRegex(R"(^[A-Za-z0-9]+$)");
        return request_id.length() <= MAX_REQUEST_ID_LENGTH && std::regex_match(request_id, requestIdRegex);
    }

    bool isAllowedClientSocketDir(const std::string& sock_path)
    {
        std::string parent = std::filesystem::path(sock_path).lexically_normal().parent_path().string();
        for(auto dir : ALLOWED_CLIENT_SOCKET_DIRS)
        {
            if(parent == dir)
                return true;
        }
        return false;
    }

    // Also re-run right before delivering the final result (see
    // sendAsyncNotification), since the request-time check and the delivery
    // happen at different times and the filesystem entity at sock_path could
    // have been swapped out in between.
    bool isClientSocketOwnerAllowed(const std::string& sock_path)
    {
        struct stat st;
        if(stat(sock_path.c_str(), &st) != 0)
        {
            LOG_S(ERROR) << "Cannot stat client socket " << sock_path << " : " << strerror(errno);
            return false;
        }
        if(!S_ISSOCK(st.st_mode))
        {
            LOG_S(ERROR) << "Client socket path is not a socket: " << sock_path;
            return false;
        }
        if(st.st_uid == 0 && !permitRootOwnedClientSocket)
        {
            LOG_S(ERROR) << "Client socket " << sock_path << " is owned by root, which is not permitted";
            return false;
        }
        return true;
    }

    void sendAsyncNotification(const std::string& sock_path, const json& payload)
    {
        if(!isClientSocketOwnerAllowed(sock_path))
        {
            LOG_S(ERROR) << "Async notification: client socket " << sock_path << " failed ownership check at delivery time, dropping result";
            return;
        }

        sockpp::unix_connector conn;
        if(!conn.connect(sockpp::unix_address(sock_path)))
        {
            LOG_S(ERROR) << "Async notification cannot connect to client socket " << sock_path << " : " << conn.last_error_str();
            return;
        }

        std::string str_payload = payload.dump();
        if(str_payload.length() >= ASYNC_NOTIFY_MAXBUF)
        {
            LOG_S(ERROR) << "Async notification payload too long for " << sock_path;
            conn.close();
            return;
        }

        char buf[ASYNC_NOTIFY_MAXBUF];
        memset(buf, 0, sizeof(buf));
        strlcpy(buf, str_payload.c_str(), sizeof(buf));
        ssize_t nwrite = conn.write_n(buf, sizeof(buf));
        if(nwrite != (ssize_t)sizeof(buf))
            LOG_S(WARNING) << "Async notification short write to " << sock_path << ": wrote " << nwrite << " bytes, expected " << sizeof(buf);
        conn.close();
    }
}

WorkerAccessGuard::WorkerAccessGuard(std::atomic<bool>& flag)
{
    bool expected = false;
    if(flag.compare_exchange_strong(expected, true))
        flag_ = &flag;
}

WorkerAccessGuard::WorkerAccessGuard(WorkerAccessGuard&& other) noexcept : flag_(other.flag_)
{
    other.flag_ = nullptr;
}

WorkerAccessGuard& WorkerAccessGuard::operator=(WorkerAccessGuard&& other) noexcept
{
    if(this != &other)
    {
        release();
        flag_ = other.flag_;
        other.flag_ = nullptr;
    }
    return *this;
}

WorkerAccessGuard::~WorkerAccessGuard()
{
    release();
}

void WorkerAccessGuard::release()
{
    if(flag_ != nullptr)
    {
        flag_->store(false);
        flag_ = nullptr;
    }
}

json NmWorkerBase::execCmdAsync(NmCommandData* pcmd, std::function<json(NmCommandData*)> syncExec)
{
    json cmd_data;
    std::string sock_path;
    std::string request_id;

    try
    {
        cmd_data = pcmd->getJsonData();
        if(!cmd_data.contains(JSON_PARAM_SOCKET) || !cmd_data[JSON_PARAM_SOCKET].is_string())
        {
            LOG_S(WARNING) << "execCmdAsync: missing or invalid " << JSON_PARAM_SOCKET;
            return JSON_RESULT_ERR;
        }
        sock_path = cmd_data[JSON_PARAM_SOCKET].get<std::string>();

        if(!cmd_data.contains(JSON_PARAM_REQUEST) || !cmd_data[JSON_PARAM_REQUEST].is_string())
        {
            LOG_S(WARNING) << "execCmdAsync: missing or invalid " << JSON_PARAM_REQUEST;
            return JSON_RESULT_ERR;
        }
        request_id = cmd_data[JSON_PARAM_REQUEST].get<std::string>();
    }
    catch(std::exception& e)
    {
        LOG_S(WARNING) << "execCmdAsync: exception reading request: " << e.what();
        return JSON_RESULT_ERR;
    }

    if(sock_path.empty())
        return JSON_RESULT_ERR;

    if(!isValidRequestId(request_id))
    {
        LOG_S(WARNING) << "execCmdAsync: " << JSON_PARAM_REQUEST << " must be 1-" << MAX_REQUEST_ID_LENGTH << " alphanumeric characters";
        return JSON_RESULT_ERR;
    }

    if(!isAllowedClientSocketDir(sock_path))
    {
        LOG_S(ERROR) << "execCmdAsync: client socket " << sock_path << " is not in an allowed directory";
        return JSON_RESULT_ERR;
    }

    if(!isClientSocketOwnerAllowed(sock_path))
        return JSON_RESULT_ERR;

    // Non-blocking: if this worker is already busy (sync command, or a
    // previous async job still running in its background thread), fail fast
    // with BUSY rather than stalling the dispatcher (which holds work_access
    // for the whole duration of this call).
    WorkerAccessGuard worker_guard(worker_access);
    if(!worker_guard.owns())
    {
        LOG_S(WARNING) << "execCmdAsync: worker busy with another operation";
        return JSON_RESULT_BUSY;
    }

    {
        sockpp::unix_connector testconn;
        if(!testconn.connect(sockpp::unix_address(sock_path)))
        {
            LOG_S(ERROR) << "execCmdAsync cannot connect to client socket " << sock_path << " : " << testconn.last_error_str();
            return JSON_RESULT_ERR;
        }
        testconn.close();
    }

    std::string str_cmd_data = cmd_data.dump();

    // pcmd itself is owned by the dispatcher, which deletes it as soon as
    // execCmd (and therefore this function) returns, so the background thread
    // gets its own independent copy instead of the original pointer.
    std::thread([syncExec, str_cmd_data, sock_path, request_id, guard = std::move(worker_guard)]() mutable
    {
        json result = JSON_RESULT_ERR;
        try
        {
            NmCommandData async_cmd(str_cmd_data);
            result = syncExec(&async_cmd);
        }
        catch(std::exception& e)
        {
            LOG_S(ERROR) << "execCmdAsync: exception executing async command: " << e.what();
            result = JSON_RESULT_ERR;
        }
        guard.release();

        bool succeeded = result.contains(JSON_PARAM_RESULT) && result[JSON_PARAM_RESULT] == JSON_PARAM_SUCC;
        json notify = { { JSON_PARAM_RESULT, succeeded ? JSON_PARAM_SUCC : JSON_PARAM_ERR }, { JSON_PARAM_REQUEST, request_id } };
        sendAsyncNotification(sock_path, notify);
    }).detach();

    return JSON_RESULT_SUCCESS;
}
