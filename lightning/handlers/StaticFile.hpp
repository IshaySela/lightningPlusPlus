#pragma once
#include "../lightning.hpp"
#include "../response/HttpResponse.hpp"
#include "../response/HttpResponseBuilder.hpp"
#include "../request/HttpRequest.hpp"
#include <string>

namespace lightning::handlers
{
    const auto NOT_FOUND_RESOLVER = [](HttpRequest request) -> HttpResponse
    {
        static const std::string body = "<h1>404 Not Found</h1>";

        return HttpResponseBuilder::create()
            .withStatusCode(404)
            .withStatusPhrase("Not Found")
            .withBody(body)
            .withHeader("Content-Type", "text/html")
            .withHeader("Content-Length", std::to_string(body.size()))
            .build();
    };

    /**
     * @brief Create a resolver for a static file.
     * 
     * @param path The path to the file.
     * @param contentType the content type header in the response to the client.
     * @param notFoundResolver Resolver that is called if the file coult not be read.
     * @return Resolver The resolver that is serving the file.
     */
    auto uriToStaticFile(std::string path, std::string contentType = "text/html", Resolver notFoundResolver = NOT_FOUND_RESOLVER) -> Resolver;

    /**
     * @brief Serve entire folder based on the request uri.
     * 
     * @param path The path to the folder
     * @param notFoundResolver Resolver that is called if the file coult not be read.
     * 
     * @return Resolver The resolver for serving the folder.
     */
    auto serveFolder(std::string path, Resolver notFoundResolver = NOT_FOUND_RESOLVER) -> Resolver;
}