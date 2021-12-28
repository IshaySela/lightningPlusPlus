#include "lightning/request/HttpRequest.hpp"
#include <cstring>

namespace lightning
{
    HttpRequest::HttpRequest(std::string method, std::string rawUri, std::string protocolVersion) : method(method), rawUri(rawUri), protocolVersion(protocolVersion) 
    {
    }
    
}