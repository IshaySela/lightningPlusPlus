#pragma once
#include <openssl/ssl.h>
#include "SSLClient.hpp"

namespace lightning
{

    /**
     * @brief The class SSLServer handles the basic TLS/SSL communication with the client.
     * In future version, the context will be passed in the constructor and a builder will be provided.
     */
    class SSLServer
    {
    public:
        SSLServer(int port, const char *certPath, const char *privateKeyPath);

        /**
         * @brief Accept a new client and return a new SSLClient object.
         * 
         * @return SSLClient The SSLClient object that was constructed.
         */
        auto accept() -> SSLClient;

    private:
        /**
         * @brief Configure SSLServer::sslContext to use the certificate and private key provided, 
         * the port to use and other protocol metadata.
         */
        auto configureContex() -> void;

        /**
         * @brief Create and configure the raw socket fd of the server.
         */
        auto createSocket() -> void;

        auto handleSslAcceptError(SSL* ssl, int ret) -> void;

        int port;
        int rawSocketFd;
        const char *certificatePath;
        const char *privateKeyPath;

        SSL_CTX *sslContext;
    };
}