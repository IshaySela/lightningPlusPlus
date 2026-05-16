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
         * @param appErrorCode The error code for the exception.
         * @param capturedErrno The errno value of the error
         */
        LowLevelApiException(const char *msg, int appErrorCode, int capturedErrno = 0);

        const int appErrorCode;
        const int capturedErrno;
    };
}