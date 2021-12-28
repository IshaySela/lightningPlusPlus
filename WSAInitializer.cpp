#include "lightning/WSAInitializer.hpp"
#include <iostream>

namespace lightning
{
    WSAInitializer::WSAInitializer() : versionRequired(MAKEWORD(2, 2))
    {
        int error;
        if ((error = WSAStartup(versionRequired, this->wsdata) < 0))
        {
            std::cout << error << std::endl;
        }
    }

    WSAInitializer::~WSAInitializer()
    {
        WSACleanup();
    }
}