#include "lightning/SmartResource.hpp"
#include <iostream>

namespace lightning
{
    auto SmartSslResource::sslObjectDeleter(SSL *ssl) -> void
    {
        std::cout << "deleting the ssl object" << ssl << std::endl;

        auto shutdownState = SSL_get_shutdown(ssl);
        
        // if shutdownState is SSL_SENT_SHUTDOWN or SSL_RECEIVED_SHUTDOWN, someone already has closed the connection.
        // TODO: Look at the openssl documentation and research the ideal way of handling the shutdown logic.
        if (shutdownState == 0)
        {
            SSL_shutdown(ssl);
            SSL_free(ssl);
        }
    }

    auto SmartSslResource::constructSmartSslResource(SSL *ssl) -> SmartResource<SSL>
    {
        return SmartResource<SSL>(ssl, sslObjectDeleter);
    }
}