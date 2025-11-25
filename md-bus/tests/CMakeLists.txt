cmake_minimum_required(VERSION 3.16)

include(FetchContent)

# Fetch and build GoogleTest
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
  SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/googletest-src
  BINARY_DIR ${CMAKE_BINARY_DIR}/_deps/googletest-build
)

# Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Make GoogleTest available
FetchContent_MakeAvailable(googletest)

# Create test executable
add_executable(test_bus test_bus.cpp)

# Link against md-bus-engine library and GoogleTest
target_link_libraries(test_bus PRIVATE md-bus-engine gtest_main gtest)

# Enable testing and register test
enable_testing()
add_test(NAME BusTests COMMAND test_bus)
