
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <winsock2.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "lightning/LowLevelApiException.hpp"
#include <sstream>
#include <limits>
#include "lightning/WSAInitializer.hpp"
#include "lightning/SSLServer.hpp"
#include "lightning/request/HttpRequest.hpp"
#include "lightning/response/HttpResponseBuilder.hpp"
#include "lightning/HttpProtocol.hpp"
#include "lightning/httpServer/HttpServer.hpp"

constexpr auto CERT_FILE_PATH = "C:\\msys64\\usr\\httpFramework\\cert\\localhost\\localhost.crt";
constexpr auto PRIVATE_KEY_PATH = "C:\\msys64\\usr\\httpFramework\\cert\\localhost\\localhost.decrypted.key";

void test()
{
    WSADATA data;
    auto versionRequired = MAKEWORD(2, 2);
    int error;
    if ((error = WSAStartup(versionRequired, &data)) != 0)
    {
        throw lightning::LowLevelApiException("Error while calling WSAStartup()", error);
    }

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

    WSACleanup();
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
