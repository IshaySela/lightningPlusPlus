#pragma once
#include <stdexcept>
#include <string>

namespace lightning
{
    class LowLevelApiException : public std::runtime_error
    {
    public:
        /**
         * @brief Describe a general error that has occured while consuming a low level C API.
         * 
         * @param msg The message string to pass to runtime_error.
         * @param errorCode The error code for the exception.
         */
        LowLevelApiException(const char *msg, int errorCode);
        const char *what() const noexcept override;

        const int errorCode;
    };
}