#pragma once
#include <unordered_map>
#include <string>
#include <functional>
#include "TaskExecutor.hpp"
#include <optional>

namespace lightning
{
    static constexpr const char* DOUBLE_CRLF = "\r\n\r\n";
    static constexpr const char* CRLF = "\r\n";

    using HeadersMap = std::unordered_map<std::string, std::string>;

    class HttpResponse;
    class HttpRequest;
    using Resolver = std::function<HttpResponse(HttpRequest request)>;


    template<typename T>
    struct comply_with_task
    {
        static constexpr bool value = Task<T>;
    };

    using DefaultPostMiddlewareType = std::function<bool(HttpResponse&)>;
    using PreMiddlewareReturn = std::optional<HttpResponse>;

    /**
     * @brief The PreMiddlewareType recives an HttpRequest and returns and optional HttpResponse.
     * If the optional contains a value, that value will be sent back to the client. Otherwise,
     * the middleware chain will continue.
     */
    using DefaultPreMiddlewareType = std::function<PreMiddlewareReturn(HttpRequest&)>;
}