#include "lightning/httpServer/HttpServer.hpp"
#include "lightning/response/HttpResponseBuilder.hpp"

namespace lightning
{
    HttpServer::HttpServer(SSLServer lowLevelServer) : lowLevelServer(lowLevelServer)
    {
        for (int i = 0; i < HttpProtocol::supportedHttpMethods.size(); i++)
        {
            auto &method = HttpProtocol::supportedHttpMethods[i];

            this->resolvers.insert({method, std::unordered_map<std::string, Resolver>()});
        }
    }

    auto HttpServer::start() -> void
    {
        int counter = 0;

        while (true)
        {
            auto client = this->lowLevelServer.accept();

            auto headers = client.getStream().readUntilToken("\r\n\r\n");

            lightning::HttpRequest request = lightning::HttpRequest::createRequest(std::string(headers.begin(), headers.end()));

            std::string example = "Hello, World #" + std::to_string(counter++);

            auto response = lightning::HttpResponseBuilder::create()
                                .withBody(std::vector<char>(example.begin(), example.end()))
                                .withHeader("Content-Type", "text/html")
                                .build()
                                .toHttpResponse();

            
            client.getStream().write(response.data(), response.size());
        }
    }

    auto HttpServer::get(std::string uri, Resolver resolver) -> void
    {
        this->addResolver(HttpProtocol::Method::Get, uri, resolver);
    }

    auto HttpServer::post(std::string uri, Resolver resolver) -> void
    {
        this->addResolver(HttpProtocol::Method::Post, uri, resolver);
    }

    auto HttpServer::addResolver(HttpProtocol::Method method, std::string uri, Resolver resolver) -> void
    {
        auto methodString = HttpProtocol::convertMethodToString(method);

        this->resolvers.at(methodString).insert(std::pair<std::string, Resolver>{uri, resolver});
    }

}