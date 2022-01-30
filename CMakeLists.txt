cmake_minimum_required(VERSION 3.21.4)

project(http)

set(CMAKE_CXX_STANDARD 20)


add_library(http STATIC
    ./source.cpp
    ./lightning/sockets.hpp ./sockets.cpp
    ./lightning/SSLServer.hpp ./SSLServer.cpp
    ./lightning/LowLevelApiException.hpp ./LowLevelApiException.cpp
    ./lightning/SmartResource.hpp ./SmartResource.cpp
    
    ./lightning/SSLClient.hpp ./SSLClient.cpp
    ./lightning/stream/IStream.hpp
    ./lightning/stream/SSLStream.hpp ./SSLStream.cpp

    ./lightning/request/HttpRequest.hpp ./HttpRequest.cpp
    ./lightning/request/FrameworkInfo.hpp
    ./lightning/response/HttpResponse.hpp ./HttpResponse.cpp
    ./lightning/response/StatusLine.hpp
    ./lightning/response/HttpResponseBuilder.hpp ./HttpResponseBuilder.cpp
    ./lightning/HttpProtocol.hpp
    ./lightning/httpServer/HttpServer.hpp ./HttpServer.cpp

    ./lightning/uriMapper/UriMapper.hpp ./UriMapper.cpp
    ./lightning/uriMapper/Strings.hpp ./Strings.cpp

    ./lightning/TaskExecutor.hpp

    ./lightning/handlers/StaticFile.hpp ./StaticFile.cpp
)

find_package(OpenSSL REQUIRED)
target_link_libraries(http OpenSSL::SSL)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})


if(WIN32)
  target_link_libraries(http wsock32 ws2_32)
endif()
