#include "lightning/httpServer/HttpServer.hpp"
#include "lightning/LowLevelApiException.hpp"
#include <fstream>
#include <openssl/err.h>

namespace lightning
{
    const HttpServer::ShouldStopPredicate HttpServer::neverStop = [](HttpServer&)
    { return false; };
    const std::vector<char> HttpServer::INTERNAL_SERVER_ERROR = HttpResponseBuilder::create()
        .withStatusCode(500)
        .withStatusPhrase("InternalError")
        .build()
        .toHttpResponse();

    const Resolver HttpServer::defaultResolver = [](HttpRequest request) -> lightning::HttpResponse
    {
        return HttpResponseBuilder::create()
            .withStatusCode(404)
            .withStatusPhrase("Not Found")
            .build();
    };

    HttpServer::HttpServer(std::unique_ptr<ILowLevelSocketServer> lowLevelServer, const int threadCount) : lowLevelServer(std::move(lowLevelServer)), tasks(threadCount)
    {
        for (int i = 0; i < HttpProtocol::supportedHttpMethods.size(); i++)
        {
            auto& method = HttpProtocol::supportedHttpMethods[i];

            this->resolvers.insert({ method, UriMapper() });
        }
    }

    auto HttpServer::start(ShouldStopPredicate shouldStop) -> void
    {
        do
        {
            std::unique_ptr<IClient> client = this->lowLevelServer->accept();
            uint64_t requestArrivalTime = HttpServer::getTimeSinceEpoch();
            std::string matchedRegex;

            std::vector<char> requestBuffer;

            try
            {
                requestBuffer = client->getStream().readUntilToken("\r\n\r\n");
            }
            catch (const OpenSslErrorQueueException& e)
            {
                std::cout << "Low level openssl error was caught: " << e.what() << '\n';
                // Print the openssl error queue
                continue;
            }
            catch (const LowLevelApiException& e)
            {
                std::cout << "Low level openssl error was caught: " << e.what() << '\n';
                continue;
            }
            catch (const std::runtime_error& e)
            {
                this->sendInternalServerError(client->getStream());
                continue;
            }

            lightning::HttpRequest request = lightning::HttpRequest::createRequest(requestBuffer.begin(), requestBuffer.end());

            auto resolver = this->getResolver(request.getMethod(), request.getRawUri(), matchedRegex).value_or(HttpServer::defaultResolver);

            request.getFrameworkInfo() = lightning::FrameworkInfo{ .matchedRegex = matchedRegex, .requestArrivalTime = requestArrivalTime };
            this->tasks.add_task(ClientHandlerTask(std::move(client), resolver, request, &this->middlewares));


            std::cout << "Client request parsed and passed to TaskExecutor" << std::endl;
        } while (!shouldStop(*this));
    }

    auto HttpServer::get(std::string uri, Resolver resolver) -> void
    {
        if (uri == "*")
        {
            this->defaultGetResolver = resolver;
        }
        else
        {
            this->addResolver(HttpProtocol::Method::Get, uri, resolver);
        }
    }

    auto HttpServer::post(std::string uri, Resolver resolver) -> void
    {
        this->addResolver(HttpProtocol::Method::Post, uri, resolver);
    }
    auto HttpServer::put(std::string uri, Resolver resolver) -> void
    {
        this->addResolver(HttpProtocol::Method::Put, uri, resolver);
    }
    auto HttpServer::head(std::string uri, Resolver resolver) -> void
    {
        this->addResolver(HttpProtocol::Method::Head, uri, resolver);
    }
    auto HttpServer::resolveDelete(std::string uri, Resolver resolver) -> void
    {
        this->addResolver(HttpProtocol::Method::Delete, uri, resolver);
    }
    auto HttpServer::addResolver(HttpProtocol::Method method, std::string uri, Resolver resolver) -> void
    {
        auto methodString = HttpProtocol::convertMethodToString(method);

        this->resolvers.at(methodString).add(uri, resolver);
    }
    auto HttpServer::getResolver(std::string method, std::string uri, std::string& regexUri) -> std::optional<Resolver>
    {
        auto methodMap = this->resolvers.find(method);

        if (methodMap == this->resolvers.end())
            return std::nullopt;

        auto urisMap = (*methodMap).second;

        auto res = urisMap.match(uri);

        if (res.has_value())
        {
            auto [resolver, rgx] = res.value();

            regexUri = rgx;
            return resolver;
        }

        return std::nullopt;
    }

    auto HttpServer::getResolver(std::string method, std::string uri) -> std::optional<Resolver>
    {
        std::string discard;
        return this->getResolver(method, uri, discard);
    }


    auto HttpServer::getTimeSinceEpoch() -> std::uint64_t
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    }

    auto HttpServer::usePreMiddleware(DefaultPreMiddlewareType middleware) -> void
    {
        this->middlewares.addPre(middleware);
    }
    auto HttpServer::usePostMiddleware(DefaultPostMiddlewareType middleware) -> void
    {
        this->middlewares.addPost(middleware);
    }

    auto HttpServer::sendInternalServerError(stream::IStream& stream) -> void
    {
        stream.write(INTERNAL_SERVER_ERROR.data(), INTERNAL_SERVER_ERROR.size());
        stream.close();
    }
}