#include "lightning/handlers/StaticFile.hpp"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <regex>

namespace lightning::handlers
{
    auto uriToStaticFile(std::string path, std::string contentType, Resolver notFoundResolver) -> Resolver
    {
        const auto resolver = [path, contentType, notFoundResolver](HttpRequest request) -> HttpResponse
        {
            std::ifstream file(path);

            if (!file.is_open())
                return notFoundResolver(request);

            std::string out;
            std::getline(file, out, '\0');

            return HttpResponseBuilder::create()
                .withBody(out)
                .withHeader("Content-Type", contentType)
                .withHeader("Content-Length", std::to_string(out.size()))
                .withStatusCode(200)
                .withStatusPhrase("OK")
                .build();
        };
        return resolver;
    }

    auto serveFolder(std::string folderPath, Resolver notFoundResolver) -> Resolver
    {
        const auto resolver = [folderPath, notFoundResolver](HttpRequest request) -> HttpResponse
        {
            std::string path = folderPath + *(request.getUriParameters().end() - 1); // TODO: Add feature to ignore the query string at the end.

            std::ifstream file(path);

            if (!file.is_open())
                return notFoundResolver(request);

            std::string out;
            std::getline(file, out, '\0');

            return HttpResponseBuilder::create()
                .withBody(out)
                .withHeader("Content-Type", "text/html") // TODO: Infer content type based on file extension.
                .withHeader("Content-Length", std::to_string(out.size()))
                .withStatusCode(200)
                .withStatusPhrase("OK")
                .build();
        };

        return resolver;
    }
} // namespace lightning::handlers
