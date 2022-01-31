#pragma once
#include <gmock/gmock.h>
#include <lightning/LowLevelSocketServer.hpp>

namespace mocks
{
    class MockLowLevelSocketServer : public lightning::ILowLevelSocketServer
    {
    public:
        MOCK_METHOD(std::unique_ptr<lightning::IClient>, accept, (), (override));
    };

}
