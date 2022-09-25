#pragma once
#include "LowLevelApiException.hpp"
#include <openssl/err.h>
#include <iostream>
#include <vector>

namespace lightning
{
    /**
     * @brief The class OpenSslErrorQueueException is an exception that captures the openssl error queue
     * when thrown and stores a copy of it.
     */
    class OpenSslErrorQueueException : public LowLevelApiException
    {
    public:
        /**
         * @brief Captures the errno and error queue of openssl and stores them.
         *
         * @param msg Brief description of the error. Passed to LowLevelApiException
         * @param errorCode Passed to LowLevelApiException.
         */
        OpenSslErrorQueueException(const char* msg, int errorCode, int capturedErrno = 0);

        struct ErrorData
        {
            const char* msg;
            int code;
        };

    private:
        // std::vector<ErrorData> errors;
    };
}