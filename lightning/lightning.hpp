#pragma once
#include <unordered_map>

namespace lightning
{
    static constexpr const char *DOUBLE_CRLF = "\r\n\r\n";
    static constexpr const char *CRLF = "\r\n";

    using HeadersMap = std::unordered_map<std::string, std::string>;

}