cmake_minimum_required(VERSION 3.21.4)

project(httpTests)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/release-1.11.0.zip
)

# FetchContent_Declare(
#   lightning
#   GIT_REPOSITORY https://github.com/IshaySela/lightningPlusPlus.git
#   GIT_TAG de7b9bede4e01ebf03e6d8f281c5d0e3bd4dcbf0
# )

FetchContent_MakeAvailable(googletest)

enable_testing()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../ ${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/http)

add_executable(httpTests
  source.cpp
  ./HttpServerTest.hpp
  ./mocks/MockStream.hpp
  ./mocks/MockClient.hpp
  ./mocks/MockLowLevelSocketServer.hpp
  ./mocks/Mocks.hpp
)

target_link_libraries(
  httpTests
  gmock_main
  http)



include_directories(${http_SOURCE_DIR})

include(GoogleTest)

gtest_discover_tests(httpTests)