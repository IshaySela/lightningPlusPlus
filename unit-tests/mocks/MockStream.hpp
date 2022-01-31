#pragma once
#include <lightning/stream/IStream.hpp>
#include <gmock/gmock.h>
#include <vector>

namespace mocks
{
    class MockStream : public lightning::stream::IStream
    {
    public:
        MOCK_METHOD(std::vector<char>, read, (int amount), (override));
        MOCK_METHOD(int, write, (const char *buffer, int size), (override));
        MOCK_METHOD(std::vector<char>, readUntilToken, (std::string token), (override));
        MOCK_METHOD(void, close, (), (override));
    };
}
