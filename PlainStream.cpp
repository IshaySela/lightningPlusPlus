#include "lightning/stream/PlainStream.hpp"
#include "lightning/LowLevelApiException.hpp"
#include <sys/socket.h>
#include <unistd.h>

namespace lightning::stream
{
    PlainStream::PlainStream(int fd) : fd(fd) {}

    auto PlainStream::read(int amount) -> std::vector<char>
    {
        int bytesReadFromLeftover = 0;
        std::vector<char> buffer = readFromLeftover(amount, bytesReadFromLeftover);

        int totalReadBytes = bytesReadFromLeftover;

        while (totalReadBytes < amount)
        {
            int ret = ::recv(this->fd, buffer.data() + totalReadBytes, amount - totalReadBytes, 0);
            if (ret < 0)
                throw lightning::LowLevelApiException("recv() failed", errno, errno);
            if (ret == 0)
                break;
            totalReadBytes += ret;
        }

        return buffer;
    }

    auto PlainStream::write(const char *buffer, int size) -> int
    {
        int ret = ::send(this->fd, buffer, size, 0);
        if (ret < 0)
            throw lightning::LowLevelApiException("send() failed", errno, errno);
        return ret;
    }

    auto PlainStream::readUntilToken(std::string token) -> std::vector<char>
    {
        std::string buffer;
        buffer.reserve(1024);
        std::string chunk(1024, '\0');

        bool foundToken = false;
        size_t tokenPosition = std::string::npos;

        while (!foundToken)
        {
            int ret = ::recv(this->fd, chunk.data(), chunk.size(), 0);
            if (ret < 0)
                throw lightning::LowLevelApiException("recv() failed", errno, errno);
            if (ret == 0)
                break;

            buffer.append(chunk.data(), ret);
            tokenPosition = buffer.find(token);
            if (tokenPosition != std::string::npos)
                foundToken = true;
        }

        if (tokenPosition != std::string::npos)
        {
            std::string leftover(buffer.begin() + tokenPosition + token.length(), buffer.end());
            this->leftoverBuffer.insert(this->leftoverBuffer.end(), leftover.begin(), leftover.end());
        }

        size_t headerEnd = tokenPosition != std::string::npos ? tokenPosition : buffer.size();
        return std::vector<char>(buffer.begin(), buffer.begin() + headerEnd);
    }

    auto PlainStream::close() -> void
    {
        ::shutdown(this->fd, SHUT_RDWR);
        ::close(this->fd);
    }

    auto PlainStream::setTimeout(int seconds) -> void
    {
        struct timeval tv { .tv_sec = seconds, .tv_usec = 0 };
        setsockopt(this->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }

    auto PlainStream::readFromLeftover(int amount, int &bytesRead) -> std::vector<char>
    {
        std::vector<char> buffer(amount);
        bytesRead = 0;

        if (this->leftoverBuffer.empty())
            return buffer;

        int toCopy = amount > (int)this->leftoverBuffer.size() ? this->leftoverBuffer.size() : amount;
        bytesRead = toCopy;
        std::copy(this->leftoverBuffer.begin(), this->leftoverBuffer.begin() + toCopy, buffer.begin());
        this->leftoverBuffer.erase(this->leftoverBuffer.begin(), this->leftoverBuffer.begin() + toCopy);

        return buffer;
    }
}