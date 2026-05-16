#pragma once
#include "LowLevelSocketServer.hpp"

namespace lightning
{
    class PlainServer : public ILowLevelSocketServer
    {
    public:
        PlainServer(int port);
        virtual auto accept() -> std::unique_ptr<IClient> override;
        ~PlainServer() = default;

    private:
        auto createSocket() -> void;

        int port;
        int rawSocketFd;
    };
}