#pragma once
#include "IStream.hpp"

namespace lightning::stream
{
    class PlainStream : public IStream
    {
    public:
        PlainStream(int fd);

        auto read(int amount) -> std::vector<char> override;
        auto write(const char *buffer, int size) -> int override;
        auto readUntilToken(std::string token) -> std::vector<char> override;
        auto close() -> void override;

    private:
        int fd;
        std::vector<char> leftoverBuffer;

        auto readFromLeftover(int amount, int &bytesRead) -> std::vector<char>;
    };
}