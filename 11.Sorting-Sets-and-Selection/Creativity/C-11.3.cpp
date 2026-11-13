// File : C-11.3.cpp

/*
    C-11.3 Given two sets A and B represented as sorted sequences, describe an efficient algorithm
    for computing A ⊕ B, which is the set of elements that are
    in A or B, but not in both.
*/

#include "symmetric_difference.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

void run_tests() {
  std::cout << "[*] Running unit tests for symmetric difference (A ⊕ B)...\n";

  // Test 1: Standard overlapping sets
  {
    std::vector<int> A = {1, 2, 4, 6, 8};
    std::vector<int> B = {2, 3, 4, 5, 9};
    auto result = dsa::compute_symmetric_difference(A.begin(), A.end(),
                                                    B.begin(), B.end());
    std::vector<int> expected = {1, 3, 5, 6, 8, 9};
    assert(result == expected);
  }

  // Test 2: Disjoint sets
  {
    std::vector<int> A = {1, 2, 3};
    std::vector<int> B = {4, 5, 6};
    auto result = dsa::compute_symmetric_difference(A.begin(), A.end(),
                                                    B.begin(), B.end());
    std::vector<int> expected = {1, 2, 3, 4, 5, 6};
    assert(result == expected);
  }

  // Test 3: Identical sets (symmetric difference should be empty)
  {
    std::vector<int> A = {10, 20, 30};
    std::vector<int> B = {10, 20, 30};
    auto result = dsa::compute_symmetric_difference(A.begin(), A.end(),
                                                    B.begin(), B.end());
    assert(result.empty());
  }

  // Test 4: One or both sets empty
  {
    std::vector<int> A = {1, 2, 3};
    std::vector<int> B = {};
    auto result1 = dsa::compute_symmetric_difference(A.begin(), A.end(),
                                                     B.begin(), B.end());
    assert(result1 == (std::vector<int>{1, 2, 3}));

    auto result2 = dsa::compute_symmetric_difference(B.begin(), B.end(),
                                                     A.begin(), A.end());
    assert(result2 == (std::vector<int>{1, 2, 3}));

    std::vector<int> C = {};
    auto result3 = dsa::compute_symmetric_difference(B.begin(), B.end(),
                                                     C.begin(), C.end());
    assert(result3.empty());
  }

  // Test 5: Proper subset relationship
  {
    std::vector<int> A = {2, 4};
    std::vector<int> B = {1, 2, 3, 4, 5};
    auto result = dsa::compute_symmetric_difference(A.begin(), A.end(),
                                                    B.begin(), B.end());
    std::vector<int> expected = {1, 3, 5};
    assert(result == expected);
  }

  std::cout << "[+] All functional unit tests passed successfully!\n";
}

void run_stress_test() {
  std::cout << "[*] Running performance stress test (N = 1,000,000)...\n";

  constexpr size_t N = 1'000'000;
  std::vector<int> A(N);
  std::vector<int> B(N);

  // Populate sorted data with partial overlap
  for (size_t i = 0; i < N; ++i) {
    A[i] = static_cast<int>(i * 2);       // Even numbers
    B[i] = static_cast<int>(i * 2 + 1);   // Odd numbers
  }
  // Introduce overlap every 10 elements
  for (size_t i = 0; i < N; i += 10) {
    B[i] = A[i];
  }

  auto start = std::chrono::high_resolution_clock::now();
  auto result =
      dsa::compute_symmetric_difference(A.begin(), A.end(), B.begin(), B.end());
  auto end = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double, std::milli> elapsed = end - start;
  std::cout << "[+] Stress test completed in " << elapsed.count() << " ms.\n";
  std::cout << "[+] Symmetric difference size: " << result.size()
            << " elements.\n";

  // Verify sorted property
  for (size_t i = 1; i < result.size(); ++i) {
    assert(result[i - 1] < result[i]);
  }
  std::cout << "[+] Output correctness verified (sorted & unique).\n";
}

int main() {
  run_tests();
  run_stress_test();
  std::cout << "[SUCCESS] All checks passed without errors.\n";
  return 0;
}