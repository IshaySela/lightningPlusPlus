#pragma once
#include <string>
#include <optional>
#include <array>

/**
 * @brief Helper function for using strings with regex and more.
 */

namespace lightning::strings
{
    template <typename T, typename... Ts>
    auto format(std::string &str, T t, Ts... args) -> std::optional<std::string>
    {
        static const auto buildToken = [](int value) -> std::string
        {
            return "{" + std::to_string(value) + "}";
        };

        std::array<T, sizeof...(args) + 1> replacers = {t, args...};
        std::string replaced;

        // Store the offset of the latest token.
        size_t offset = 0;

        for (int i = 0; i < replacers.size(); i++)
        {
            const std::string currentToken = buildToken(i);
            auto tokenIndex = str.find(currentToken);

            if (tokenIndex == std::string::npos)
            {
                return {};
            }

            replaced += std::string(str.begin() + offset, str.begin() + tokenIndex) + replacers[i];

            offset = tokenIndex + currentToken.length();
        }

        return replaced;
    }

}