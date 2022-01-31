#pragma once
#include <lightning/IClient.hpp>
#include <lightning/stream/IStream.hpp>
#include <gmock/gmock.h>

namespace mocks
{
    class MockClient : public lightning::IClient
    {
    public:
        MOCK_METHOD(lightning::stream::IStream &, getStream, (), (override));
    };

}