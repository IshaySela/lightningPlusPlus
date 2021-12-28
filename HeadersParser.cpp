#include "lightning/HeadersParser.hpp"

namespace lightning::parser
{

    auto parseHeadersFromStream(stream::IStream &stream) -> Headers
    {
        Headers headers;

        auto rawHeadersVector = stream.readUntilToken(parser::CRLF);
        std::string rawHeaders(rawHeadersVector.begin(), rawHeadersVector.end());
        return headers;
    }

}