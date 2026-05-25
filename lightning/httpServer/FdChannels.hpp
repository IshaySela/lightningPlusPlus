#pragma once
#include <boost/type_traits/has_trivial_destructor.hpp>
#include <memory>
#include <mutex>
#include <vector>
#include "../IClient.hpp"
#include "lightning/moodycamel/concurrentqueue.h"

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
        ReturnedConnection(std::unique_ptr<IClient> client, bool keepAlive) : client(std::move(client)), keepAlive(keepAlive) {}
    };
    struct ReturnChannel
    {
        moodycamel::ConcurrentQueue<ReturnedConnection> connections;
        int pipeRead = -1;
        int pipeWrite = -1;
    };
} // namespace lightning
