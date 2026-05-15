#include <iostream>
#include "lightning/httpServer/ServerBuilder.hpp"
#include <lightning/httpServer/MiddlewareContainer.hpp>
#include <lightning/httpServer/ClientHandlerTask.hpp>
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
        .withUnderlyingServer(std::make_unique<lightning::PlainServer>(8080))
        .withThreads(7)
        .build();

    auto benchmark = [](lightning::HttpRequest request)
    {
        auto contentLengthHeader = request.getHeader("Content-Length");
        std::cout << "Handling request for " << request.getRawUri() << "With content length" << contentLengthHeader.value_or("no_header_set") << '\n';
        
        auto body = request.getBody();
        std::string resp;
        auto obj = nlohmann::json::parse(body.begin(), body.end());
        
        if (obj.contains("name"))
        {
            resp = "hello " + obj["name"].get<std::string>() + "!";
        }
        else
        {
            resp = "Hello! Please provide a name in the body as json!";
        }

        return lightning::HttpResponseBuilder::create()
            .withBody(resp)
            .withHeader("Content-Type", "text/html")
            .withHeader("Content-Length", std::to_string(resp.size()))
            .build();
    };

    server.post("/", benchmark);

    std::cout << "Server has started on port " << 8080 << '\n';
    server.start();

    return 0;
}