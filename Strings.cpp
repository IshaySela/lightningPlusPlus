#include "lightning/uriMapper/Strings.hpp"
#include <regex>

namespace lightning::strings
{
    
    auto sanitizeForRegex(std::string &str) -> std::string
    {
        static const std::regex specialCharacters("\\[|\\]|\\*|\\/|\\?");

        return std::regex_replace(str, specialCharacters, "\\$&");
    }

    auto sanitizeExecptWildcardForRegex(std::string& str) -> std::string
    {
        static const std::regex specialCharactersWithoutWildcard("\\[|\\]|\\/|\\?");

        return std::regex_replace(str, specialCharactersWithoutWildcard, "\\$&");
    }
}