#pragma once
#include "IClient.hpp"
#include "stream/PlainStream.hpp"

namespace lightning
{
    class PlainClient : public IClient
    {
    public:
        PlainClient(int fd);
        auto getStream() -> stream::PlainStream& override;
        ~PlainClient() = default;

    private:
        int fd;
        stream::PlainStream plainStream;
    };
}