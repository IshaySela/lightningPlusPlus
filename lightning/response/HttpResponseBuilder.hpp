#pragma once
#include "../lightning.hpp"
#include "HttpResponse.hpp"
#include <string>

namespace lightning
{
    class HttpResponseBuilder
    {
    private:
        HttpResponseBuilder();

        int statusCode;
        std::string statusPhrase;
        std::string httpVersion;

        std::vector<char> body;
        HeadersMap headers;

    public:
        auto withStatusCode(int statusCode) -> HttpResponseBuilder &;
        auto withStatusPhrase(const char *statusPhrase) -> HttpResponseBuilder &;
        auto withHttpVersion(const char *httpVersion) -> HttpResponseBuilder &;
        auto withHeader(std::string key, std::string value) -> HttpResponseBuilder &;
        auto withBody(std::vector<char> body) -> HttpResponseBuilder &;

        /**
         * @brief Create the http response from all of the data provided.
         */
        auto build() -> HttpResponse;

        /**
         * @brief Create new HttpResponseBuilder
         */
        static auto create() -> HttpResponseBuilder;
    };
} // namespace lightning
