#pragma once
#include <memory>
#include <openssl/ssl.h>
#include <functional>

namespace lightning
{
    template <typename T>
    using SmartResource = std::unique_ptr<T, std::function<void(T *)>>;

    namespace SmartSslResource
    {
        /**
        * @brief Delete the ssl object provided by calling SSL_shutdown and SSL_free.
        * 
        * @param ssl The ssl object to delete. 
        */
        auto sslObjectDeleter(SSL *ssl) -> void;

        /**
         * @brief Create new SmartResource<SSL> with sslObjectDeleter as the deleter.
         * 
         * @param ssl 
         * @return SmartResource<SSL> 
         */
        auto constructSmartSslResource(SSL *ssl) -> SmartResource<SSL>;
    }
}