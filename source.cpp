
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "lightning/LowLevelApiException.hpp"
#include <sstream>
#include "lightning/SSLServer.hpp"
#include "lightning/request/HttpRequest.hpp"
#include "lightning/response/HttpResponseBuilder.hpp"
#include "lightning/HttpProtocol.hpp"
#include "lightning/httpServer/HttpServer.hpp"
#include "lightning/sockets.hpp"
#include <regex>
#include "lightning/uriMapper/Strings.hpp"
#include <optional>
#include "lightning/uriMapper/UriMapper.hpp"

constexpr auto CERT_FILE_PATH = "/home/ishaysela/projects/lightningPlusPlus/tests/localhost.cert";
constexpr auto PRIVATE_KEY_PATH = "/home/ishaysela/projects/lightningPlusPlus/tests/localhost.key";

void test()
{
    auto wsaCleaner = initWSData();

    lightning::SSLServer sslServer(8080, CERT_FILE_PATH, PRIVATE_KEY_PATH);

    lightning::HttpServer httpServer(sslServer);

    int counter = 0;
    auto testResolver = [&counter](lightning::HttpRequest request) -> lightning::HttpResponse
    {
        // Test program:
        std::string example = "Hello, World #" + std::to_string(counter++);

        auto response = lightning::HttpResponseBuilder::create()
                            .withBody(std::vector<char>(example.begin(), example.end()))
                            .withHeader("Content-Type", "text/html")
                            .withHeader("Content-Length", std::to_string(example.size()))
                            .withStatusCode(200)
                            .withStatusPhrase("OK")
                            .build();

        return response;
    };

    httpServer.get("/", testResolver);

    httpServer.start();
}

int main(int argc, char **argv)
{
    try
    {
        test();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }

    std::cout << "Ended gracefully" << std::endl;
}