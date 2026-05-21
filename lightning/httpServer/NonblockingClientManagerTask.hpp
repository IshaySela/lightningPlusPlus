#pragma once
#include <functional>
#include <sys/epoll.h>
#include <unordered_map>
#include <vector>
#include "ClientRequestHandler.hpp"
#include "FdChannels.hpp"
#include "../TaskExecutor.hpp"

namespace lightning
{
    class HttpServer;

    class NonblockingClientManagerTask
    {
    public:
        NonblockingClientManagerTask(
            NewFdChannel& newFdChannel,
            ReturnChannel& returnChannel,
            int threadCount,
            std::reference_wrapper<HttpServer> server
        );

        auto operator()() -> void;

    private:
        struct ConnectionState
        {
            std::unique_ptr<IClient> client;
        };

        static constexpr int MAX_HEADER_BYTES = 65536;

        NewFdChannel& newFdChannel;
        ReturnChannel& returnChannel;
        TaskExecutor<ClientRequestHandler> workerPool;
        std::reference_wrapper<HttpServer> server;

        int epollFd = -1;
        std::unordered_map<int, ConnectionState> connections;

        auto addToEpoll(int fd, uint32_t events) -> void;
        auto rearmFd(int fd) -> void;
        auto drainNewFdChannel() -> void;
        auto drainReturnChannel() -> void;
        auto handleClientEvent(int fd, uint32_t events) -> void;
    };
} // namespace lightning
