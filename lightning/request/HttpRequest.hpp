#pragma once
#include <string>


namespace lightning
{
    class HttpRequest
    {
    private:
        std::string method;
        std::string rawUri;
        std::string protocolVersion;

    public:
        HttpRequest(std::string method, std::string rawUri, std::string protocolVersion);

        /**
         * @brief Construct a http request from a raw request line, as specified
         * in the rfc standard https://datatracker.ietf.org/doc/html/rfc2616#section-5.1
         *
         * @param requestLine The raw request line read from the client.
         * @return HttpRequest
         */
        static auto createRequestFromRequestLine(std::string requestLine) -> HttpRequest;
    };
}