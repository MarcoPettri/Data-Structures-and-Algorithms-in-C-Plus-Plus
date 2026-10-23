// File : P-8.4.cpp

/*
P-8.4 Implement the in-place heap-sort algorithm. Compare its running time
with that of the standard heap-sort that uses an external heap.
*/

#include "heap_sort_comparison.hpp"
#include <cassert>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

void runTests() {
  std::cout << "Running correctness tests...\n";

  // Test 1: In-place Heap Sort correctness
  {
    std::vector<int> vec = {12, 11, 13, 5, 6, 7, 11};
    inPlaceHeapSort(vec);
    assert(vec == (std::vector<int>{5, 6, 7, 11, 11, 12, 13}));
  }

  // Test 2: External Heap Sort correctness
  {
    std::vector<int> vec = {12, 11, 13, 5, 6, 7, 11};
    externalHeapSort(vec);
    assert(vec == (std::vector<int>{5, 6, 7, 11, 11, 12, 13}));
  }

  std::cout << "All correctness tests passed successfully!\n\n";
}

void runBenchmark() {
  std::cout << "Running performance benchmark (N = 100,000 integers)...\n";

  const size_t N = 100000;
  std::vector<int> original_data(N);

  std::mt19937 rng(1337);
  std::uniform_int_distribution<int> dist(1, 1000000);
  for (size_t i = 0; i < N; ++i) {
    original_data[i] = dist(rng);
  }

  // Benchmark In-Place Heap Sort
  {
    std::vector<int> data = original_data;
    auto start = std::chrono::high_resolution_clock::now();
    inPlaceHeapSort(data);
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> elapsed = end - start;
    std::cout << "In-Place Heap Sort Time:   " << elapsed.count() << " ms\n";
  }

  // Benchmark External Heap Sort
  {
    std::vector<int> data = original_data;
    auto start = std::chrono::high_resolution_clock::now();
    externalHeapSort(data);
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> elapsed = end - start;
    std::cout << "External Heap Sort Time:   " << elapsed.count() << " ms\n";
  }
}

int main() {
  runTests();
  runBenchmark();
  return 0;
}