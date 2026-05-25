#include "lightning/PlainClient.hpp"

namespace lightning
{
    PlainClient::PlainClient(int fd) : fd(fd), plainStream(fd) {}

    auto PlainClient::getStream() -> stream::PlainStream&
    {
        return this->plainStream;
    }

    auto PlainClient::getFd() -> int
    {
        return this->fd;
    }
}