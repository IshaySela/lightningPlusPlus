#pragma once
#include "../SSLServer.hpp"
#include "../request/HttpRequest.hpp"
#include "../response/HttpResponse.hpp"
#include "../HttpProtocol.hpp"
#include "../response/HttpResponseBuilder.hpp"
#include "../lightning.hpp"
#include "../uriMapper/UriMapper.hpp"
#include <functional>
#include <future>
#include <unordered_map>

namespace lightning
{
    class HttpServer
    {
    public:
        // An unordered_map that maps methods to maps of URIs.
        using ResolversMap = std::unordered_map<std::string, UriMapper>;

        /**
         * @brief Construct a new Http Server object, and initilize the resolver to contain an empty map
         * for each supported HTTP method.
         *
         * @param lowLevelServer The underlying low level server API that is used to accept new
         * clients. ; TODO: Convert SSLServer to a pure virtual class and pass a pointer to the class instead.
         */
        HttpServer(SSLServer lowLevelServer);

        /**
         * @brief Start handling clients from the server.
         */
        auto start() -> void;

        /**
         * @brief Call HttpServer::adResolver with the method parameter as "GET"
         */
        auto get(std::string uri, Resolver resolver) -> void;

        /**
         * @brief Call HttpServer::addResolver with the method parameter as "POST".
         */
        auto post(std::string uri, Resolver resolver) -> void;

        /**
         * @brief Call HttpServer::addResolver with the method parameter as "PUT".
         */
        auto put(std::string uri, Resolver resolver) -> void;

        auto head(std::string uri, Resolver resolver) -> void;
        /**
         * @brief Call HttpServer::addResolver with the method parameter as "DELETE".
         */
        auto resolveDelete(std::string uri, Resolver resolver) -> void;
        /**
         * @brief Map a resolve function to a method and uri.
         *
         * @param method The method to map to.
         * @param uri The uri to map to.
         * @param resolver The functions that resolves the request.
         */
        auto addResolver(HttpProtocol::Method method, std::string uri, Resolver resolver) -> void;

        /**
         * @brief Get the resolver from the resolvers map, based on the method and uri.
         *
         * @param method The method of the resolver.
         * @param uri The uri of the resolver
         * @return std::optional<Resolver> An optinal object that contains the resolver, if one exists.
         */
        auto getResolver(std::string method, std::string uri) -> std::optional<Resolver>;

        /**
         * @brief Call HttpServer::getResolver, if no resolver was found return the default resolver.
         * otherwise, return the resolver from HttpServer::getResolver.
         *
         * @param method The method of the resolver.
         * @param uri The uri of the resolver.
         * @return Resolver The return of HttpServer::getResolver, or the default request resolver.
         */
        auto getResolverOrDefault(std::string method, std::string uri) -> Resolver;
    private:
        SSLServer lowLevelServer;
        HttpServer::ResolversMap resolvers;

        Resolver defaultGetResolver;
    };
} // namespace lightning
