#pragma once
#include "SmartResource.hpp"

auto initWSData() -> lightning::SmartResource<int>;

#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#define PLATFORM 'W'

#endif

#ifdef __unix__
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#define PLATFORM 'U'

/**
 * @brief A polyfill for the GetLastErrorMethod. return the errno instead.
 *
 * @return int
 */
auto inline GetLastError() -> int
{
    return errno;
}

#endif