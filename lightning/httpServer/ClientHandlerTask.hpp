#pragma once
#include <memory>
#include "MiddlewareContainer.hpp"
#include "../request/HttpRequest.hpp"
#include "../response/HttpResponse.hpp"
#include "../IClient.hpp"

namespace lightning
{
    /**
    * @brief The class ClienetHandlerTask fulfills the constraint Task<T>, and is passed to TaskExecutor::add_task.
    * The operator() is invoked by TaskExecutor, then the following code executes:
    * 
    * @li The pre middleware chain is called with the httprequest.
    * 
    * @li The resolver is called.
    * 
    * @li The post middleware chain is called, with the result returned from the resolver.
    * 
    * @li The response is converted to plain string.
    * 
    * @li The response is sent to the client.
    */
    class ClientHandlerTask
    {
    public:
        ClientHandlerTask(std::unique_ptr<IClient> client, Resolver resolver, HttpRequest request, MiddlewareContainer<>* middlewareChains);
        /**
        * @brief Call resolver and both middleware chains, and send the response to the client.
        */
        auto operator()() -> void;
    private:
        std::unique_ptr<IClient> client;
        Resolver resolver;
        HttpRequest request;
        MiddlewareContainer<>* middlewares;
    };
}