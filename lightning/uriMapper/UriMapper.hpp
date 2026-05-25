#pragma once
#include <unordered_map>
#include <optional>
#include <regex>
#include "../lightning.hpp"

namespace lightning
{
    /**
     * @brief Map uri to a resolver, support for regex and advanced matching
     * Examples:
     * For a uri mapper that contains the following keys - { '/test', '*', '/test / *' }
     */
    class UriMapper
    {
    private:
        std::unordered_map<std::string, std::pair<std::regex, Resolver>> resolvers;

        /**
         * @brief Search for the wildcard string / * and return its index.
         * Return string::npos if the string does not exist.
         * 
         * @param uri The uri to search in.
         * @return size_t The index of the wildcard, or string::npos if none was found.
         */
        static auto containsWildcard(std::string uri) -> size_t;

    public:
        /**
         * @brief Create exact match regexp for the uri. 
         * "/test" -> "^\/test$"
         * "/hello/world" -> "^\/hello\/world$"
         * 
         * @param uri  
         * @return std::string The exact match regular expression.
         */
        static auto createExactMatch(std::string uri) -> std::string;

        /**
         * @brief Convert a uri with wildcard / * to the regex equivalent.
         * 
         * /test/ * -> ^\/test(\/.*|$) = /test or /test/<any character set possible>
         * /test/ * /hello -> ^\/test(\/.*|$)\/hello = /test/<any character set possible>/hello
         * 
         * @param uri The uri with the wildcard that is used to construct the regex.
         * @return std::string The constructed regex.
         */
        static auto createWithWildcard(std::string uri) -> std::string;

        /**
         * @brief Add a new resolver to the resolvers map.
         * If no special characters are detected (wildcard *, id [] etc), an exact match will occur.
         * "/test" -> "^\/test$"
         *
         * If a wildcard is detected at the end of the uri, the $ will be dropped,
         * and the uri itself will be valid:
         *
         * "/test/ *" -> "^\/test\/(.*)|^\/test" ; The string /test/ and then any string, or the string '/test/ itself.
         *
         * Note: The order of UriMapper::add is important. If 2 expressions
         * will result in a match, the latter expression will be resolved since it will be stored before it in the 
         * unordered_map.
         * 
         * @param expression The regex expression.
         * @param resolver The resolver.
         * @returns The resolver that was passed in.
         */
        auto add(std::string expression, Resolver resolver) -> Resolver;

        /**
         * @brief Return the value (resolver) of the first key (regex expression) that matches the uri provided, and the
         * regex that was matched.
         *
         * @param uri The uri to match against.
         * @return Optional pair containing the resolver and the regex that was matched.
         */
        auto match(std::string uri) -> std::optional<std::pair<Resolver, std::string>>;
    };
}