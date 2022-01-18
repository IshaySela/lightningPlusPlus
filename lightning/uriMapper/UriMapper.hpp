#pragma once
#include <unordered_map>
#include <regex>
#include "../httpServer/HttpServer.hpp"

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
        std::unordered_map<std::string, HttpServer::Resolver> resolvers;

        /**
         * @brief Find any special character that is relevant to the uri mapper, and return its index.
         * Return std::string::npos if no character was found.
         *
         * Currently, only looking for wildcard at the end of the uri.
         *
         * @param uri
         * @return size_t The index of the special character, or std::string::npos if none was found.
         */
        auto findSpecialCharacters(std::string uri) -> size_t;

        /**
         * @brief Create exact match regexp for the uri. 
         * "/test" -> "^\/test$"
         * "/hello/world" -> "^\/hello\/world$"
         * 
         * @param uri  
         * @return std::string The exact match regular expression.
         */
        auto createExactMatch(std::string uri) -> std::string;
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
         *
         * @param experssion The regex expression.
         * @param resolver The resolver.
         */
        auto add(std::string experssion, HttpServer::Resolver resolver) -> void;

        /**
         * @brief Return the value (resolver) of the first key (regex expression) that mathces to the uri provided.
         *
         * @param uri The uri to match against.
         * @return HttpServer::Resolver The resolver for which the uri was matched successfully.
         */
        auto match(std::string uri) -> HttpServer::Resolver;
    };
}