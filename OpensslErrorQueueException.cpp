#include "lightning/OpensslErrorQueueException.hpp"


namespace lightning
{
    OpenSslErrorQueueException::OpenSslErrorQueueException(const char* msg, int errorCode, int caputredErrno) : LowLevelApiException(msg, errorCode, caputredErrno)
    {
        int currentError = 0;
        const char* filename, * errorData;
        int line, flags = ERR_TXT_STRING;

        while ((currentError = ERR_get_error_line_data(&filename, &line, &errorData, &flags)) != 0)
        {
            std::cout << "Error code: " << currentError << " with text:\t" << errorData << "\n";
        }
    }

}