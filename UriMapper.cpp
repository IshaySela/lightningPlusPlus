#include "lightning/uriMapper/UriMapper.hpp"
#include "lightning/uriMapper/Strings.hpp"
#include <regex>
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
        static const std::string separator = "\\/*";

        auto cleanUri = strings::sanitizeExceptWildcardForRegex(uri);

        return strings::formatEx<std::string>(cleanUri, separator, wildcardRegex).value_or(uri); // TODO: Figure out a general way to handle optional values.
    }

    auto UriMapper::add(std::string uri, Resolver resolver) -> Resolver
    {
        auto wildcard = UriMapper::containsWildcard(uri);

        std::string regexStr = (wildcard == std::string::npos)
            ? UriMapper::createExactMatch(uri)
            : UriMapper::createWithWildcard(uri);

        std::regex compiled(regexStr);
        return this->resolvers.insert_or_assign(regexStr, std::make_pair(std::move(compiled), resolver))
            .first->second.second;
    }

    auto UriMapper::match(std::string uri) -> std::optional<std::pair<Resolver, std::string>>
    {
        for (auto& [regexStr, regexAndResolver] : this->resolvers)
        {
            auto& [rgx, resolver] = regexAndResolver;
            std::cmatch result;

            if (std::regex_match(uri.data(), result, rgx))
            {
                return std::pair<Resolver, std::string>(resolver, regexStr);
            }
        }

        return std::nullopt;
    }
}