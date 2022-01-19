#include "lightning/uriMapper/UriMapper.hpp"
#include "lightning/uriMapper/Strings.hpp"

namespace lightning
{
    auto UriMapper::createExactMatch(std::string uri) -> std::string
    {
        static const std::string exactMatchTemplate = "^{}$"; // <start of string> <uri literal> <end of string>.
        auto literal = strings::sanitizeForRegex(uri);

        auto result = strings::format<std::string>(exactMatchTemplate, literal).value_or("");

        if (result.length() == 0)
        {
            throw std::runtime_error("Error while formatting the template");
        }

        return result;
    }

    auto UriMapper::containsWildcard(std::string uri) -> size_t
    {
        auto wildcardIndex = uri.find('*');
        
        if (wildcardIndex == std::string::npos)
        {
            return std::string::npos;
        }

        char charBefore = wildcardIndex > 0 ? uri[wildcardIndex - 1] : '\0';

        return charBefore == '/' ? wildcardIndex : std::string::npos;
    }

    auto UriMapper::createWithWildcard(std::string uri) -> std::string
    {
        static const std::string wildcardRegex = "(\\/.*|$)";
        static const std::string seprator = "\\/*";

        auto cleanUri = strings::sanitizeExecptWildcardForRegex(uri);

        return strings::formatEx<std::string>(cleanUri, seprator, wildcardRegex).value_or(uri); // TODO: Figure out a general way to handle optional values.
    }

    auto UriMapper::add(std::string uri, Resolver resolver) -> Resolver
    {
        auto wildcard = this->containsWildcard(uri);

        if (wildcard == std::string::npos)
        {
            auto exactMatch = this->createExactMatch(uri);
            // Insert and return the resolver added.
            return (*this->resolvers.insert({exactMatch, resolver}).first).second;
        }

        auto wildcardRegex = this->createWithWildcard(uri);

        return (*this->resolvers.insert({wildcardRegex, resolver}).first).second;
    }

    auto UriMapper::match(std::string uri) -> std::optional<Resolver>
    {
        for (auto &[regexStr, resolver] : this->resolvers)
        {
            std::cmatch result;
            std::regex rgx(regexStr);

            if(std::regex_match(uri.data(), result, rgx))
            {
                return resolver;
            }
        }

        return std::nullopt;
    }
}