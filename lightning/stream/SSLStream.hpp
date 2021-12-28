#pragma once
#include "IStream.hpp"
#include <openssl/ssl.h>

namespace lightning::stream
{
    /**
     * @brief Wrap the raw SSL_read and SSL_write functions for ease of use.
     */
    class SSLStream : public IStream
    {
    private:
        SSL *ssl;

        // The leftover data that was read from readUntilToken is stored
        // Here. SSLStream::read first reads from this vector before
        // reading from the socket.
        std::vector<char> leftoverBuffer;

        /**
         * @brief Handle a case where SSL_read fails.
         * 
         * @param ret The return value of SSL_read.
         */
        auto handleReadError(int ret) -> void;

        /**
         * @brief Read from the leftover vector, and remove the bytes read from it.
         * 
         * @param amount The amount to read from the vector.
         * @param bytesRead The amount of bytes that where read from the leftover vector.
         * @return std::vector<char> vector of chars that will at least be size of amount.
         */
        auto readFromLevtover(int amount, int &bytesRead) -> std::vector<char>;

    public:
        static constexpr int SSL_NO_ERROR = 1;

        SSLStream(SSL *ssl);

        auto read(int amount) -> std::vector<char> override;
        auto write(const char *buffer, int size) -> int override;
        auto readUntilToken(std::string token) -> std::vector<char> override;

        auto getSsl() const -> SSL *;
    };

}