#include <iostream>
#include <lightning/httpServer/ServerBuilder.hpp>
#include <lightning/httpServer/MiddlewareContainer.hpp>
#include <lightning/httpServer/ClientHandlerTask.hpp>
#include <openssl/md5.h>
#include <lightning/handlers/StaticFile.hpp>

constexpr const char* PublicKeyPath = "/home/ishaysela/projects/lightningPlusPlus/tests/localhost.cert";
constexpr const char* PrivateKeyPath = "/home/ishaysela/projects/lightningPlusPlus/tests/localhost.key";

auto main() -> int
{
    auto server = lightning::ServerBuilder::createNew(8080)
        .withSsl(PublicKeyPath, PrivateKeyPath)
        .withThreads(1)
        .build();

    auto getForcast = [](lightning::HttpRequest request)
    {
        std::string resp = "<h1>Hello, World!</h1>";
        return lightning::HttpResponseBuilder::create()
            .withBody(resp)
            .withHeader("Content-Type", "text/html")
            .build();
    };

    server.usePreMiddleware([](lightning::HttpRequest& request) -> auto {
        static auto fileResolver = lightning::handlers::serveFolder(".");

        auto response = fileResolver(request);

        return std::optional<lightning::HttpResponse>(response);
        });

    server.get("/forcast", getForcast);

    std::cout << "Server has started on port " << 8080 << '\n';
    server.start();

    return 0;
}