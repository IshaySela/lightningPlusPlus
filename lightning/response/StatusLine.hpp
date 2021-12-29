#pragma once
#include <sstream>

namespace lightning
{
    using StatusLine = struct StatusLine
    {
        int statusCode;

        const char *statusPhrase;
        const char *httpVersion;

        /**
         * @brief Construct a new Status Line object.
         *
         * @param statusCode The status code.
         * @param statusPhrase The status phrase.
         * @param httpVersion The http version. Only the number without the HTTP - 1.1, 1.0, 2.0 etc.
         */
        StatusLine(int statusCode, const char *statusPhrase, const char *httpVersion) : statusCode(statusCode),
                                                                                        statusPhrase(statusPhrase),
                                                                                        httpVersion(httpVersion)
        {
        }

        /**
         * @brief Create the status line as specified in the rfc https://datatracker.ietf.org/doc/html/rfc2616#section-6.1.
         * A status line would be structured as follows: HTTP-Version SP Status-Code SP Reason-Phrase CRLF
         *
         * @return std::string The raw reason phrace constructed from the members.
         */
        auto createStatusLine() -> std::string
        {
            std::stringstream stream;

            stream << "HTTP/" << this->httpVersion << " " << std::to_string(this->statusCode) << " " << this->statusPhrase;

            return stream.str();
        }
    };
} // namespace lightning
