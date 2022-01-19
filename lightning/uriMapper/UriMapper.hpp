#pragma once
#include <unordered_map>
#include <regex>
#include <optional>
#include "../lightning.hpp"

namespace lightning
{
    /**
     * @brief Map uri to a resolver, support for regex and advanced matching
     * Examples:
     * For a uri mapper that contains the following keys - { '/test', '*', '/test/*' }
     */
    class UriMapper
    {
    private:
        std::unordered_map<std::string, Resolver> resolvers;

        /**
         * @brief Search for the wildcard string /* and return its index.
         * Return string::npos if the string does not exists.
         * 
         * @param uri The uri to search in.
         * @return size_t The index of the wildcard, or string::npos if none was found.
         */
        auto containsWildcard(std::string uri) -> size_t;

        /**
         * @brief Create exact match regexp for the uri. 
         * "/test" -> "^\/test$"
         * "/hello/world" -> "^\/hello\/world$"
         * 
         * @param uri  
         * @return std::string The exact match regular expression.
         */
        auto createExactMatch(std::string uri) -> std::string;

        /**
         * @brief Convert a uri with wildcard /* to the regex equivelent.
         * 
         * /test/* -> ^\/test(\/.*|$) = /test or /test/<any character set possible>
         * /test/* /hello -> ^\/test(\/.*|$)\/hello = /test/<any character set possible>/hello
         * 
         * @param uri The uri with the wildcard that is used to construct the regex.
         * @return std::string The constructed regex.
         */
        auto createWithWildcard(std::string uri) -> std::string;

    public:
        /**
         * @brief Add a new resolver to the resolvers map.
         * If no special characters are detect (wildcard *, id [] etc), an exact match will occur.
         * "/test" -> "^\/test$"
         *
         * If a wildcard is detected at the end of the uri, the the $ will be dropped,
         * and the uri iteself will be valid:
         *
         * "/test/*" -> "^\/test\/(.*)|^\/test" ; The string /test/ and then any string, or the string '/test/ iteself.
         *
         * Note: The order of UriMapper::add is important. If 2 expressions
         * will result in a match, the latter experssion will be resolved since it will be stored before it in the 
         * unordered_map.
         * 
         * @param experssion The regex expression.
         * @param resolver The resolver.
         * @returns The resolver that was passed in.
         */
        auto add(std::string experssion, Resolver resolver) -> Resolver;

        /**
         * @brief Return the value (resolver) of the first key (regex expression) that mathces to the uri provided.
         *
         * @param uri The uri to match against.
         * @return Resolver The resolver for which the uri was matched successfully.
         */
        auto match(std::string uri) -> std::optional<Resolver>;
    };
}