
#include <iostream>
#include "lightning/SSLServer.hpp"
#include "lightning/request/HttpRequest.hpp"
#include "lightning/response/HttpResponseBuilder.hpp"
#include "lightning/httpServer/HttpServer.hpp"
#include "lightning/TaskExecutor.hpp"
#include "lightning/handlers/StaticFile.hpp"
#include "lightning/uriMapper/UriMapper.hpp"
#include "lightning/IClient.hpp"
#include <openssl/ssl.h>

constexpr auto CERT_FILE_PATH = "/home/ishaysela/projects/lightningPlusPlus/tests/localhost.cert";
constexpr auto PRIVATE_KEY_PATH = "/home/ishaysela/projects/lightningPlusPlus/tests/localhost.key";

void test()
{
    auto wsaCleaner = initWSData();

    lightning::SSLServer sslServer(8080, CERT_FILE_PATH, PRIVATE_KEY_PATH);

    lightning::HttpServer httpServer(sslServer, 2);

    int counter = 0;

    httpServer.get("/*", lightning::handlers::serveFolder("../tests"));
    httpServer.start();
}

int main(int argc, char **argv)
{
    test();
}