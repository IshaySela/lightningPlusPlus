#pragma once
#include "../httpServer/HttpServer.hpp"
#include "../handlers/StaticFile.hpp"


namespace lightning::middleware
{
    using lightning::DefaultPreMiddlewareType;

    /**
     * @brief The middleware useFolder creates a middleware that is calling the serveFolder handler, and checking if the file was found.
     * if the file was found, return the result from serverFolder.
     * otherwise, continue in the middleware chain.
     * 
     * @warning This method is unsafe and a crafted request can view every file in the filesystem, since input is not santized (mainly for ..)
     * 
     * @param path The path to serve files from.
     * @return DefaultPreMiddlewareType The created middleware
     */
    auto useFolder(std::string path) -> DefaultPreMiddlewareType
    {
        const DefaultPreMiddlewareType middleware = [path](lightning::HttpRequest& req) -> PreMiddlewareReturn
        {
            static const auto fileResovler = handlers::serveFolder(path);

            auto response = fileResovler(req);

            if (response.getStatusLine().statusCode == 404)
            {
                return lightning::Continue;
            }

            return response;
        };

        return middleware;
    }
} // namespace lightning::middleware
