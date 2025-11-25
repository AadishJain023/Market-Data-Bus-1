cmake_minimum_required(VERSION 3.16)
project(md-bus VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TESTS "Build tests" ON)

# Fetch fmt library
include(FetchContent)
FetchContent_Declare(
  fmt
  URL https://github.com/fmtlib/fmt/archive/refs/tags/10.1.1.zip
)
FetchContent_MakeAvailable(fmt)

add_subdirectory(engine)

if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
