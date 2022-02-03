#pragma once
#include "stream/IStream.hpp"
#include <iostream>

namespace lightning
{
    class IClient
    {
    public:
        virtual auto getStream() -> stream::IStream& = 0;

        virtual ~IClient() = default;
    };
}