#include "lightning/response/HttpResponse.hpp"

namespace lightning
{
    HttpResponse::HttpResponse(StatusLine statusLine, HeadersMap headers, std::vector<char> body) : statusLine(statusLine), headers(headers), body(body)
    {
        this->setHeader("Content-Length", std::to_string(this->body.size()));
    }

    auto HttpResponse::setHeader(std::string key, std::string value) -> void
    {
        this->headers.insert_or_assign(key, value);
    }

    auto HttpResponse::getHeader(std::string key) -> std::optional<std::string>
    {
        auto it = this->headers.find(key);

        if (it == this->headers.end())
        {
            return {};
        }

        return (*it).second;
    }

    auto HttpResponse::getStatusLine() -> StatusLine &
    {
        return this->statusLine;
    }

    auto HttpResponse::getHeaders() -> HeadersMap &
    {
        return this->headers;
    }

}