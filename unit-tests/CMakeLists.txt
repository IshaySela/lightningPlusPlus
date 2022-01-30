cmake_minimum_required(VERSION 3.21.4)

project(httpTests)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)

FetchContent_Declare(
  lightning
  GIT_REPOSITORY https://github.com/IshaySela/lightningPlusPlus.git
  GIT_TAG main
)

FetchContent_MakeAvailable(googletest lightning)

enable_testing()

add_executable(httpTests
  source.cpp
)

target_link_libraries(
  httpTests
  gtest_main
  http)



include_directories(${http_SOURCE_DIR})

include(GoogleTest)

gtest_discover_tests(httpTests)