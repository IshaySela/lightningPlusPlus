#include "lightning/SSLClient.hpp"

namespace lightning
{
    SSLClient::SSLClient(int fd, SmartResource<SSL> smartSsl) : ssl(std::move(smartSsl)), rawFd(fd), sslStream(stream::SSLStream(this->ssl.get()))
    {
    }

    auto SSLClient::getSsl() const -> SSL *
    {
        return this->ssl.get();
    }

    auto SSLClient::getStream() -> stream::SSLStream &
    {
        return this->sslStream;
    }

} // namespace lightning
