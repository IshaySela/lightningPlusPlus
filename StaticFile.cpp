#include "lightning/handlers/StaticFile.hpp"

#include <fstream>
#include <iostream>

namespace lightning::handlers
{
    auto uriToStaticFile(std::string path, std::string contentType, Resolver notFoundResolver) -> Resolver
    {
        const auto resolver = [path, contentType, notFoundResolver](HttpRequest request) -> HttpResponse 
        {
            std::ifstream file(path);

            if(!file.is_open())
                return notFoundResolver(request);

            std::string out;
            std::cout << "Serving file of type " << contentType << " " << path << " is open? " << file.is_open() << std::endl;            
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
} // namespace lightning::handlers
