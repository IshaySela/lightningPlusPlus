#pragma once
#include <memory>
#include "HttpServer.hpp"
#include "../LowLevelSocketServer.hpp"

namespace lightning
{
    class ServerBuilder
    {
    protected:
        ServerBuilder(int port);

        std::unique_ptr<ILowLevelSocketServer> underlyingServer;
        int threadCount;
        int port;

    public:
        static auto createNew(int port) -> ServerBuilder;

        /**
         * @brief Create the http server. Throw std::runtime_error if the underlying server was not specified.
         *
         * @return HttpServer The created http server with arguments passed through the various methods.
         */
        auto build() -> HttpServer;
        /**
         * @brief Specify the thread count of the server. By default is 1.
         * throw std::runtime_error if threadCount is less or equal to 0.
         *
         * @param threadCount The amount of threads to use.
         * @return ServerBuilder& Reference to this.
         */
        auto withThreads(int threadCount) -> ServerBuilder &;

        /**
         * @brief Specify the low level socket server.
         * Throw std::runtime_error if underlyingServer is nullptr.
         *
         * @param underlyingServer The low level socket server.
         * @return ServerBuilder& Reference to this.
         */
        auto withUnderlyingServer(std::unique_ptr<ILowLevelSocketServer> underlyingServer) -> ServerBuilder &;

        /**
         * @brief Configure the server to use SSLServer as underlying socket server.
         *
         * @param publicKeyPath The path to the public key file.
         * @param privateKeyPath The path to the private key file.
         *
         * @return ServerBuilder& Reference to this.
         */
        auto withSsl(std::string publicKeyPath, std::string privateKeyPath) -> ServerBuilder &;
    };
} // namespace lightning
