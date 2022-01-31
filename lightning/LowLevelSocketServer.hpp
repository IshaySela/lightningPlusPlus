#pragma once
#include "IClient.hpp"
#include <memory>

namespace lightning
{
    class ILowLevelSocketServer
    {
    public:
        /**
         * @brief Wait for new client and return it.
         *
         * @return TClient The accepted client.
         */
        virtual auto accept() -> std::unique_ptr<IClient> = 0;
    };
} // namespace lightning
