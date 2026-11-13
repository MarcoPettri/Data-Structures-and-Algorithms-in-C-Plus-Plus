// File : C-12.13.cpp

/*
    C-12.13 Given a string X of length n and a string Y of length m, describe an O(n +m)-time 
    algorithm for finding the longest prefix of X that is a suffix of Y .
*/

#include "prefix_suffix_overlap.hpp"

#include <cassert>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <vector>

namespace {

/**
 * @brief Independent brute-force oracle to formally verify prefix-suffix overlap.
 * Checks lengths from min(|X|, |Y|) down to 1 for exact equality.
 */
std::size_t brute_force_oracle(std::string_view x, std::string_view y) {
  const std::size_t max_possible = std::min(x.size(), y.size());

  for (std::size_t len = max_possible; len >= 1; --len) {
    if (x.substr(0, len) == y.substr(y.size() - len, len)) {
      return len;
    }
  }

  return 0;
}

void test_edge_cases() {
  std::cout << "[*] Running edge case tests...\n";

  // Edge Case 1: Both empty
  {
    auto res = dsa::find_longest_prefix_suffix_overlap("", "");
    assert(res.length == 0);
    assert(res.matching_slice.empty());
    assert(!res.has_overlap());
    assert(!res.suffix_start_index.has_value());
    assert(res.all_overlap_lengths.empty());
  }

  // Edge Case 2: Empty X, non-empty Y
  {
    auto res = dsa::find_longest_prefix_suffix_overlap("", "hello");
    assert(res.length == 0);
    assert(!res.has_overlap());
    assert(!res.suffix_start_index.has_value());
  }

  // Edge Case 3: Non-empty X, empty Y
  {
    auto res = dsa::find_longest_prefix_suffix_overlap("world", "");
    assert(res.length == 0);
    assert(!res.has_overlap());
    assert(!res.suffix_start_index.has_value());
  }

  // Edge Case 4: Single character matching
  {
    auto res = dsa::find_longest_prefix_suffix_overlap("a", "a");
    assert(res.length == 1);
    assert(res.matching_slice == "a");
    assert(res.has_overlap());
    assert(res.suffix_start_index.value() == 0);
    assert(res.all_overlap_lengths.size() == 1);
    assert(res.all_overlap_lengths[0] == 1);
  }

  // Edge Case 5: Single character mismatch
  {
    auto res = dsa::find_longest_prefix_suffix_overlap("a", "b");
    assert(res.length == 0);
    assert(!res.has_overlap());
    assert(!res.suffix_start_index.has_value());
  }

  // Edge Case 6: Identical strings (X == Y)
  {
    auto res = dsa::find_longest_prefix_suffix_overlap("prefix_and_suffix",
                                                       "prefix_and_suffix");
    assert(res.length == 17);
    assert(res.matching_slice == "prefix_and_suffix");
    assert(res.suffix_start_index.value() == 0);
  }

  // Edge Case 7: Disjoint alphabets
  {
    auto res = dsa::find_longest_prefix_suffix_overlap("abcdef", "123456");
    assert(res.length == 0);
    assert(!res.has_overlap());
  }

  std::cout << "    [+] Edge cases passed.\n";
}

void test_functional_cases() {
  std::cout << "[*] Running functional and structural tests...\n";

  // Test 1: Standard partial overlap
  // X = "cde_fgh", Y = "abc_cde" -> prefix of X "cde" is suffix of Y
  {
    auto res = dsa::find_longest_prefix_suffix_overlap("cde_fgh", "abc_cde");
    assert(res.length == 3);
    assert(res.matching_slice == "cde");
    assert(res.suffix_start_index.value() == 4);
  }

  // Test 2: Entire X is a suffix of Y (n < m)
  // X = "algorithm", Y = "advanced_algorithm"
  {
    auto res = dsa::find_longest_prefix_suffix_overlap("algorithm",
                                                       "advanced_algorithm");
    assert(res.length == 9);
    assert(res.matching_slice == "algorithm");
    assert(res.suffix_start_index.value() == 9);
  }

  // Test 3: Entire Y is a prefix of X (m < n)
  // X = "datastructure", Y = "data"
  {
    auto res = dsa::find_longest_prefix_suffix_overlap("datastructure", "data");
    assert(res.length == 4);
    assert(res.matching_slice == "data");
    assert(res.suffix_start_index.value() == 0);
  }

  // Test 4: Multiple valid overlap lengths
  // X = "abacaba", Y = "xyzabaca"
  // Prefixes of X that are suffixes of Y: "abaca" (len 5) and "a" (len 1)
  {
    auto res = dsa::find_longest_prefix_suffix_overlap("abacaba", "xyzabaca");
    assert(res.length == 5);
    assert(res.matching_slice == "abaca");
    assert(res.suffix_start_index.value() == 3);
    assert(res.all_overlap_lengths.size() == 2);
    assert(res.all_overlap_lengths[0] == 5);
    assert(res.all_overlap_lengths[1] == 1);
  }

  // Test 5: Repetitive pattern
  // X = "aaaa", Y = "aa" -> length 2
  {
    auto res = dsa::find_longest_prefix_suffix_overlap("aaaa", "aa");
    assert(res.length == 2);
    assert(res.matching_slice == "aa");
    assert(res.suffix_start_index.value() == 0);
    assert(res.all_overlap_lengths.size() == 2);
    assert(res.all_overlap_lengths[0] == 2);
    assert(res.all_overlap_lengths[1] == 1);
  }

  // Test 6: Reusable PrefixSuffixMatcher on multiple queries
  {
    dsa::PrefixSuffixMatcher matcher("pattern_recognition");
    assert(matcher.string_x() == "pattern_recognition");

    auto res1 = matcher.find_overlap_with("test_pattern");
    assert(res1.length == 7);
    assert(res1.matching_slice == "pattern");
    assert(res1.suffix_start_index.value() == 5);

    auto res2 = matcher.find_overlap_with("learn_pattern_recog");
    assert(res2.length == 13);
    assert(res2.matching_slice == "pattern_recog");
    assert(res2.suffix_start_index.value() == 6);

    auto res3 = matcher.find_overlap_with("totally_different");
    assert(res3.length == 0);
    assert(!res3.has_overlap());
  }

  std::cout << "    [+] Functional and structural tests passed.\n";
}

void test_oracle_brute_force_equivalence() {
  std::cout << "[*] Verifying against brute-force oracle across randomized "
               "datasets...\n";

  std::mt19937_64 rng(2026);

  // Test over binary alphabet {0, 1}
  {
    std::uniform_int_distribution<int> char_dist('0', '1');
    std::uniform_int_distribution<std::size_t> len_dist(1, 60);

    for (int iter = 0; iter < 300; ++iter) {
      const std::size_t n = len_dist(rng);
      const std::size_t m = len_dist(rng);

      std::string x(n, ' ');
      for (auto &c : x)
        c = static_cast<char>(char_dist(rng));

      std::string y(m, ' ');
      for (auto &c : y)
        c = static_cast<char>(char_dist(rng));

      const std::size_t oracle_len = brute_force_oracle(x, y);
      auto kmp_res = dsa::find_longest_prefix_suffix_overlap(x, y);

      assert(kmp_res.length == oracle_len);
      if (oracle_len > 0) {
        assert(kmp_res.matching_slice == x.substr(0, oracle_len));
        assert(kmp_res.matching_slice ==
               y.substr(y.size() - oracle_len, oracle_len));
        assert(kmp_res.suffix_start_index.value() == y.size() - oracle_len);
      } else {
        assert(!kmp_res.has_overlap());
      }
    }
  }

  // Test over ternary alphabet {a, b, c}
  {
    std::uniform_int_distribution<int> char_dist('a', 'c');
    std::uniform_int_distribution<std::size_t> len_dist(1, 60);

    for (int iter = 0; iter < 300; ++iter) {
      const std::size_t n = len_dist(rng);
      const std::size_t m = len_dist(rng);

      std::string x(n, ' ');
      for (auto &c : x)
        c = static_cast<char>(char_dist(rng));

      std::string y(m, ' ');
      for (auto &c : y)
        c = static_cast<char>(char_dist(rng));

      const std::size_t oracle_len = brute_force_oracle(x, y);
      auto kmp_res = dsa::find_longest_prefix_suffix_overlap(x, y);

      assert(kmp_res.length == oracle_len);
    }
  }

  std::cout << "    [+] Brute-force equivalence verified across 600 randomized "
               "test cases.\n";
}

void test_stress_and_performance() {
  std::cout << "[*] Running stress and performance benchmark...\n";

  // Text sizes n = 1,000,000, m = 1,000,000
  const std::size_t n = 1'000'000;
  const std::size_t m = 1'000'000;
  const std::size_t overlap_len = 400'000;

  std::string x(n, 'a');
  x[overlap_len] = 'b'; // disruption after overlap

  std::string y(m, 'c');
  // Set the suffix of Y to match the prefix of X of length overlap_len
  for (std::size_t i = 0; i < overlap_len; ++i) {
    y[m - overlap_len + i] = x[i];
  }

  const auto start = std::chrono::high_resolution_clock::now();
  auto res = dsa::find_longest_prefix_suffix_overlap(x, y);
  const auto end = std::chrono::high_resolution_clock::now();

  const auto duration_ms =
      std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
          end - start)
          .count();

  assert(res.length == overlap_len);
  assert(res.suffix_start_index.has_value());
  assert(res.suffix_start_index.value() == m - overlap_len);

  std::cout << "    -> |X| = " << n << ", |Y| = " << m
            << " | Overlap Length = " << res.length
            << " | Elapsed Time = " << std::fixed << std::setprecision(2)
            << duration_ms << " ms\n";

  std::cout << "    [+] Stress and benchmark tests completed successfully.\n";
}

} // namespace

int main() {
  std::cout << "========================================================\n";
  std::cout << "  C-12.13: Longest Prefix of X that is a Suffix of Y   \n";
  std::cout << "========================================================\n\n";

  test_edge_cases();
  test_functional_cases();
  test_oracle_brute_force_equivalence();
  test_stress_and_performance();

  std::cout
      << "\n[SUCCESS] All unit tests, assertions, and benchmarks passed!\n";
  return 0;
}
