#include "lightning/httpServer/HttpServer.hpp"
#include "lightning/LowLevelApiException.hpp"
#include <fstream>

namespace lightning
{
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
            auto &method = HttpProtocol::supportedHttpMethods[i];

            this->resolvers.insert({method, UriMapper()});
        }
    }

    auto HttpServer::start() -> void
    {
        int counter = 0;

        while (true)
        {
            auto client = this->lowLevelServer->accept();
            auto requestArrivalTime = HttpServer::getTimeSinceEpoch();
            std::string matchedRegex;

            std::vector<char> requestBuffer;

            try
            {
                requestBuffer = client->getStream().readUntilToken("\r\n\r\n");
            }
            catch (const LowLevelApiException &e)
            {
                client->getStream().close();
                continue;
            }

            lightning::HttpRequest request = lightning::HttpRequest::createRequest(requestBuffer.begin(), requestBuffer.end());

            auto resolver = this->getResolver(request.getMethod(), request.getRawUri(), matchedRegex).value_or(HttpServer::defaultResolver);

            request.getFrameworkInfo() = lightning::FrameworkInfo{.matchedRegex = matchedRegex, .requestArrivalTime = requestArrivalTime};
            
            auto resolveAndSend = new ResolveAndSend(std::move(client), resolver, request);
            this->tasks.add_task( resolveAndSend, true);
        }
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

    auto HttpServer::getResolver(std::string method, std::string uri, std::string &regexUri) -> std::optional<Resolver>
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

    void HttpServer::ResolveAndSend::operator()()
    {
        // Here all of the middlewares (future) and pre resolve tasks execute

        // This is needs to be done on the working thread
        // And can only be done after frameworkInfo was injected.
        request.computeUriParameters();

        auto response = this->resolver(request).toHttpResponse();
        this->client->getStream().write(response.data(), response.size());
    }

    HttpServer::ResolveAndSend::ResolveAndSend(std::unique_ptr<IClient> client, Resolver resolver, HttpRequest request) : client(std::move(client)), resolver(resolver), request(request) {}

    auto HttpServer::getTimeSinceEpoch() -> std::uint64_t
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    }
}