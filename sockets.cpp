#include "lightning/sockets.hpp"

#if defined(_WIN32) || defined(_WIN64)

auto initWSData() -> lightning::SmartResource<int>
{
    // Platform specific code;
    static auto cleanWsa = [](int *)
    {
        WSACleanup();
    };

    WSADATA data;
    auto versionRequired = MAKEWORD(2, 2);
    int error;
    if ((error = WSAStartup(versionRequired, &data)) != 0)
    {
        throw lightning::LowLevelApiException("Error while calling WSAStartup()", error);
    }

    return lightning::SmartResource<int>(0, cleanWsa);
}
#endif


#if defined(__unix__)

auto initWSData() -> lightning::SmartResource<int>
{
    static const auto empty = [](int *) {};

    return lightning::SmartResource<int>(0, empty);
}
#endif