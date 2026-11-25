cmake_minimum_required(VERSION 3.14)
project(memory_allocator_sim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Sanitizers apply to the test target only, so the experiment timings are not
# distorted. Turn off with -DENABLE_SANITIZERS=OFF.
option(ENABLE_SANITIZERS "Build allocator_test with ASan + UBSan" ON)

# Header-only library: memory_allocator.hpp includes memory_allocator.tpp.
add_library(memsim INTERFACE)
target_include_directories(memsim INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
if(NOT MSVC)
  target_compile_options(memsim INTERFACE -Wall -Wextra)
endif()

add_executable(fit_experiment fit_experiment_main.cpp)
target_link_libraries(fit_experiment PRIVATE memsim)

add_executable(allocator_test memory_allocator_test.cpp)
target_link_libraries(allocator_test PRIVATE memsim)
if(ENABLE_SANITIZERS AND NOT MSVC)
  target_compile_options(allocator_test PRIVATE
    -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(allocator_test PRIVATE -fsanitize=address,undefined)
endif()

enable_testing()
add_test(NAME allocator_test COMMAND allocator_test)