#pragma once
#include <vector>
#include <optional>
#include "StatusLine.hpp"
#include "../lightning.hpp"

namespace lightning
{
    /**
     * @brief The class HttpRespones contains the data that is required to construct a http response,
     * as specified in the rfc https://datatracker.ietf.org/doc/html/rfc2616#section-6.
     */
    class HttpResponse
    {
    private:
        StatusLine statusLine;
        HeadersMap headers;
        std::vector<char> body;
    public:
        HttpResponse(StatusLine statusLine, HeadersMap headers, std::vector<char> body);

        auto setHeader(std::string key, std::string value) -> void;

        auto getHeader(std::string key) -> std::optional<std::string>;

        auto getStatusLine() -> StatusLine &;
        auto getHeaders() -> HeadersMap &;

        auto getBody() const -> const std::vector<char>&;
    
        /**
         * @brief Convert the response to a http response, as sepcified in https://www.rfc-editor.org/rfc/rfc2616.html#section-6
         * 
         * @return std::vector<char> The raw buffer containing the response.
         */
        auto toHttpResponse() -> std::vector<char>;
    };

} // namespace lightning
