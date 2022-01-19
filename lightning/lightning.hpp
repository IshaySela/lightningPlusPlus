#pragma once
#include <unordered_map>
#include <string>
#include <functional>

namespace lightning
{
    static constexpr const char *DOUBLE_CRLF = "\r\n\r\n";
    static constexpr const char *CRLF = "\r\n";

    using HeadersMap = std::unordered_map<std::string, std::string>;

    class HttpResponse;
    class HttpRequest;
    using Resolver = std::function<HttpResponse(HttpRequest request)>;
}