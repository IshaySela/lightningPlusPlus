#pragma once
#include <memory>
#include <mutex>
#include <utility>
#include <vector>
#include "../IClient.hpp"
#include "lightning/MPMCQueue.hpp"
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
        ReturnedConnection& operator=(ReturnedConnection&& other) noexcept = default;
    };

    struct ReturnChannel
    {
        rigtorp::MPMCQueue<ReturnedConnection> connections;
        int pipeRead = -1;
        int pipeWrite = -1;

        ReturnChannel() : connections(64)
        {
        }
    };
} // namespace lightning
