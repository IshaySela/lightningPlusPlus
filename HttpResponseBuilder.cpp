#include "lightning/response/HttpResponseBuilder.hpp"

namespace lightning
{
    HttpResponseBuilder::HttpResponseBuilder() : statusCode(200), statusPhrase("OK"), httpVersion("1.1")
    {
    }

    auto HttpResponseBuilder::create() -> HttpResponseBuilder
    {
        return HttpResponseBuilder();
    }
    auto HttpResponseBuilder::build() -> HttpResponse
    {
        return HttpResponse(StatusLine(this->statusCode, this->statusPhrase.c_str(), this->httpVersion.c_str()), this->headers, this->body);
    }

    auto HttpResponseBuilder::withStatusCode(int statusCode) -> HttpResponseBuilder &
    {
        this->statusCode = statusCode;
        return *this;
    }

    auto HttpResponseBuilder::withStatusPhrase(const char *statusPhrase) -> HttpResponseBuilder &
    {
        this->statusPhrase = statusPhrase;
        return *this;
    }

    auto HttpResponseBuilder::withHttpVersion(const char *httpVersion) -> HttpResponseBuilder &
    {
        this->httpVersion = httpVersion;
        return *this;
    }

    auto HttpResponseBuilder::withBody(std::vector<char> body) -> HttpResponseBuilder &
    {
        this->body = body;
        return *this;
    }

    auto HttpResponseBuilder::withHeader(std::string key, std::string value) -> HttpResponseBuilder &
    {
        this->headers.insert_or_assign(key, value);
        return *this;
    }

}