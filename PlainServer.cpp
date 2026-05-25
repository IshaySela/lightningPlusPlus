#include "lightning/PlainServer.hpp"
#include "lightning/PlainClient.hpp"
#include "lightning/LowLevelApiException.hpp"
#include "lightning/sockets.hpp"

namespace lightning
{
    PlainServer::PlainServer(int port) : port(port), rawSocketFd(0)
    {
        this->createSocket();
    }

    auto PlainServer::createSocket() -> void
    {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(this->port);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        this->rawSocketFd = socket(AF_INET, SOCK_STREAM, 0);
        if (this->rawSocketFd < 0)
            throw LowLevelApiException("socket() failed", GetLastError(), errno);

        int opt = 1;
        setsockopt(this->rawSocketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        if (bind(this->rawSocketFd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
            throw LowLevelApiException("bind() failed", GetLastError(), errno);

        if (listen(this->rawSocketFd, SOMAXCONN) < 0)
            throw LowLevelApiException("listen() failed", GetLastError(), errno);
    }

    auto PlainServer::accept() -> std::unique_ptr<IClient>
    {
        struct sockaddr_in addr;
        socklen_t addrlen = sizeof(addr);

        int clientFd = ::accept(this->rawSocketFd, (struct sockaddr *)&addr, &addrlen);
        if (clientFd < 0)
            throw LowLevelApiException("accept() failed", GetLastError(), errno);

        return std::make_unique<PlainClient>(clientFd);
    }
}