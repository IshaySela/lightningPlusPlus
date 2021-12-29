#pragma once
#include <string>
#include <unordered_map>

namespace lightning
{
    class HttpRequest
    {
    private:
        std::string method;
        std::string rawUri;
        std::string protocolVersion;

        using HeadersMap = std::unordered_map<std::string, std::string>;
        HeadersMap headers;

        static auto getNextToken(std::string line, std::string del, int &outIndex, int offset = 0) -> std::string;        
    public:
        HttpRequest(std::string method, std::string rawUri, std::string protocolVersion, HeadersMap &headers);

        std::string &getMethod();
        std::string &getRawUri();
        std::string &getProtocolVersion();

        static constexpr const char *DOUBLE_CRLF = "\r\n\r\n";
        static constexpr const char *CRLF = "\r\n";
        /**
         * @brief Construct a http request from request (without the body) that was sent by the client.
         * The request structure is specified in the rfc standard https://datatracker.ietf.org/doc/html/rfc2616#section-5
         *
         * @param requestLine The raw request line read from the client.
         * @return HttpRequest
         */
        static auto createRequest(std::string request) -> HttpRequest;

        using RequestLine = struct RequestLine
        {
            std::string method;
            std::string rawUri;
            std::string protocolVersion;
        };

        static auto parseRequestLine(std::string requestLine) -> HttpRequest::RequestLine;
        static auto parseHeaders(std::string headers) -> HeadersMap;
    };
}