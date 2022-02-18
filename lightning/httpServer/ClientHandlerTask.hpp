#pragma once
#include <memory>
#include "../lightning.hpp"
#include "../IClient.hpp"

namespace lightning
{
    class ClientHandlerTask
    {
    public:
        ClientHandlerTask(std::unique_ptr<IClient> client, Resolver resolver, HttpRequest request);
        /**
             * @brief Call resolver with request and send the result back to the client.
             * Satisfy the Task<T> constraint.
             */
        auto operator()() -> void;
    private:
        std::unique_ptr<IClient> client;
        Resolver resolver;
        HttpRequest request;
    };
}