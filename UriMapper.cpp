#include "lightning/uriMapper/UriMapper.hpp"

namespace lightning
{
    auto UriMapper::createExactMatch(std::string uri) -> std::string
    {
        return "";
    }

    auto UriMapper::findSpecialCharacters(std::string uri) -> size_t
    {
        return uri.find('*', uri.length() - 1);
    }

    auto UriMapper::add(std::string uri, HttpServer::Resolver resolver) -> void
    {
        auto specialCharacter = this->findSpecialCharacters(uri);

        if (specialCharacter == std::string::npos)
        {
        }
    }
}