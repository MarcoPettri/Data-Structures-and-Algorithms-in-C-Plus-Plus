// File : R-11.25.cpp

/*
    R-11.25 Describe an in-place version of the quick-select algorithm in pseudo-code
*/

#include "sorted_sequence_union.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

void run_tests() {
  std::cout << "[*] Running unit tests for sorted sequence union...\n";

  // Test 1: Basic sequences with duplicates
  {
    std::vector<int> A = {1, 1, 2, 3, 5, 5, 8};
    std::vector<int> B = {2, 3, 3, 4, 5, 9};
    auto result = dsa::compute_sorted_sequence_union(A.begin(), A.end(),
                                                     B.begin(), B.end());
    std::vector<int> expected = {1, 2, 3, 4, 5, 8, 9};
    assert(result == expected);
  }

  // Test 2: Disjoint sequences
  {
    std::vector<int> A = {1, 2, 3};
    std::vector<int> B = {4, 5, 6};
    auto result = dsa::compute_sorted_sequence_union(A.begin(), A.end(),
                                                     B.begin(), B.end());
    std::vector<int> expected = {1, 2, 3, 4, 5, 6};
    assert(result == expected);
  }

  // Test 3: Identical sequences
  {
    std::vector<int> A = {10, 20, 20, 30};
    std::vector<int> B = {10, 20, 20, 30};
    auto result = dsa::compute_sorted_sequence_union(A.begin(), A.end(),
                                                     B.begin(), B.end());
    std::vector<int> expected = {10, 20, 30};
    assert(result == expected);
  }

  // Test 4: One or both sequences empty
  {
    std::vector<int> A = {1, 2, 3};
    std::vector<int> B = {};
    auto result1 = dsa::compute_sorted_sequence_union(A.begin(), A.end(),
                                                      B.begin(), B.end());
    assert(result1 == (std::vector<int>{1, 2, 3}));

    auto result2 = dsa::compute_sorted_sequence_union(B.begin(), B.end(),
                                                      A.begin(), A.end());
    assert(result2 == (std::vector<int>{1, 2, 3}));

    std::vector<int> C = {};
    auto result3 = dsa::compute_sorted_sequence_union(B.begin(), B.end(),
                                                      C.begin(), C.end());
    assert(result3.empty());
  }

  // Test 5: All elements identical across both sequences
  {
    std::vector<int> A = {7, 7, 7};
    std::vector<int> B = {7, 7, 7, 7};
    auto result = dsa::compute_sorted_sequence_union(A.begin(), A.end(),
                                                     B.begin(), B.end());
    assert(result == (std::vector<int>{7}));
  }

  std::cout << "[+] All functional unit tests passed successfully!\n";
}

void run_stress_test() {
  std::cout << "[*] Running performance stress test (N = 1,000,000)...\n";

  constexpr size_t N = 1'000'000;
  std::vector<int> A(N);
  std::vector<int> B(N);

  // Populate with sorted random-like data containing duplicates
  std::mt19937 rng(1337);
  std::uniform_int_distribution<int> dist(1, 5'000'000);

  for (size_t i = 0; i < N; ++i) {
    A[i] = dist(rng);
    B[i] = dist(rng);
  }

  std::sort(A.begin(), A.end());
  std::sort(B.begin(), B.end());

  auto start = std::chrono::high_resolution_clock::now();
  auto result = dsa::compute_sorted_sequence_union(A.begin(), A.end(),
                                                   B.begin(), B.end());
  auto end = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double, std::milli> elapsed = end - start;
  std::cout << "[+] Stress test completed in " << elapsed.count() << " ms.\n";
  std::cout << "[+] Union size: " << result.size() << " unique elements.\n";

  // Verify sorted property and strict uniqueness of result
  for (size_t i = 1; i < result.size(); ++i) {
    assert(result[i - 1] < result[i]);
  }
  std::cout << "[+] Output correctness verified (sorted & strictly unique).\n";
}

int main() {
  run_tests();
  run_stress_test();
  std::cout << "[SUCCESS] All checks passed without errors.\n";
  return 0;
}