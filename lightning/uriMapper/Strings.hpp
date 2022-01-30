#pragma once
#include <string>
#include <optional>
#include <array>

/**
 * @brief Helper function for using strings with regex and more.
 */

namespace lightning::strings
{
    /**
     * @brief Replace occurences of {} with the corresponding replacements pass 
     * as variadic template.
     * If 1 replacement was provided and multiple <sepratorToken> where found, all occurrences of <sepratorToken>
     * will be replaced with the single replacer.
     * 
     * Exapmles:
     * format("Hello {}", "World") -> "Hello World"
     * format("Hello {} {}", "World") -> "Hello World World"
     * format("Hello {}","World", "break") -> ""
     * 
     * 
     * // TODO: Rephrase this comment and use more verbose language.
     * 
     * @tparam T The type of the array. This is used to ensure that at least 1 argument is passed, and to verify that all arguments are of the same type.
     * @tparam Ts A variadic parameter pack of all the replacers string to use.
     * @param str The string to format.
     * @param t The replacer. This is used to verify that at least 1 argument is passed, and ensure the type of args.
     * @param args The replacers list.
     * @param speratorToken The token to find in str and replace with args
     * 
     * @return std::optional<std::string> The formmated string.
     */
    template <typename T = std::string, typename... Ts>
    auto formatEx(const std::string &str, const std::string sepratorToken, T t, Ts... args) -> std::optional<std::string>
    {
        std::array<T, sizeof...(args) + 1> replacers = {t, args...};
        std::string replaced;
        size_t tokenIndex = std::string::npos;

        // Store the offset of the latest token.
        size_t offset = 0;

        for (int i = 0; i < replacers.size(); i++)
        {
            tokenIndex = str.find(sepratorToken, offset);

            if (tokenIndex == std::string::npos)
            {
                return {};
            }

            replaced += std::string(str.begin() + offset, str.begin() + tokenIndex) + replacers[i];

            offset = tokenIndex + sepratorToken.length();
        }

        replaced += std::string(str.begin() + offset, str.end());
        
        if(tokenIndex != std::string::npos && replaced.length() != 0)
            replaced = formatEx(replaced, sepratorToken, t, args...).value_or(replaced);

        return replaced;
    }
    template <typename T = std::string, typename... Ts>
    auto format(const std::string &str, T t, Ts... args)
    {
        static const std::string defaultSepratorToken = "{}";

        return strings::formatEx(str, defaultSepratorToken, t, args...);
    }


    /**
     * @brief Santize a string to be literal regex value, by appending all 
     * of the special characters [-.\\+*?\\[^\\]$(){}=!<>|:\\\\] with backslash.
     * 
     * @param str The string to sanitize
     * @return std::string The santizied string.
     */
    auto sanitizeForRegex(std::string &str) -> std::string;

    /**
     * @brief Sanitize all but the * for wildcard.
     * 
     * @return std::string The sanitized string.
     */
    auto sanitizeExecptWildcardForRegex(std::string& str) -> std::string;
}