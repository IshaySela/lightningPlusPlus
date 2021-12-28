#pragma once
#include <winsock2.h>

namespace lightning
{
    class WSAInitializer
    {
    public:
        LPWSADATA wsdata;
        WORD versionRequired;
        
        WSAInitializer();
        ~WSAInitializer();
    };

}