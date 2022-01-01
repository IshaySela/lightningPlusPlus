#pragma once
#include "../SSLServer.hpp"
#include "../request/HttpRequest.hpp"
#include "../response/HttpResponse.hpp"
#include "../HttpProtocol.hpp"
#include <functional>
#include <future>
#include <unordered_map>

namespace lightning
{
    class HttpServer
    {
    public:
        using Resolver = std::function<HttpResponse(HttpRequest request)>;
        // An unordered_map that maps methods to maps of URIs.
        using ResolversMap = std::unordered_map<std::string, std::unordered_map<std::string, Resolver>>;

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
         * @brief Map a resolve function to a method and uri.
         *
         * @param method The method to map to.
         * @param uri The uri to map to.
         * @param resolver The functions that resolves the request.
         */
        auto addResolver(HttpProtocol::Method method, std::string uri, Resolver resolver) -> void;

    private:
        SSLServer lowLevelServer;
        HttpServer::ResolversMap resolvers;
    };
} // namespace lightning
