#include "lightning/LowLevelApiException.hpp"
#include <iostream>
#include <sstream>

namespace lightning
{
    LowLevelApiException::LowLevelApiException(const char* msg, int appErrorCode, int caputredErrno) :
        runtime_error("Error Code\t" + std::to_string(appErrorCode) + "\twith errno\t" + std::to_string(caputredErrno) + "::\t" + msg),
        appErrorCode(appErrorCode),
        capturedErrno(capturedErrno)
    {
    }

}