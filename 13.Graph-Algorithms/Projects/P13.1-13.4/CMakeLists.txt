cmake_minimum_required(VERSION 3.14)
project(graph_adt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Header-only library
add_library(graph_adt INTERFACE)
target_include_directories(graph_adt INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# GoogleTest: use the installed package, or fetch it if it's missing
find_package(GTest QUIET)
if(NOT GTest_FOUND)
  include(FetchContent)
  FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip)
  FetchContent_MakeAvailable(googletest)
endif()

find_package(Threads REQUIRED)

add_executable(test_graphs test_graphs.cpp)
target_link_libraries(test_graphs PRIVATE graph_adt GTest::gtest_main Threads::Threads)
target_compile_options(test_graphs PRIVATE -Wall -Wextra -Wpedantic)

# Optional sanitizers: cmake -DSANITIZE=ON ..
option(SANITIZE "Build with ASan + UBSan" OFF)
if(SANITIZE)
  target_compile_options(test_graphs PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(test_graphs PRIVATE -fsanitize=address,undefined)
endif()

enable_testing()
add_test(NAME test_graphs COMMAND test_graphs)