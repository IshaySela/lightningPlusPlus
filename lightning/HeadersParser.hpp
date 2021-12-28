#pragma once
#include <vector>
#include <unordered_map>
#include "stream/IStream.hpp"

namespace lightning::parser
{
    using Headers = std::unordered_map<std::string, std::string>;

    constexpr const char *CRLF = "\r\n\r\n";
    /**
     * @brief Read the data from the stream until reaching CRLF, parse the HTTP line and the headers.
     *
     * @param stream The stream to read from.
     * @return Headers A map of headers to 
     */
    auto parseHeadersFromStream(stream::IStream &stream) -> Headers;
}