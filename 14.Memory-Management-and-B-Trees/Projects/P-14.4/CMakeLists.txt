cmake_minimum_required(VERSION 3.16)
project(external_merge_sort LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(EXTSORT_SANITIZE "Build the tests with ASan + UBSan" ON)

# Header-only library (the .hpp includes the .tpp at the bottom).
add_library(external_merge_sort INTERFACE)
target_include_directories(external_merge_sort INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(external_merge_sort INTERFACE cxx_std_17)

set(EXTSORT_WARNINGS -Wall -Wextra -Wpedantic)

# Unit, stress and performance-sanity tests.
add_executable(test_external_merge_sort test_external_merge_sort.cpp)
target_link_libraries(test_external_merge_sort PRIVATE external_merge_sort)
target_compile_options(test_external_merge_sort PRIVATE ${EXTSORT_WARNINGS})
if(EXTSORT_SANITIZE)
  target_compile_options(test_external_merge_sort PRIVATE
    -g -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(test_external_merge_sort PRIVATE
    -fsanitize=address,undefined)
endif()

# Benchmark against std::sort (always built without sanitizers).
add_executable(benchmark_external_sort benchmark_external_sort.cpp)
target_link_libraries(benchmark_external_sort PRIVATE external_merge_sort)
target_compile_options(benchmark_external_sort PRIVATE ${EXTSORT_WARNINGS})

enable_testing()
add_test(NAME external_merge_sort_tests COMMAND test_external_merge_sort)
# Sanitizers slow the run down, so skip the wall-clock assertion under ctest.
set_tests_properties(external_merge_sort_tests PROPERTIES
  ENVIRONMENT "EXTSORT_SKIP_TIMING=1")
