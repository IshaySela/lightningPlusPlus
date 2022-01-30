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

    // An array of all the supported http methods,
    // ordered by their declartion in the enum class Method.
    const std::array<std::string, static_cast<int>(Method::Methods_COUNT)> supportedHttpMethods =
        {{std::string("OPTIONS"),
          "GET",
          "HEAD",
          "POST",
          "PUT",
          "DELETE",
          "TRACE",
          "CONNECT"}};

    /**
     * @brief Convert an of type Method to the string value of that method.
     * convertMethodToString(Method::Get) -> "GET"
     * convertMethodToString(Method::Post) -> "POST"
     *
     * @param method The method to convert.
     * @return std::string The string value of the method.
     */
    inline auto convertMethodToString(const Method method) -> std::string
    {
        int index = static_cast<int>(method);
        return supportedHttpMethods.at(index);
    }
}