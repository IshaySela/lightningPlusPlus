#include "lightning/stream/SSLStream.hpp"
#include "lightning/LowLevelApiException.hpp"

namespace lightning::stream
{
    constexpr int SSLStream::SSL_NO_ERROR;

    SSLStream::SSLStream(SSL *ssl) : ssl(ssl) {}

    auto SSLStream::read(int amount) -> std::vector<char>
    {
        int bytesReadFromLeftover = 0;
        std::vector<char> buffer = readFromLevtover(amount, bytesReadFromLeftover);

        int totalReadBytes = bytesReadFromLeftover; // Store the amount that was read from the socket.
        int chunkRead = 0;                          // The amount of bytes that where read in the current call.
        int error = SSLStream::SSL_NO_ERROR;

        do
        {
            totalReadBytes += chunkRead;

            char *chunk = buffer.data() + totalReadBytes;

            error = SSL_read_ex(this->ssl, chunk, amount - totalReadBytes, (size_t *)&chunkRead);

        } while (totalReadBytes < amount && error == SSLStream::SSL_NO_ERROR);

        if (error != SSLStream::SSL_NO_ERROR)
        {
            auto errorCode = SSL_get_error(ssl, error);
            this->handleReadError(error);
        }

        return buffer;
    }

    auto SSLStream::readUntilToken(std::string token) -> std::vector<char>
    {
        std::string buffer, chunk;
        buffer.reserve(1024);
        chunk.resize(1024);

        int readResult = SSLStream::SSL_NO_ERROR;
        bool foundToken = false;
        size_t tokenPosition = std::string::npos, chunkBytesRead = 0;

        while (readResult == SSLStream::SSL_NO_ERROR && !foundToken)
        {
            readResult = SSL_read_ex(this->ssl, reinterpret_cast<void*>(chunk.data()), 1024, &chunkBytesRead);

            if (readResult < SSLStream::SSL_NO_ERROR)
            {
                this->handleReadError(readResult);
            }

            tokenPosition = chunk.find(token);

            if (tokenPosition != std::string::npos)
            {
                foundToken = true;
            }

            buffer.append(chunk);
        }

        // Get the leftover body that was read with the headers.
        std::string leftover(buffer.begin() + static_cast<int>(tokenPosition) + token.length(), buffer.end());

        // Only the header
        std::vector<char> headersBuffer(buffer.begin(), buffer.begin() + static_cast<int>(tokenPosition));

        return headersBuffer;
    }

    auto SSLStream::write(const char *buffer, int size) -> int
    {
        size_t written = 0;

        int error = SSL_write_ex(this->ssl, buffer, size, &written);

        if (error != 1)
        {
            auto errorCode = SSL_get_error(ssl, error);
            throw lightning::LowLevelApiException("Error while writing to SSL sink", errorCode);
        }

        return (int)written;
    }

    auto SSLStream::getSsl() const -> SSL *
    {
        return this->ssl;
    }

    auto SSLStream::handleReadError(int ret) -> void
    {
        auto error = SSL_get_error(this->ssl, ret);

        switch (error)
        {
        case SSL_ERROR_ZERO_RETURN:
        case SSL_ERROR_SYSCALL:
        case SSL_ERROR_SSL:
            throw lightning::LowLevelApiException("A non-recoverable, fatal error has occurred while reading data from the peer", error);
        default:
            break;
        }
    }

    auto SSLStream::readFromLevtover(int amount, int &bytesRead) -> std::vector<char>
    {
        std::vector<char> buffer(amount);
        bytesRead = 0;

        if (this->leftoverBuffer.size() == 0)
        {
            return buffer;
        }

        // Cap the amount to read to be the length of leftoverBuffer.
        amount = amount > this->leftoverBuffer.size() ? this->leftoverBuffer.size() : amount;

        bytesRead = amount;

        // Delete the read data.
        this->leftoverBuffer.resize(this->leftoverBuffer.size() - amount);

        return buffer;
    }

    auto SSLStream::close() -> void
    {
        SSL_shutdown(this->ssl);
        SSL_free(this->ssl);
    }

}