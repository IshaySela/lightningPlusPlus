#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include "../lightning.hpp"
#include "FrameworkInfo.hpp"

namespace lightning
{
    class HttpRequest
    {
    private:
        std::string method;
        std::string rawUri;
        std::string protocolVersion;
        std::vector<std::string> uriParameters;
        FrameworkInfo frameworkInfo;

        HeadersMap headers;

        /**
         * @brief Parse the line and retrive every character from the offset until reaching the delimiter.
         *
         * @param line The string to get the substr from.
         * @param del The delimiter to find.
         * @param outIndex Output variable that will contain the index of the found delimiter. If the delimiter was not found, it will be the length of line.
         * @param offset The offset character to start from.
         * @return std::string The substring from offset to del.
         */
        static auto getNextToken(std::string line, std::string del, int &outIndex, int offset = 0) -> std::string;

    public:
        HttpRequest(std::string method, std::string rawUri, std::string protocolVersion, HeadersMap &headers, FrameworkInfo frameworkInfo);

        auto getMethod() -> std::string &;
        auto getRawUri() -> std::string &;
        auto getProtocolVersion() -> std::string &;
        auto getFrameworkInfo() -> FrameworkInfo &;
        auto getUriParameters() -> std::vector<std::string> &;
        auto getHeaders() -> HeadersMap&;

        /**
         * @brief Get a header from the headers map.
         * 
         * @param key The header key.
         * @return std::optional<std::string> An optional value that contains the value of the header if the header exists.
         */
        auto getHeader(std::string key) -> std::optional<std::string>;

        /**
         * @brief Calculate the uri paramters.
         */
        auto computeUriParameters() -> void;

        /**
         * @brief Construct a http request from request (without the body) that was sent by the client.
         * The request structure is specified in the rfc standard https://datatracker.ietf.org/doc/html/rfc2616#section-5
         *
         * @param requestLine The raw request line read from the client.
         * @return HttpRequest
         */
        static auto createRequest(std::string request) -> HttpRequest;
        static auto createRequest(std::vector<char>::iterator beg, std::vector<char>::iterator end) -> HttpRequest;

        using RequestLine = struct RequestLine
        {
            std::string method;
            std::string rawUri;
            std::string protocolVersion;
        };

        /**
         * @brief Parse the request line sent by the client.
         * The request line should be as specified in https://datatracker.ietf.org/doc/html/rfc2616#section-5.1.
         * General request line will be structured as follows: Method SP Request-URI SP HTTP-Version CRLF
         *
         * @param requestLine The request line to parse.
         * @return HttpRequest::RequestLine An object that contains the method, request uri and http version.
         */
        static auto parseRequestLine(std::string requestLine) -> HttpRequest::RequestLine;

        /**
         * @brief Parse a string that is structured as specified in the rfc https://datatracker.ietf.org/doc/html/rfc2616#section-4.2.
         * General headers sent by the client will be structured as follows: Key ": " Value CRLF Key ": " Value ...
         *
         * @param rawHeaders The raw headers string.
         * @return HeadersMap An object that maps string keys to string values.
         */
        static auto parseHeaders(std::string rawHeaders) -> HeadersMap;
    };
}