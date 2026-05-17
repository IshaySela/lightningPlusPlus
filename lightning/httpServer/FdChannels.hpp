#pragma once
#include <memory>
#include <mutex>
#include <vector>
#include "../IClient.hpp"

namespace lightning
{
    struct NewFdChannel
    {
        std::mutex m;
        std::vector<std::unique_ptr<IClient>> clients;
        int pipeRead = -1;
        int pipeWrite = -1;
    };

    struct ReturnedConnection
    {
        std::unique_ptr<IClient> client;
        bool keepAlive;
    };

    struct ReturnChannel
    {
        std::mutex m;
        std::vector<ReturnedConnection> connections;
        int pipeRead = -1;
        int pipeWrite = -1;
    };
} // namespace lightning
