#include "lightning/httpServer/ClientHandlerTask.hpp"
#include "lightning/LowLevelApiException.hpp"
#include "lightning/OpensslErrorQueueException.hpp"
#include "lightning/response/HttpResponseBuilder.hpp"
#include "lightning/httpServer/HttpServer.hpp"
#include <functional>

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


    ClientHandlerTask::ClientHandlerTask(std::unique_ptr<IClient> client, std::reference_wrapper<HttpServer> server, MiddlewareContainer<>* middlewareChains)
        : client(std::move(client)),
        server(server),
        middlewares(middlewareChains)
    {
    }
    auto ClientHandlerTask::operator()() -> void
    {
        std::string matchedRegex;
        uint64_t requestArrivalTime = this->getTimeSinceEpoch();

        std::vector<char> requestBuffer;

        try
        {
            requestBuffer = client->getStream().readUntilToken("\r\n\r\n");
        }
        catch (const LowLevelApiException& e)
        {
            std::cout << "Low level api error was caught: " << e.what() << "closing gracefully\n";
            client.get()->getStream().close();
            ERR_print_errors_fp(stderr);
            return;
        }
        catch (const std::runtime_error& e)
        {
            this->sendInternalServerError(client->getStream());
            return;
        }
        
        std::optional<HttpRequest> request = HttpRequest::createRequest(requestBuffer.begin(), requestBuffer.end());;
        
        if (!request.has_value())
        {
            client->getStream().write(BAD_REQUEST_ERROR.data(), BAD_REQUEST_ERROR.size());
            client.get()->getStream().close();
            return;
        }
        

        auto resolver = this->server.get()
            .getResolver(request.value().getMethod(), request.value().getRawUri(), matchedRegex)
            .value_or(HttpServer::defaultResolver);

        request.value().getFrameworkInfo() = FrameworkInfo{ .matchedRegex = matchedRegex, .requestArrivalTime = requestArrivalTime };

            
        // This is needs to be done on the working thread
        // And can only be done after frameworkInfo was injected.
        request.value().computeUriParameters();
        request.value().setStream(&this->client->getStream());
        std::optional<HttpResponse> preMiddlewareResult;

        for (auto& currPreMiddleware : this->middlewares->getPreMiddlewares())
        {
            preMiddlewareResult = currPreMiddleware(request.value());
            
            if (preMiddlewareResult.has_value())
                break;
        }

        // If one of the pre middleware has returned a response, 
        // ues that response instead.
        auto response = preMiddlewareResult.has_value() ? preMiddlewareResult.value() : resolver(request.value());

        for (auto& post : this->middlewares->getPostMiddlewares())
        {
            if (!post(response)) {
                break;
            }
        }
        response.setHeader("Connection", "close");
        auto responseBuffer = response.toHttpResponse();
        this->client->getStream().write(responseBuffer.data(), responseBuffer.size());
        this->client.get()->getStream().close();
    }

    auto ClientHandlerTask::getTimeSinceEpoch() -> std::uint64_t
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    }

    auto ClientHandlerTask::sendInternalServerError(stream::IStream& stream) -> void
    {
        stream.write(INTERNAL_SERVER_ERROR.data(), INTERNAL_SERVER_ERROR.size());
        stream.close();
    }
} // namespace lightning
