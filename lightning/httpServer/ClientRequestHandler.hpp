#pragma once
#include <functional>
#include <memory>
#include <vector>
#include "FdChannels.hpp"
#include "../request/HttpRequest.hpp"
#include "../response/HttpResponse.hpp"

namespace lightning
{
    class HttpServer;

    class ClientRequestHandler
    {
    public:
        ClientRequestHandler(
            std::unique_ptr<IClient> client,
            std::vector<char> requestBuffer,
            std::reference_wrapper<HttpServer> server,
            ReturnChannel& returnChannel
        );

        ClientRequestHandler(ClientRequestHandler&&) = default;
        ClientRequestHandler& operator=(ClientRequestHandler&&) = default;
        ClientRequestHandler(const ClientRequestHandler&) = delete;
        ClientRequestHandler& operator=(const ClientRequestHandler&) = delete;

        auto operator()() -> void;

    private:
        static const std::vector<char> BAD_REQUEST_ERROR;
        static const std::vector<char> INTERNAL_SERVER_ERROR;

        std::unique_ptr<IClient> client;
        std::vector<char> requestBuffer;
        std::reference_wrapper<HttpServer> server;
        std::reference_wrapper<ReturnChannel> returnChannel;

        auto runRequestResolver(HttpRequest& request, std::uint64_t requestArrivalTime) -> HttpResponse;
        auto getTimeSinceEpoch() -> std::uint64_t;
        auto signalReturn(bool keepAlive) -> void;
    };
} // namespace lightning
