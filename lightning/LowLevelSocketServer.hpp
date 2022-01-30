#pragma once

namespace lightning
{
    template <typename TClient>
    class LowLevelSocketServer
    {
    public:
        /**
         * @brief Wait for new client and return it.
         *
         * @return TClient The accepted client.
         */
        virtual auto accept() -> TClient = 0;
        virtual ~LowLevelSocketServer() = 0;
    };
} // namespace lightning
