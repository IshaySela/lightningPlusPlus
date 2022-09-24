#pragma once
#include "../lightning.hpp"
#include "../response/HttpResponse.hpp"

namespace lightning::middleware
{
    auto addHeaderToResponse(std::string key, std::string value) -> DefaultPostMiddlewareType
    {
        DefaultPostMiddlewareType middleware = [key, value](HttpResponse& resp) -> bool
        {
            resp.setHeader(key, value);
            return true;
        };

        return middleware;
    }
}