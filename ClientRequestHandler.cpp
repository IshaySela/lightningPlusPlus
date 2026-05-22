#include "lightning/httpServer/ClientRequestHandler.hpp"
#include "lightning/httpServer/FdChannels.hpp"
#include "lightning/httpServer/HttpServer.hpp"
#include "lightning/request/FrameworkInfo.hpp"
#include "lightning/response/HttpResponseBuilder.hpp"
#include <chrono>
#include <unistd.h>

namespace lightning
{
    const std::vector<char> ClientRequestHandler::BAD_REQUEST_ERROR = HttpResponseBuilder::create()
        .withStatusCode(400).withStatusPhrase("BadRequest").build().toHttpResponse();
    const std::vector<char> ClientRequestHandler::INTERNAL_SERVER_ERROR = HttpResponseBuilder::create()
        .withStatusCode(500).withStatusPhrase("InternalError").build().toHttpResponse();

    ClientRequestHandler::ClientRequestHandler(
        std::unique_ptr<IClient> client,
        std::vector<char> requestBuffer,
        std::reference_wrapper<HttpServer> server,
        ReturnChannel& returnCh
    ) : client(std::move(client)),
        requestBuffer(std::move(requestBuffer)),
        server(server),
        returnChannel(returnCh)
    {}

    auto ClientRequestHandler::operator()() -> void
    {
        auto request = HttpRequest::createRequest(requestBuffer.begin(), requestBuffer.end());
        if (!request.has_value())
        {
            client->getStream().write(BAD_REQUEST_ERROR.data(), BAD_REQUEST_ERROR.size());
            signalReturn(false);
            return;
        }

        bool keepAlive = false;
        auto connectionHeader = request->getHeader("Connection");
        if (connectionHeader.has_value() && connectionHeader.value() == "keep-alive")
            keepAlive = true;

        try
        {
            auto response = runRequestResolver(request.value(), getTimeSinceEpoch());
            response.setHeader("Connection", keepAlive ? "keep-alive" : "close");
            auto responseBytes = response.toHttpResponse();
            client->getStream().write(responseBytes.data(), responseBytes.size());
        }
        catch (...)
        {
            client->getStream().write(INTERNAL_SERVER_ERROR.data(), INTERNAL_SERVER_ERROR.size());
            keepAlive = false;
        }

        signalReturn(keepAlive);
    }

    auto ClientRequestHandler::runRequestResolver(HttpRequest& request, std::uint64_t requestArrivalTime) -> HttpResponse
    {
        std::string matchedRegex;
        auto resolver = server.get()
            .getResolver(request.getMethod(), request.getRawUri(), matchedRegex)
            .value_or(HttpServer::defaultResolver);

        request.getFrameworkInfo() = FrameworkInfo{
            .matchedRegex = matchedRegex,
            .requestArrivalTime = requestArrivalTime
        };
        request.computeUriParameters();
        request.setStream(&client->getStream());
        return resolver(request);
    }

    auto ClientRequestHandler::getTimeSinceEpoch() -> std::uint64_t
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }

    auto ClientRequestHandler::signalReturn(bool keepAlive) -> void
    {
        returnChannel.get().connections.emplace(std::move(client), keepAlive);
        char byte = 'x';
        ::write(returnChannel.get().pipeWrite, &byte, 1);
    }
} // namespace lightning
