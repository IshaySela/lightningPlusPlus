#pragma once
#include <openssl/ssl.h>
#include <memory>
#include <functional>
#include "SmartResource.hpp"
#include "stream/SSLStream.hpp"
#include "IClient.hpp"

namespace lightning
{
    class SSLClient : public IClient
    {
    public:
        /**
         * @brief Construct a new SSLClient object. 
         * 
         * @param fd The raw socket fd.
         * @param ssl The ssl object.
         */
        SSLClient(int fd, SmartResource<SSL> ssl);
        auto getSsl() const -> SSL *;
        auto getStream() -> stream::SSLStream& override;

        ~SSLClient() = default;
    private:
        SmartResource<SSL> ssl;
        stream::SSLStream sslStream;

        int rawFd; //< TODO: Change to a custom stream object.
    };

}