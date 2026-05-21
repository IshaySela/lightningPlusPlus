#include <iostream>
#include "lightning/httpServer/ServerBuilder.hpp"
#include <lightning/httpServer/MiddlewareContainer.hpp>
#include <openssl/md5.h>
#include <lightning/middlewares/middlewares.hpp>
#include <string>
#include <lightning/PlainServer.hpp>
#include "nlohmann/json.hpp"

constexpr const char* PublicKeyPath = "/home/ishay/projects/lightningPlusPlus/tests/localhost.cert";
constexpr const char* PrivateKeyPath = "/home/ishay/projects/lightningPlusPlus/tests/localhost.key";

auto main() -> int
{
    auto server = lightning::ServerBuilder::createNew(8080)
        .withThreads(32)
        .build();

    auto benchmark = [](lightning::HttpRequest request)
    {
        auto contentLengthHeader = request.getHeader("Content-Length");

        return lightning::HttpResponseBuilder::create()
            .withHtmlBody("<h1>Hello!</h1>")
            .withStatusCode(200)
            .withStatusPhrase("OK")
            .build();
    };

    server.post("/", benchmark);

    std::cout << "Server started on port " << 8080 << '\n';
    server.start();

    return 0;
}