#include "lightning/LowLevelApiException.hpp"
#include <iostream>
#include <sstream>

namespace lightning
{
    LowLevelApiException::LowLevelApiException(const char *msg, int errorCode) : runtime_error("Error " + std::to_string(errorCode) + "::\t" + msg), errorCode(errorCode) 
    {
    }

    const char *LowLevelApiException::what() const noexcept
    {
        return runtime_error::what();
    }

}