#include "lightning/SmartResource.hpp"
#include <iostream>

namespace lightning
{
    auto SmartSslResource::sslObjectDeleter(SSL *ssl) -> void
    {
        std::cout << "deleting the ssl object" << ssl << std::endl;
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }

    auto SmartSslResource::constructSmartSslResource(SSL* ssl) -> SmartResource<SSL> 
    {
        return SmartResource<SSL>(ssl, sslObjectDeleter);
    }
}