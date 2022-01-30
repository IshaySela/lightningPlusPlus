#pragma once
#include "stream/IStream.hpp"


namespace lightning
{
    class IClient
    {
    public:
        virtual auto getStream() -> stream::IStream& = 0;
    };
}