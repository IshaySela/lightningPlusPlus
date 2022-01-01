#include "lightning/httpServer/HttpServer.hpp"

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