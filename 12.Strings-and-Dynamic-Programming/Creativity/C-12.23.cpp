// File : C-12.23.cpp

/*
    C-12.23 Let x and y be strings of length n and m respectively. Define B(i, j) to
    be the length of the longest common substring of the suffix of length i
    in x and the suffix of length j in y. Design an O(nm)-time algorithm for
    computing all the values of B(i, j) for i = 1, . . . , n and j = 1, . . . , m.
*/

#include "suffix_longest_common_substring.hpp"

#include <cassert>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <string_view>

namespace {

/**
 * @brief Independent brute-force oracle computing the longest common substring
 * of two strings by inspecting all substrings of the first string.
 */
std::size_t brute_force_lcs(std::string_view s1, std::string_view s2) {
  std::size_t max_len = 0;
  const std::size_t len1 = s1.size();

  for (std::size_t i = 0; i < len1; ++i) {
    for (std::size_t len = 1; i + len <= len1; ++len) {
      std::string_view sub = s1.substr(i, len);
      if (s2.find(sub) != std::string_view::npos) {
        if (len > max_len) {
          max_len = len;
        }
      }
    }
  }

  return max_len;
}

void test_edge_cases() {
  std::cout << "[*] Running edge case tests...\n";

  // Edge Case 1: Both empty
  {
    auto table = dsa::compute_suffix_lcs_table("", "");
    assert(table.n() == 0);
    assert(table.m() == 0);
    assert(table(0, 0) == 0);
    assert(table.overall_longest() == 0);
  }

  // Edge Case 2: Source empty
  {
    auto table = dsa::compute_suffix_lcs_table("", "abc");
    assert(table.n() == 0);
    assert(table.m() == 3);
    for (std::size_t j = 0; j <= 3; ++j) {
      assert(table(0, j) == 0);
    }
  }

  // Edge Case 3: Target empty
  {
    auto table = dsa::compute_suffix_lcs_table("xyz", "");
    assert(table.n() == 3);
    assert(table.m() == 0);
    for (std::size_t i = 0; i <= 3; ++i) {
      assert(table(i, 0) == 0);
    }
  }

  // Edge Case 4: Single character match
  {
    auto table = dsa::compute_suffix_lcs_table("a", "a");
    assert(table(1, 1) == 1);
    assert(table.overall_longest() == 1);
  }

  // Edge Case 5: Single character mismatch
  {
    auto table = dsa::compute_suffix_lcs_table("a", "b");
    assert(table(1, 1) == 0);
    assert(table.overall_longest() == 0);
  }

  // Edge Case 6: Identical strings: B(i, i) should be exactly i
  {
    std::string s = "algorithms";
    auto table = dsa::compute_suffix_lcs_table(s, s);
    for (std::size_t i = 1; i <= s.size(); ++i) {
      assert(table(i, i) == i);
    }
    assert(table.overall_longest() == s.size());
  }

  std::cout << "    [+] Edge cases passed.\n";
}

void test_analytical_and_manual_cases() {
  std::cout << "[*] Running analytical and manual verification tests...\n";

  // Test 1: Hand-computed example: X = "ab", Y = "ba"
  // Suffixes of X: X^(1) = "b", X^(2) = "ab"
  // Suffixes of Y: Y^(1) = "a", Y^(2) = "ba"
  // B(1, 1) = LCS("b", "a") = 0
  // B(1, 2) = LCS("b", "ba") = 1 ("b")
  // B(2, 1) = LCS("ab", "a") = 1 ("a")
  // B(2, 2) = LCS("ab", "ba") = 1 ("a" or "b")
  {
    auto table = dsa::compute_suffix_lcs_table("ab", "ba");
    assert(table(1, 1) == 0);
    assert(table(1, 2) == 1);
    assert(table(2, 1) == 1);
    assert(table(2, 2) == 1);
  }

  // Test 2: Hand-computed example 2: X = "abc", Y = "xaby"
  // Suffixes of X: X^(1) = "c", X^(2) = "bc", X^(3) = "abc"
  // Suffixes of Y: Y^(1) = "y", Y^(2) = "by", Y^(3) = "aby", Y^(4) = "xaby"
  {
    auto table = dsa::compute_suffix_lcs_table("abc", "xaby");
    assert(table(3, 3) == 2); // LCS("abc", "aby") = "ab" (len 2)
    assert(table(3, 4) == 2); // LCS("abc", "xaby") = "ab" (len 2)
    assert(table(2, 3) == 1); // LCS("bc", "aby") = "b" (len 1)
    assert(table(1, 1) == 0); // LCS("c", "y") = 0
  }

  // Test 3: Structural Monotonicity and Boundedness Verification
  // B(i, j) >= B(i-1, j) and B(i, j) >= B(i, j-1)
  // 0 <= B(i, j) <= min(i, j)
  {
    std::string x = "supercalifragilistic";
    std::string y = "fragile_california";
    auto table = dsa::compute_suffix_lcs_table(x, y);

    for (std::size_t i = 1; i <= x.size(); ++i) {
      for (std::size_t j = 1; j <= y.size(); ++j) {
        // Monotonicity
        assert(table(i, j) >= table(i - 1, j));
        assert(table(i, j) >= table(i, j - 1));
        // Boundedness
        assert(table(i, j) <= std::min(i, j));
      }
    }
  }

  std::cout << "    [+] Analytical and manual verification tests passed.\n";
}

void test_oracle_brute_force_equivalence() {
  std::cout << "[*] Verifying all table cells against independent brute-force "
               "oracle...\n";

  std::mt19937_64 rng(12345);
  std::uniform_int_distribution<int> char_dist(
      'a', 'c'); // small alphabet for overlaps
  std::uniform_int_distribution<std::size_t> len_dist(1, 20);

  for (int iter = 0; iter < 100; ++iter) {
    const std::size_t n = len_dist(rng);
    const std::size_t m = len_dist(rng);

    std::string x(n, ' ');
    for (auto &c : x)
      c = static_cast<char>(char_dist(rng));

    std::string y(m, ' ');
    for (auto &c : y)
      c = static_cast<char>(char_dist(rng));

    auto table = dsa::compute_suffix_lcs_table(x, y);

    // Verify every single B(i, j) against the brute-force oracle
    for (std::size_t i = 1; i <= n; ++i) {
      std::string_view suffix_x = std::string_view(x).substr(n - i, i);

      for (std::size_t j = 1; j <= m; ++j) {
        std::string_view suffix_y = std::string_view(y).substr(m - j, j);

        const std::size_t oracle_val = brute_force_lcs(suffix_x, suffix_y);
        const std::size_t dp_val = table(i, j);

        assert(dp_val == oracle_val);
      }
    }
  }

  std::cout << "    [+] Brute-force equivalence verified across 100 test "
               "matrices (thousands of cells checked).\n";
}

void test_stress_and_performance() {
  std::cout << "[*] Running stress and performance benchmark...\n";

  // Matrix size: 3000 x 3000 = 9,000,000 entries
  const std::size_t n = 3000;
  const std::size_t m = 3000;

  std::mt19937_64 rng(777);
  std::uniform_int_distribution<int> char_dist('a', 'z');

  std::string x(n, ' ');
  for (auto &c : x)
    c = static_cast<char>(char_dist(rng));

  std::string y(m, ' ');
  for (auto &c : y)
    c = static_cast<char>(char_dist(rng));

  const auto start = std::chrono::high_resolution_clock::now();
  auto table = dsa::compute_suffix_lcs_table(x, y);
  const auto end = std::chrono::high_resolution_clock::now();

  const auto duration_ms =
      std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
          end - start)
          .count();

  assert(table.overall_longest() > 0);
  assert(table.overall_longest() <= std::min(n, m));

  std::cout << "    -> |X| = " << n << ", |Y| = " << m
            << " | Total Cells Computed: 9M | Overall Longest = "
            << table.overall_longest() << " | Elapsed Time = " << std::fixed
            << std::setprecision(2) << duration_ms << " ms\n";

  std::cout << "    [+] Stress and benchmark tests completed successfully.\n";
}

} // namespace

int main() {
  std::cout << "========================================================\n";
  std::cout << "  C-12.23: Suffix Longest Common Substring Table B(i,j) \n";
  std::cout << "========================================================\n\n";

  test_edge_cases();
  test_analytical_and_manual_cases();
  test_oracle_brute_force_equivalence();
  test_stress_and_performance();

  std::cout
      << "\n[SUCCESS] All unit tests, assertions, and benchmarks passed!\n";
  return 0;
}
