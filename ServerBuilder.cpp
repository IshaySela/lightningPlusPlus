#include "lightning/httpServer/ServerBuilder.hpp"
#include "lightning/SSLServer.hpp"

namespace lightning
{
    ServerBuilder::ServerBuilder(int port) : threadCount(1), underlyingServer(nullptr), port(port)
    {
    }

    auto ServerBuilder::createNew(int port) -> ServerBuilder
    {
        return ServerBuilder(port);
    }

    auto ServerBuilder::withThreads(int threadCount) -> ServerBuilder &
    {
        if(threadCount <= 0)
            throw std::runtime_error("Error: argument <threadCount> must be higher then 0.");
        
        this->threadCount = threadCount;
        return *this;
    }

    auto ServerBuilder::withUnderlyingServer(std::unique_ptr<ILowLevelSocketServer> underlyingServer) -> ServerBuilder&
    {
        if(underlyingServer.get() == nullptr)
            throw std::runtime_error("Error underlying server cannot be nullptr.");

        this->underlyingServer = std::move(underlyingServer);
        return *this;
    }

    auto ServerBuilder::withSsl(std::string publicKey, std::string privateKey) -> ServerBuilder&
    {
        return this->withUnderlyingServer(std::make_unique<SSLServer>(this->port,publicKey.c_str(), privateKey.c_str()));
    }

    auto ServerBuilder::build() -> HttpServer
    {
        if(this->underlyingServer.get() == nullptr)
            throw std::runtime_error("Error: a call to ServerBuilder::withUnderlyingServer must occur in order to build HttpServer.");

        return HttpServer(std::move(this->underlyingServer), this->threadCount);
    }

} // namespace lightning
