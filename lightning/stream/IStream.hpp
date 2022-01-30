#pragma once

#include <vector>
#include <string>

namespace lightning::stream
{
    class IStream
    {
    public:
        /**
         * @brief Read X amount of data from the source.
         * 
         * @param amount The amount to read.
         * @return std::vector<char> The data that was read.
         */
        virtual auto read(int amount) -> std::vector<char> = 0;

        /**
         * @brief Write the buffer to the source.
         * 
         * @param buffer The buffer to write.
         * @param size The size of the buffer
         * @return int The amount that was written.
         */
        virtual auto write(const char *buffer, int size) -> int = 0;

        /**
         * @brief Read from the source until reached the given token. What to do with any leftover is up to the implementation.
         * Note that the token should be left out from the buffer.
         * 
         * @param token The string to find.
         * @return std::vector<char> The buffer read from the source without the token.
         */
        virtual auto readUntilToken(std::string token) -> std::vector<char> = 0;

        /**
         * @brief Close the connection.The behaviour of a call to IStream::read and IStream::write
         * is not defined.
         */
        virtual auto close() -> void = 0;
    };

} // namespace lightning::stream
