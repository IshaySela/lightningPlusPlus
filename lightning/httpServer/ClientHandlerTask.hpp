#pragma once
#include <functional>
#include <memory>
#include "../IClient.hpp"
namespace lightning
{
    class HttpServer;
    /**
    * @brief The class ClientHandlerTask fulfills the constraint Task<T>, and is passed to TaskExecutor::add_task.
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
        ClientHandlerTask(std::unique_ptr<IClient> client, std::reference_wrapper<HttpServer> server);
        /**
        * @brief Call resolver and both middleware chains, and send the response to the client.
        */
        auto operator()() -> void;
    private:
        static const std::vector<char> INTERNAL_SERVER_ERROR;
        static const std::vector<char> BAD_REQUEST_ERROR;

        std::unique_ptr<IClient> client;
        std::reference_wrapper<HttpServer> server;

        auto getTimeSinceEpoch() -> std::uint64_t;
        auto sendInternalServerError(stream::IStream& stream) -> void;
        auto sendBadRequestError(stream::IStream& stream) -> void;
    };
}