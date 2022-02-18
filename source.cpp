#include <iostream>
#include <lightning/httpServer/ServerBuilder.hpp>
#include <lightning/httpServer/MiddlewareContainer.hpp>
#include <lightning/httpServer/ClientHandlerTask.hpp>

constexpr const char* PublicKeyPath = "/home/ishaysela/projects/lightningPlusPlus/tests/localhost.cert";
constexpr const char* PrivateKeyPath = "/home/ishaysela/projects/lightningPlusPlus/tests/localhost.key";

auto main() -> int
{
    // auto server = lightning::ServerBuilder::createNew(8080)
    //                   .withSsl(PublicKeyPath, PrivateKeyPath)
    //                   .withThreads(1)
    //                   .build();

    // auto getForcast = [](lightning::HttpRequest request)
    // {
    //     int x;
    //     return lightning::HttpResponseBuilder::create()
    //         .build();
    // };
    // server.get("/forcast", getForcast);

    // server.start();


    return 0;
}