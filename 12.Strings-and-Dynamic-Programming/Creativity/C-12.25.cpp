// File : C-12.25.cpp

/*
    C-12.25 Let three integer arrays, A, B, and C, be given, each of size n. Given an
    arbitrary integer x, design an O(n^2 log n)-time algorithm to determine if
    there exist numbers, a in A, b in B, and c in C, such that x = a + b + c.
*/

#include "three_sum_arrays.hpp"

#include <cassert>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

namespace {

/**
 * @brief Independent brute-force O(n^3) oracle.
 */
template <typename T>
bool brute_force_oracle(const std::vector<T> &A, const std::vector<T> &B,
                        const std::vector<T> &C, T x) {
  for (const auto &a : A) {
    for (const auto &b : B) {
      for (const auto &c : C) {
        if (a + b + c == x) {
          return true;
        }
      }
    }
  }
  return false;
}

void test_edge_cases() {
  std::cout << "[*] Running edge case tests...\n";

  // Edge Case 1: Empty arrays
  {
    std::vector<int> empty;
    auto res = dsa::three_sum_arrays(empty, empty, empty, 10);
    assert(!res);
    assert(!res.found);
  }

  // Edge Case 2: Single element match
  {
    std::vector<int> A = {2};
    std::vector<int> B = {3};
    std::vector<int> C = {5};
    auto res = dsa::three_sum_arrays(A, B, C, 10);
    assert(res);
    assert(res.a == 2 && res.b == 3 && res.c == 5);
    assert(res.index_a == 0 && res.index_b == 0 && res.index_c == 0);
    assert(res.verify(10));
  }

  // Edge Case 3: Single element mismatch
  {
    std::vector<int> A = {2};
    std::vector<int> B = {3};
    std::vector<int> C = {5};
    auto res = dsa::three_sum_arrays(A, B, C, 11);
    assert(!res);
  }

  // Edge Case 4: Target requiring negative numbers
  {
    std::vector<int> A = {-5, 10};
    std::vector<int> B = {3, -8};
    std::vector<int> C = {-2, 7};
    // -5 + -8 + -2 = -15
    auto res = dsa::three_sum_arrays(A, B, C, -15);
    assert(res);
    assert(res.verify(-15));
    assert(A[res.index_a] == res.a);
    assert(B[res.index_b] == res.b);
    assert(C[res.index_c] == res.c);
  }

  // Edge Case 5: All zeros with zero target
  {
    std::vector<int> A = {0, 0};
    std::vector<int> B = {0, 0};
    std::vector<int> C = {0, 0};
    auto res = dsa::three_sum_arrays(A, B, C, 0);
    assert(res);
    assert(res.verify(0));
  }

  std::cout << "    [+] Edge cases passed.\n";
}

void test_functional_and_analytical_cases() {
  std::cout << "[*] Running functional and algorithmic comparison tests...\n";

  // Test 1: Standard positive integers
  {
    std::vector<int> A = {10, 20, 30};
    std::vector<int> B = {1, 2, 3};
    std::vector<int> C = {100, 200, 300};
    const int target = 20 + 3 + 100; // 123

    auto res_bs = dsa::ThreeSumSolver::solve_binary_search(A, B, C, target);
    assert(res_bs);
    assert(res_bs.verify(target));
    assert(A[res_bs.index_a] == res_bs.a);
    assert(B[res_bs.index_b] == res_bs.b);
    assert(C[res_bs.index_c] == res_bs.c);

    auto res_tp = dsa::ThreeSumSolver::solve_two_pointers(A, B, C, target);
    assert(res_tp);
    assert(res_tp.verify(target));
  }

  // Test 2: Unachievable target sum
  {
    std::vector<int> A = {1, 2};
    std::vector<int> B = {10, 20};
    std::vector<int> C = {100, 200};
    assert(!dsa::ThreeSumSolver::solve_binary_search(A, B, C, 999));
    assert(!dsa::ThreeSumSolver::solve_two_pointers(A, B, C, 999));
  }

  // Test 3: Multiple valid triplets using find_all_triplets
  {
    std::vector<int> A = {1, 2};
    std::vector<int> B = {3, 2};
    std::vector<int> C = {6, 6};
    // Sum 10:
    // (1, 3, 6) with two C choices -> 2 triplets
    // (2, 2, 6) with two C choices -> 2 triplets
    // Total = 4 valid triplet combinations
    auto all = dsa::ThreeSumSolver::find_all_triplets(A, B, C, 10);
    assert(all.size() == 4);
    for (const auto &trip : all) {
      assert(trip.verify(10));
      assert(A[trip.index_a] == trip.a);
      assert(B[trip.index_b] == trip.b);
      assert(C[trip.index_c] == trip.c);
    }
  }

  std::cout << "    [+] Functional and algorithmic comparison tests passed.\n";
}

void test_oracle_brute_force_equivalence() {
  std::cout << "[*] Verifying against brute-force O(n^3) oracle across 200 "
               "randomized instances...\n";

  std::mt19937_64 rng(1337);
  std::uniform_int_distribution<int> val_dist(-50, 50);
  std::uniform_int_distribution<std::size_t> len_dist(1, 25);

  for (int iter = 0; iter < 200; ++iter) {
    const std::size_t n = len_dist(rng);

    std::vector<int> A(n);
    std::vector<int> B(n);
    std::vector<int> C(n);

    for (auto &x : A)
      x = val_dist(rng);
    for (auto &x : B)
      x = val_dist(rng);
    for (auto &x : C)
      x = val_dist(rng);

    // Pick target either as guaranteed sum or random
    int target = 0;
    if (iter % 2 == 0) {
      std::uniform_int_distribution<std::size_t> idx_dist(0, n - 1);
      target = A[idx_dist(rng)] + B[idx_dist(rng)] + C[idx_dist(rng)];
    } else {
      target = val_dist(rng) * 3;
    }

    const bool oracle_found = brute_force_oracle(A, B, C, target);
    auto res_bs = dsa::ThreeSumSolver::solve_binary_search(A, B, C, target);
    auto res_tp = dsa::ThreeSumSolver::solve_two_pointers(A, B, C, target);

    assert(res_bs.found == oracle_found);
    assert(res_tp.found == oracle_found);

    if (oracle_found) {
      assert(res_bs.verify(target));
      assert(A[res_bs.index_a] == res_bs.a);
      assert(B[res_bs.index_b] == res_bs.b);
      assert(C[res_bs.index_c] == res_bs.c);

      assert(res_tp.verify(target));
      assert(A[res_tp.index_a] == res_tp.a);
      assert(B[res_tp.index_b] == res_tp.b);
      assert(C[res_tp.index_c] == res_tp.c);
    }
  }

  std::cout << "    [+] Brute-force equivalence verified across 200 randomized "
               "test cases.\n";
}

void test_stress_and_performance() {
  std::cout << "[*] Running stress and performance benchmark (n = 1000)...\n";

  const std::size_t n = 1000;
  std::mt19937_64 rng(42);
  std::uniform_int_distribution<int> val_dist(-100'000, 100'000);

  std::vector<int> A(n);
  std::vector<int> B(n);
  std::vector<int> C(n);

  for (auto &x : A)
    x = val_dist(rng);
  for (auto &x : B)
    x = val_dist(rng);
  for (auto &x : C)
    x = val_dist(rng);

  // Guaranteed target placed near the end
  const int target = A[n - 1] + B[n - 1] + C[n - 1];

  // Benchmark O(n^2 log n) Binary Search Method (as requested in C-12.25)
  const auto start_bs = std::chrono::high_resolution_clock::now();
  auto res_bs = dsa::ThreeSumSolver::solve_binary_search(A, B, C, target);
  const auto end_bs = std::chrono::high_resolution_clock::now();

  const auto duration_bs_ms =
      std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
          end_bs - start_bs)
          .count();

  assert(res_bs.found);
  assert(res_bs.verify(target));

  // Benchmark O(n^2) Two-Pointer Method
  const auto start_tp = std::chrono::high_resolution_clock::now();
  auto res_tp = dsa::ThreeSumSolver::solve_two_pointers(A, B, C, target);
  const auto end_tp = std::chrono::high_resolution_clock::now();

  const auto duration_tp_ms =
      std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
          end_tp - start_tp)
          .count();

  assert(res_tp.found);
  assert(res_tp.verify(target));

  std::cout << "    -> N = " << n << " (1M pairs checked)\n"
            << "       O(n^2 log n) Binary Search : " << std::fixed
            << std::setprecision(2) << duration_bs_ms << " ms\n"
            << "       O(n^2) Two Pointers        : " << std::fixed
            << std::setprecision(2) << duration_tp_ms << " ms\n";

  std::cout << "    [+] Stress and benchmark tests completed successfully.\n";
}

} // namespace

int main() {
  std::cout << "========================================================\n";
  std::cout << "      C-12.25: 3-Sum Across Three Arrays in O(n^2 log n) \n";
  std::cout << "========================================================\n\n";

  test_edge_cases();
  test_functional_and_analytical_cases();
  test_oracle_brute_force_equivalence();
  test_stress_and_performance();

  std::cout
      << "\n[SUCCESS] All unit tests, assertions, and benchmarks passed!\n";
  return 0;
}
