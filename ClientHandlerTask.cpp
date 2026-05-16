#include "lightning/httpServer/ClientHandlerTask.hpp"
#include "lightning/LowLevelApiException.hpp"
#include "lightning/response/HttpResponse.hpp"
#include "lightning/response/HttpResponseBuilder.hpp"
#include "lightning/httpServer/HttpServer.hpp"
#include <cerrno>
#include <functional>
#include <string>

namespace lightning
{
    const std::vector<char> ClientHandlerTask::INTERNAL_SERVER_ERROR = HttpResponseBuilder::create()
        .withStatusCode(500)
        .withStatusPhrase("InternalError")
        .build()
        .toHttpResponse();
    const std::vector<char> ClientHandlerTask::BAD_REQUEST_ERROR = HttpResponseBuilder::create()
        .withStatusCode(400)
        .withStatusPhrase("BadRequest")
        .build()
        .toHttpResponse();


    ClientHandlerTask::ClientHandlerTask(std::unique_ptr<IClient> client, std::reference_wrapper<HttpServer> server)
        : client(std::move(client)),
        server(server)
    {
    }
    auto ClientHandlerTask::operator()() -> void
    {
        uint64_t requestArrivalTime = 0;
        bool connectionKeepAlive = false;

        do {
            requestArrivalTime = this->getTimeSinceEpoch();
            auto requestBuffer = readRequest(connectionKeepAlive);
            if (!requestBuffer)
                return;

            std::optional<HttpRequest> request = HttpRequest::createRequest(requestBuffer->begin(), requestBuffer->end());;
            
            if (!request.has_value())
            {
                this->sendBadRequestError(client->getStream());
                return;
            }

            
            std::optional<std::string> connectionHeader = request.value().getHeader("Connection");
            
            if (!connectionKeepAlive && connectionHeader.has_value() && connectionHeader.value() == "keep-alive")
            {
                connectionKeepAlive = true;
                client->getStream().setTimeout(3);
            }
            else if (connectionHeader.has_value() && connectionHeader.value() == "close")
            {
                connectionKeepAlive = false;
            }

            auto response = runRequestResolver(request.value(), requestArrivalTime);

            response.setHeader("Connection", connectionKeepAlive ? "keep-alive" : "close");
            auto responseBuffer = response.toHttpResponse();
            this->client->getStream().write(responseBuffer.data(), responseBuffer.size());
        } while (connectionKeepAlive);

        this->client.get()->getStream().close();
    }

    auto ClientHandlerTask::getTimeSinceEpoch() -> std::uint64_t
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    }

    auto ClientHandlerTask::readRequest(bool connectionKeepAlive) -> std::optional<std::vector<char>>
    {
        try
        {
            return client->getStream().readUntilToken("\r\n\r\n");
        }
        catch (const LowLevelApiException& e)
        {
            handleLowLevelApiError(e, connectionKeepAlive);
            return std::nullopt;
        }
        catch (const std::runtime_error& e)
        {
            sendInternalServerError(client->getStream());
            return std::nullopt;
        }
    }

    auto ClientHandlerTask::handleLowLevelApiError(const LowLevelApiException& e, bool connectionKeepAlive) -> void
    {
        if (connectionKeepAlive && (e.capturedErrno == EAGAIN || e.capturedErrno == EWOULDBLOCK))
        {
            client->getStream().close();
            return;
        }
        std::cout << "Low level api error was caught: " << e.what() << " closing gracefully\n";
        client->getStream().close();
        ERR_print_errors_fp(stderr);
    }

    auto ClientHandlerTask::sendInternalServerError(stream::IStream& stream) -> void
    {
        stream.write(INTERNAL_SERVER_ERROR.data(), INTERNAL_SERVER_ERROR.size());
        stream.close();
    }

    auto ClientHandlerTask::sendBadRequestError(stream::IStream& stream) -> void
    {
        stream.write(BAD_REQUEST_ERROR.data(), BAD_REQUEST_ERROR.size());
        stream.close();
    }

    auto ClientHandlerTask::runRequestResolver(HttpRequest& request, std::uint64_t requestArrivalTime) -> HttpResponse
    {
        std::string matchedRegex;

        auto resolver = this->server.get()
            .getResolver(request.getMethod(), request.getRawUri(), matchedRegex)
            .value_or(HttpServer::defaultResolver);
        
        // inject framework info to the request, so it will be available for the resolver and the middleware.
        request.getFrameworkInfo() = FrameworkInfo{ .matchedRegex = matchedRegex, .requestArrivalTime = requestArrivalTime };
        request.computeUriParameters();
        request.setStream(&this->client->getStream());
        return resolver(request);
    }
} // namespace lightning
