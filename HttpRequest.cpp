#include "lightning/request/HttpRequest.hpp"
#include <cstring>
#include <sstream>
#include <array>
#include <regex>

namespace lightning
{
    HttpRequest::HttpRequest(std::string method,
                             std::string rawUri,
                             std::string protocolVersion,
                             HeadersMap &headers,
                             FrameworkInfo frameworkInfo) : method(method),
                                                            rawUri(rawUri),
                                                            protocolVersion(protocolVersion),
                                                            headers(headers),
                                                            frameworkInfo(frameworkInfo)
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

    FrameworkInfo &HttpRequest::getFrameworkInfo()
    {
        return this->frameworkInfo;
    }

    auto HttpRequest::getNextToken(std::string line, std::string del, int &outIndex, int offset) -> std::string
    {
        int index = line.find(del, offset);

        if (index == std::string::npos)
        {
            outIndex = line.length();
            return std::string(line.begin() + offset, line.end());
        }

        outIndex = index + del.length(); // Skip the delimiter.
        return std::string(line.begin() + offset, line.begin() + index);
    };

    auto HttpRequest::parseRequestLine(std::string requestLine) -> HttpRequest::RequestLine
    {
        std::string method, uri, version, delimiter = " ";
        int offset = 0;

        method = getNextToken(requestLine, delimiter, offset, offset);
        uri = getNextToken(requestLine, delimiter, offset, offset);
        version = getNextToken(requestLine, " ", offset, offset);

        return HttpRequest::RequestLine{
            .method = method,
            .rawUri = uri,
            .protocolVersion = version};
    }

    auto HttpRequest::parseHeaders(std::string rawHeaders) -> HeadersMap
    {
        std::istringstream keyPairStream;
        std::string header = "", key = "", value = "";
        HeadersMap headersMap;
        int offset = 0;

        while ((header = HttpRequest::getNextToken(rawHeaders, CRLF, offset, offset)) != "")
        {
            int keySepratorIndex = 0;
            key = HttpRequest::getNextToken(header, ": ", keySepratorIndex, 0);
            value = std::string(header.begin() + keySepratorIndex, header.end());

            headersMap.insert({key, value});
        }

        return headersMap;
    }

    auto HttpRequest::createRequest(std::string request) -> HttpRequest
    {
        int requestLineEnd = request.find(lightning::CRLF);

        if (requestLineEnd == std::string::npos)
        {
            throw std::runtime_error("Invalid request sent by client");
        }

        auto [method, uri, version] = parseRequestLine(std::string(request.begin(), request.begin() + requestLineEnd));

        // The raw headers, without the request line and the CRLF at the end of it.
        auto rawHeaders = std::string(request.begin() + requestLineEnd + strlen(lightning::CRLF), request.end());
        auto headers = parseHeaders(rawHeaders);

        return HttpRequest(method, uri, version, headers, FrameworkInfo{.matchedRegex = "", .requestArrivalTime = 0});
    }

    auto HttpRequest::createRequest(std::vector<char>::iterator beg, std::vector<char>::iterator end) -> HttpRequest
    {
        return HttpRequest::createRequest(std::string(beg, end));
    }

    auto HttpRequest::getUriParameters() -> std::vector<std::string> &
    {
        return this->uriParameters;
    }

    auto HttpRequest::computeUriParameters() -> void
    {
        std::smatch result;
        std::vector<std::string> params;
        std::regex regex(this->frameworkInfo.matchedRegex, std::regex_constants::ECMAScript);
        this->uriParameters.clear();


        if (!std::regex_match(rawUri, result, regex))
            return;

        // Proccess each subgruop and get the parameter out of it.
        auto compute = [&params](std::pair<std::string::const_iterator, std::string::const_iterator> val)
        {
            auto &[itStart, itEnd] = val;
            std::string param(itStart.base()), postParam(itEnd.base());

            if (postParam.length() != 0)
            {
                size_t index = param.find(postParam);
                param = param.substr(index == std::string::npos ? 0 : index);
            }

            params.push_back(param);
        };

        std::for_each(++result.begin(), result.end(), compute);

        this->uriParameters = params;
    }
}