#include "wpasocket.h"

WpaSocket::WpaSocket(std::string cli_socket, std::string srv_socket) : clientSocket(cli_socket), serverSocket(srv_socket), nmExcept()
{
    psock = new sockpp::unix_dgram_socket();

    if (!psock->bind(sockpp::unix_address(clientSocket)))
    {
        LOG_S(ERROR) << "WpaSocket constructor cannot connect to client socket " << clientSocket << " : " << psock->last_error_str();
        delete psock;
        throw nmExcept;
    }

    if (!psock->connect(sockpp::unix_address(serverSocket)))
    {
        LOG_S(ERROR) << "WpaSocket constructor cannot connect to server socket " << serverSocket << " : " << psock->last_error_str();
        psock->close();
        unlink(clientSocket.c_str());
        delete psock;
        throw nmExcept;
    }
}

WpaSocket::~WpaSocket()
{
    psock->close();
    unlink(clientSocket.c_str());
    delete psock;
}

ssize_t WpaSocket::sockSend(const std::string& s) const
{
    return psock->send(s);
}

ssize_t WpaSocket::sockReceiveDontwait(void* buf, size_t n) const
{
    return psock->recv(buf, n, MSG_DONTWAIT);
}

ssize_t WpaSocket::sockReceive(void* buf, size_t n, int flags) const
{
    return psock->recv(buf, n, flags);
}

std::string WpaSocket::sockLastError() const
{
    return psock->last_error_str();
}
