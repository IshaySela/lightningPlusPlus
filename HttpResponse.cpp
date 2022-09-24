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

    auto HttpResponse::toHttpResponse() -> std::vector<char>
    {
        std::stringstream responseBuffer;
        responseBuffer << this->statusLine.createStatusLine();

        for (auto &[key, value] : this->headers)
        {
            responseBuffer << key << ':' << value << "\r\n";
        }

        // Denote the end of the headers
        responseBuffer << "\r\n";

        if (this->body.size() > 0)
        {
            responseBuffer << this->body.data();
        }

        auto buffer = responseBuffer.str();
   
        return std::vector<char>(buffer.begin(), buffer.end());
    }

    auto HttpResponse::getBody() const -> const std::vector<char>&
    {
        return this->body;
    }
}