#pragma once
#include <array>

namespace lightning::HttpProtocol
{
    // An enum that contains all of the possible HTTP methods supported.
    enum class Method
    {
        Options = 0,  // https://www.rfc-editor.org/rfc/rfc2616.html#section-9.2
        Get = 1,      // https://www.rfc-editor.org/rfc/rfc2616.html#section-9.3
        Head = 2,     // https://www.rfc-editor.org/rfc/rfc2616.html#section-9.4
        Post = 3,     // https://www.rfc-editor.org/rfc/rfc2616.html#section-9.5
        Put = 4,      // https://www.rfc-editor.org/rfc/rfc2616.html#section-9.6
        Delete = 5,   // https://www.rfc-editor.org/rfc/rfc2616.html#section-9.7
        Trace = 6,    // https://www.rfc-editor.org/rfc/rfc2616.html#section-9.8
        Connect = 7,  // https://www.rfc-editor.org/rfc/rfc2616.html#section-9.9,
        Methods_COUNT // Used to evaluate how many elements are in Method.
    };

    /**
     * @brief Convert an of type Method to the string value of that method.
     * convertMethodToString(Method::Get) -> "GET"
     * convertMethodToString(Method::Post) -> "POST"
     * convertMethodToString(Methods::Methods_COUNT) -> throw exception.
     *
     * @param method The method to convert.
     * @return std::string The string value of the method.
     */
    auto convertMethodToString(Method method) -> std::string
    {
        static const std::array<std::string, static_cast<int>(Methods::Methods_COUNT - 1)> methodToString =
            {{"OPTIONS",
              "GET",
              "HEAD",
              "POST",
              "PUT",
              "DELETE",
              "TRACE",
              "CONNECT"}};

        return methodToString[method];
    }
}