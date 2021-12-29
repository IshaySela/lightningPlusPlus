
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <winsock2.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "lightning/LowLevelApiException.hpp"
#include <sstream>
#include "lightning/WSAInitializer.hpp"
#include "lightning/SSLServer.hpp"
#include "lightning/request/HttpRequest.hpp"

constexpr auto CERT_FILE_PATH = "C:\\msys64\\usr\\httpFramework\\cert\\localhost\\localhost.crt";
constexpr auto PRIVATE_KEY_PATH = "C:\\msys64\\usr\\httpFramework\\cert\\localhost\\localhost.decrypted.key";

std::string buildReply(int amount)
{
    std::stringstream reply;
    std::string content = "Hello, Counter! " + std::to_string(amount);

    reply << "HTTP/1.1 200 OK\n"
             "Content-Type:text/html\n"
             "Content-Length:"
          << content.length() << "\n"
          << "\r\n\r\n"
          << content;

    return reply.str();
}

void test()
{
    WSADATA data;
    auto versionRequired = MAKEWORD(2, 2);
    int error;
    if ((error = WSAStartup(versionRequired, &data)) != 0)
    {
        throw lightning::LowLevelApiException("Error while calling WSAStartup()", error);
    }

    lightning::SSLServer server(8080, CERT_FILE_PATH, PRIVATE_KEY_PATH);

    int counter = 0;

    while (true)
    {
        auto client = server.accept();
        auto reply = buildReply(counter);

        auto headers = client.getStream().readUntilToken("\r\n\r\n");

        lightning::HttpRequest request = lightning::HttpRequest::createRequest(std::string(headers.begin(), headers.end()));

        client.getStream().write(reply.data(), reply.size());

        counter++;
    }

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
