#include "lightning/SSLServer.hpp"
#include <openssl/err.h>
#include <stdexcept>
#include "lightning/LowLevelApiException.hpp"

namespace lightning
{
    SSLServer::SSLServer(int port, const char *certPath, const char *privateKeyPath) : port(port),
                                                                                       certificatePath(certPath),
                                                                                       privateKeyPath(privateKeyPath),
                                                                                       sslContext(nullptr),
                                                                                       rawSocketFd(0)
    {
        this->configureContex();
        this->createSocket();
    }

    auto SSLServer::configureContex() -> void
    {
        auto method = TLS_server_method();
        this->sslContext = SSL_CTX_new(method);

        if (this->sslContext == nullptr)
        {
            ERR_print_errors_fp(stderr);
            throw std::runtime_error("Error while creating context");
        }

        if (SSL_CTX_use_certificate_file(this->sslContext, this->certificatePath, SSL_FILETYPE_PEM) <= 0)
        {
            ERR_print_errors_fp(stderr);
            throw std::runtime_error("Error while configuring the certificate path.");
        }

        if (SSL_CTX_use_PrivateKey_file(this->sslContext, this->privateKeyPath, SSL_FILETYPE_PEM) <= 0)
        {
            ERR_print_errors_fp(stderr);
            throw std::runtime_error("Error while configuring the private key path.");
        }
    }

    auto SSLServer::createSocket() -> void
    {
        struct sockaddr_in addr;

        addr.sin_family = AF_INET;
        addr.sin_port = htons(this->port);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        this->rawSocketFd = socket(AF_INET, SOCK_STREAM, 0);

        if (this->rawSocketFd < 0)
        {
            throw LowLevelApiException("Calling socket() has failed", GetLastError());
        }

        if (bind(this->rawSocketFd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            throw LowLevelApiException("Calling bind() has failed", GetLastError());
        }

        if (listen(this->rawSocketFd, 1) < 0)
        {
            throw LowLevelApiException("Calling listen() has failed", GetLastError());
        }
    }

    auto SSLServer::accept() -> SSLClient
    {
        struct sockaddr_in addr;
        socklen_t addrlen = sizeof(addr);

        int clientFd = ::accept(this->rawSocketFd, (struct sockaddr *)&addr, &addrlen);

        if (clientFd < 0)
        {
            throw LowLevelApiException("Error while calling accept()", GetLastError());
        }

        auto ssl = SmartResource<SSL>(SSL_new(this->sslContext), SmartSslResource::sslObjectDeleter);

        SSL_set_fd(ssl.get(), clientFd);

        int acceptRet = 0;
        if ((acceptRet = SSL_accept(ssl.get())) != 1)
        {
            ERR_print_errors_fp(stderr);
            this->handleSslAcceptError(ssl.get(), acceptRet);
        }
        
        return SSLClient(clientFd, std::move(ssl));
    }

    auto SSLServer::handleSslAcceptError(SSL *ssl, int ret) -> void
    {
        auto error = SSL_get_error(ssl, ret);
        //throw LowLevelApiException("Error in SSL handshake", );
    }

} // namespace lightning
