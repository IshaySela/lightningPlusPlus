#pragma once

#include <string>
#include <string_view>
#include <vector>

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

        /**
         * @brief Set a receive timeout on the underlying socket.
         *
         * @param seconds Number of seconds before a read times out (0 disables the timeout).
         */
        virtual auto setTimeout(int seconds) -> void = 0;

        /**
         * @brief The function injectBuffer allows the caller to inject data into the stream's internal buffer. 
         * this is useful for cases where the caller has read some data from the stream that it doesn't want to process immediately, but also doesn't want to lose. By injecting the data back into the stream, the caller can ensure that it will be available for future read operations. The exact behavior of how the injected data is stored and accessed is up to the implementation of the IStream interface.
         * @param data the data to inject
         */
        virtual auto injectBuffer(std::vector<char> data) -> void {}

        /**
         * @brief The function peek allows the caller to look at a certain amount of data from the stream without consuming it.
         * The function is virtual to avoid copying data for implementations that can provide a view into the internal buffer.
         * @note There is no guarantee that the view returned by peek will remain valid after subsequent calls to read, write, or injectBuffer,
         * as the internal buffer may be reallocated or modified.
         * @param amount The amount to read
         * @return std::string_view View into the internal buffer (data pointer + size of actually buffered bytes).
         */
        virtual auto peek(int amount) -> std::string_view = 0;
        virtual ~IStream() = default;
    };

} // namespace lightning::stream
