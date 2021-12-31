#pragma once
#include "../SSLServer.hpp"
#include "../request/HttpRequest.hpp"
#include "../response/HttpResponse.hpp"
#include <functional>
#include <future>

namespace lightning
{
    class HttpServer
    {
    private:
        SSLServer lowLevelServer;

    public:
        HttpServer(SSLServer lowLevelServer);
        auto start() -> std::future<void>;

        using Resolver = std::function<HttpResponse(HttpRequest request)>;

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
        auto addResolver(std::string method, std::string uri, Resolver resolver) -> void;
    };
} // namespace lightning
