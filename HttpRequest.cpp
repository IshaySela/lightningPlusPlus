#include "lightning/request/HttpRequest.hpp"
#include <cstring>
#include <sstream>
#include <array>

namespace lightning
{
    HttpRequest::HttpRequest(std::string method, std::string rawUri, std::string protocolVersion) : method(method), rawUri(rawUri), protocolVersion(protocolVersion)
    {
    }

    std::string &HttpRequest::getMethod()
    {
        return this->method;
    }
    std::string &HttpRequest::getRawUri()
    {
        return this->rawUri;
    }
    std::string &HttpRequest::getProtocolVersion()
    {
        return this->protocolVersion;
    }

    auto createRequestFromRequestLine(std::string requestLine) -> HttpRequest
    {
        auto getNext = [](std::string line, int offset = 0) -> std::string
        {
            std::string chunk;
            int index = offset;

            while (line[index] != ' ')
            {
                chunk.push_back(line[index]);
            }

            return chunk;
        };

        std::string method, uri, version;

        method = getNext(requestLine);
        uri = getNext(requestLine, method.length());
        version = getNext(requestLine, method.length() + uri.length());

        return HttpRequest(method, uri, version);
    }
}