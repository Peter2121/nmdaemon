#ifndef WPASOCKET_H
#define WPASOCKET_H

#include "loguru/loguru.hpp"
#include "sockpp/socket.h"
#include "sockpp/unix_dgram_socket.h"
#include "sockpp/version.h"
#include "nmexception.h"

class WpaSocket
{
protected:
    sockpp::unix_dgram_socket* psock;
    std::string clientSocket;
    std::string serverSocket;
    NmException nmExcept;
public:
    WpaSocket(std::string cli_socket, std::string srv_socket);
    ~WpaSocket();
    ssize_t sockSend(const std::string& s) const;
    ssize_t sockReceiveDontwait(void* buf, size_t n) const;
    ssize_t sockReceive(void* buf, size_t n, int flags=0) const;
    std::string sockLastError() const;
};

#endif // WPASOCKET_H
