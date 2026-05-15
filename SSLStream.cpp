#include "lightning/stream/SSLStream.hpp"
#include "lightning/LowLevelApiException.hpp"
#include "lightning/OpensslErrorQueueException.hpp"
#include <openssl/err.h>
#include <sys/socket.h>

namespace lightning::stream
{
    constexpr int SSLStream::SSL_NO_ERROR;

    SSLStream::SSLStream(SSL* ssl) : ssl(ssl) {}

    auto SSLStream::read(int amount) -> std::vector<char>
    {
        int bytesReadFromLeftover = 0;
        std::vector<char> buffer = readFromLeftover(amount, bytesReadFromLeftover);

        int totalReadBytes = bytesReadFromLeftover; // Store the amount that was read from the socket.
        int chunkRead = 0;                          // The amount of bytes that where read in the current call.
        int error = SSLStream::SSL_NO_ERROR;
        
        if (totalReadBytes >= amount)
        {
            return buffer;
        }

        do
        {
            totalReadBytes += chunkRead;

            char* chunk = buffer.data() + totalReadBytes;

            error = SSL_read_ex(this->ssl, chunk, amount - totalReadBytes, (size_t*)&chunkRead);

        }
        while (totalReadBytes < amount && error == SSLStream::SSL_NO_ERROR);
        
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
            const auto capturedErrno = errno;
            
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
        this->leftoverBuffer.insert(this->leftoverBuffer.end(), leftover.begin(), leftover.end());

        // Only the header
        std::vector<char> headersBuffer(buffer.begin(), buffer.begin() + static_cast<int>(tokenPosition));

        return headersBuffer;
    }

    auto SSLStream::write(const char* buffer, int size) -> int
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

    auto SSLStream::getSsl() const -> SSL*
    {
        return this->ssl;
    }

    auto SSLStream::handleReadError(int ret) -> void
    {
        const auto capturedErrno = errno;
        auto sslError = SSL_get_error(this->ssl, ret);

        switch (sslError)
        {
        case SSL_ERROR_SSL:
            /**
             * SSL_ERROR_SSL
             * A non-recoverable, fatal error in the SSL library occurred, usually a protocol error.  The OpenSSL error queue contains more information on the
             * error. If this error occurs then no further I/O operations should be performed on the connection and SSL_shutdown() must not be called.
             *
             */
            throw lightning::OpenSslErrorQueueException("SSL_ERROR_SSL", sslError, capturedErrno);
        case SSL_ERROR_WANT_READ:
            break;
        case SSL_ERROR_SYSCALL:
            /**
             * Some non-recoverable, fatal I/O error occurred. The OpenSSL error queue may contain more information on the error. For socket I/O on Unix
             * systems, consult errno for details. If this error occurs then no further I/O operations should be performed on the connection and SSL_shutdown()
             * must not be called.
             * This value can also be returned for other errors, check the error queue for details.
             */
            throw lightning::OpenSslErrorQueueException("SSL_ERROR_SYSCALL", sslError, capturedErrno);
        case SSL_ERROR_WANT_ACCEPT:
            break;
        case SSL_ERROR_WANT_ASYNC_JOB:
            break;
        default:
            break;
        }
    }

    auto SSLStream::readFromLeftover(int amount, int& bytesRead) -> std::vector<char>
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
        std::copy(this->leftoverBuffer.begin(), this->leftoverBuffer.begin() + amount, buffer.begin());
        // Delete the read data.
        this->leftoverBuffer.erase(this->leftoverBuffer.begin(), this->leftoverBuffer.begin() + amount);

        return buffer;
    }

    auto SSLStream::close() -> void
    {
        if (!SSL_is_init_finished(this->ssl))
            return;

        int ret = SSL_shutdown(this->ssl);

        if (ret == 0)
        {
            struct timeval tv { .tv_sec = 3, .tv_usec = 0 };
            setsockopt(SSL_get_fd(this->ssl), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

            char drain[256];
            while (SSL_read(this->ssl, drain, sizeof(drain)) > 0) {}
        }
        // The SSLStream class is not responsible for freeing
        // the ssl object. The entity that has provided the SSL* should own the object and free it.
        // SSL_free(this->ssl);
    }

}