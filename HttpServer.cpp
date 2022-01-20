#include "lightning/httpServer/HttpServer.hpp"
#include "lightning/LowLevelApiException.hpp"
#include <fstream>

namespace lightning
{
    HttpServer::HttpServer(SSLServer lowLevelServer, const int threadCount) : lowLevelServer(lowLevelServer), tasks(threadCount)
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
            auto client = this->lowLevelServer.accept();

            std::vector<char> requestBuffer;

            try
            {
                requestBuffer = client.getStream().readUntilToken("\r\n\r\n");
            }
            catch (const LowLevelApiException &e)
            {
                client.getStream().close();
                continue;
            }

            lightning::HttpRequest request = lightning::HttpRequest::createRequest(std::string(requestBuffer.begin(), requestBuffer.end()));
            auto resolver = this->getResolverOrDefault(request.getMethod(), request.getRawUri());

            this->tasks.add_task(new ResolveAndSend(std::move(client), resolver, request), true);
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

    auto HttpServer::getResolver(std::string method, std::string uri) -> std::optional<Resolver>
    {
        static const std::optional<Resolver> RESOLVER_NOT_FOUND;

        auto methodMap = this->resolvers.find(method);

        if (methodMap == this->resolvers.end())
            return RESOLVER_NOT_FOUND;

        auto urisMap = (*methodMap).second;

        return urisMap.match(uri);
    }

    auto HttpServer::getResolverOrDefault(std::string method, std::string uri) -> Resolver
    {
        static auto defaultResolver = [](HttpRequest request) -> lightning::HttpResponse
        {
            return HttpResponseBuilder::create()
                .withStatusCode(404)
                .withStatusPhrase("Not Found")
                .build();
        };

        auto resolver = this->getResolver(method, uri);

        return resolver.value_or(defaultResolver);
    }

    void HttpServer::ResolveAndSend::operator()()
    {
        auto response = this->resolver(request).toHttpResponse();
        this->client.getStream().write(response.data(), response.size());
    }

    HttpServer::ResolveAndSend::ResolveAndSend(SSLClient client, Resolver resolver, HttpRequest request) : client(std::move(client)), resolver(resolver), request(request) {}

}