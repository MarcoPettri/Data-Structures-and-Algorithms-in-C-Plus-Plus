// File : C-12.9.cpp

/*
    C-12.9 Let T be a text of length n, and let P be a pattern of length m. Describe an
    O(n+m)-time method for finding the longest prefix of P that is a substring
    of T .
*/

#include "longest_prefix_substring.hpp"

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
 * @brief Independent brute-force oracle to formally verify dynamic programming/KMP correctness.
 * Evaluates all prefixes of P from largest to smallest and checks substring existence.
 */
struct OracleResult {
  std::size_t length{0};
  std::string prefix{};
  std::vector<std::size_t> occurrences{};
};

OracleResult brute_force_oracle(std::string_view text,
                                std::string_view pattern) {
  OracleResult result;
  const std::size_t n = text.size();
  const std::size_t m = pattern.size();

  if (n == 0 || m == 0) {
    return result;
  }

  for (std::size_t len = m; len >= 1; --len) {
    std::string_view pref = pattern.substr(0, len);
    std::vector<std::size_t> found_indices;

    for (std::size_t i = 0; i + len <= n; ++i) {
      if (text.substr(i, len) == pref) {
        found_indices.push_back(i);
      }
    }

    if (!found_indices.empty()) {
      result.length = len;
      result.prefix = std::string(pref);
      result.occurrences = std::move(found_indices);
      return result;
    }
  }

  return result;
}

void test_edge_cases() {
  std::cout << "[*] Running edge case tests...\n";

  // Edge Case 1: Both empty
  {
    auto res = dsa::find_longest_prefix_substring("", "");
    assert(res.length == 0);
    assert(res.prefix.empty());
    assert(!res.has_match());
    assert(!res.first_occurrence.has_value());
    assert(res.occurrences.empty());
  }

  // Edge Case 2: Empty text, non-empty pattern
  {
    auto res = dsa::find_longest_prefix_substring("", "pattern");
    assert(res.length == 0);
    assert(res.prefix.empty());
    assert(!res.has_match());
    assert(!res.first_occurrence.has_value());
  }

  // Edge Case 3: Non-empty text, empty pattern
  {
    auto res = dsa::find_longest_prefix_substring("text", "");
    assert(res.length == 0);
    assert(res.prefix.empty());
    assert(!res.has_match());
    assert(!res.first_occurrence.has_value());
  }

  // Edge Case 4: Single character match
  {
    auto res = dsa::find_longest_prefix_substring("a", "a");
    assert(res.length == 1);
    assert(res.prefix == "a");
    assert(res.has_match());
    assert(res.first_occurrence.value() == 0);
    assert(res.occurrences.size() == 1);
    assert(res.occurrences[0] == 0);
  }

  // Edge Case 5: Single character mismatch
  {
    auto res = dsa::find_longest_prefix_substring("a", "b");
    assert(res.length == 0);
    assert(res.prefix.empty());
    assert(!res.has_match());
    assert(!res.first_occurrence.has_value());
    assert(res.occurrences.empty());
  }

  // Edge Case 6: Completely disjoint alphabets
  {
    auto res = dsa::find_longest_prefix_substring("abcdef", "xyz");
    assert(res.length == 0);
    assert(res.prefix.empty());
    assert(!res.has_match());
    assert(res.occurrences.empty());
  }

  // Edge Case 7: Pattern longer than text, but whole text matches prefix of pattern
  {
    auto res = dsa::find_longest_prefix_substring("abc", "abcdefgh");
    assert(res.length == 3);
    assert(res.prefix == "abc");
    assert(res.first_occurrence.value() == 0);
    assert(res.occurrences.size() == 1);
  }

  std::cout << "    [+] Edge cases passed.\n";
}

void test_functional_cases() {
  std::cout << "[*] Running functional and structural tests...\n";

  // Test 1: Full pattern match inside text
  {
    auto res =
        dsa::find_longest_prefix_substring("the quick brown fox", "brown");
    assert(res.length == 5);
    assert(res.prefix == "brown");
    assert(res.first_occurrence.value() == 10);
    assert(res.occurrences.size() == 1);
    assert(res.occurrences[0] == 10);
  }

  // Test 2: Match at the very beginning of text
  {
    auto res =
        dsa::find_longest_prefix_substring("prefix_test", "prefix_other");
    assert(res.length == 7);
    assert(res.prefix == "prefix_");
    assert(res.first_occurrence.value() == 0);
    assert(res.occurrences.size() == 1);
  }

  // Test 3: Match at the very end of text
  {
    auto res =
        dsa::find_longest_prefix_substring("look_at_the_end_algo", "algorithm");
    assert(res.length == 4);
    assert(res.prefix == "algo");
    assert(res.first_occurrence.value() == 16);
    assert(res.occurrences.size() == 1);
  }

  // Test 4: Multiple overlapping occurrences of longest prefix
  {
    // P = "aaab", T = "aaaa" -> longest prefix is "aaa" (len 3)
    // Occurs at index 0 ("aaa".) and index 1 (. "aaa")
    auto res = dsa::find_longest_prefix_substring("aaaa", "aaab");
    assert(res.length == 3);
    assert(res.prefix == "aaa");
    assert(res.first_occurrence.value() == 0);
    assert(res.occurrences.size() == 2);
    assert(res.occurrences[0] == 0);
    assert(res.occurrences[1] == 1);
  }

  // Test 5: Reusable matcher class for multiple queries
  {
    dsa::LongestPrefixMatcher matcher("database_system");
    assert(matcher.pattern() == "database_system");
    assert(!matcher.failure_table().empty());

    auto res1 = matcher.find_in("modern_data_mining");
    assert(res1.length == 4);
    assert(res1.prefix == "data");
    assert(res1.first_occurrence.value() == 7);

    auto res2 = matcher.find_in("relational_database_design");
    // "database_" matches pattern[0..8] (length 9)
    assert(res2.length == 9);
    assert(res2.prefix == "database_");
    assert(res2.first_occurrence.value() == 11);

    auto res3 = matcher.find_in("no_match_here");
    assert(res3.length == 0);
    assert(!res3.has_match());
  }

  // Test 6: Pattern with non-trivial KMP failure transitions
  {
    // P = "abacaba" (pi table: [0, 0, 1, 0, 1, 2, 3])
    // T = "abacabx_abacaba"
    auto res = dsa::find_longest_prefix_substring("abacabx_abacaba", "abacaba");
    assert(res.length == 7);
    assert(res.prefix == "abacaba");
    assert(res.first_occurrence.value() == 8);
    assert(res.occurrences.size() == 1);
  }

  std::cout << "    [+] Functional and structural tests passed.\n";
}

void test_oracle_brute_force_equivalence() {
  std::cout << "[*] Verifying against brute-force oracle across randomized "
               "datasets...\n";

  std::mt19937_64 rng(1337);

  // Test with small binary alphabet {0, 1} (high collision and overlap probability)
  {
    std::uniform_int_distribution<int> char_dist('0', '1');
    std::uniform_int_distribution<std::size_t> len_dist(1, 40);

    for (int iter = 0; iter < 200; ++iter) {
      const std::size_t n = len_dist(rng);
      const std::size_t m = len_dist(rng);

      std::string text(n, ' ');
      for (auto &c : text)
        c = static_cast<char>(char_dist(rng));

      std::string pattern(m, ' ');
      for (auto &c : pattern)
        c = static_cast<char>(char_dist(rng));

      auto oracle = brute_force_oracle(text, pattern);
      auto kmp_res = dsa::find_longest_prefix_substring(text, pattern);

      assert(kmp_res.length == oracle.length);
      assert(kmp_res.prefix == oracle.prefix);
      assert(kmp_res.occurrences == oracle.occurrences);
      if (oracle.length > 0) {
        assert(kmp_res.first_occurrence.has_value());
        assert(kmp_res.first_occurrence.value() == oracle.occurrences.front());
      } else {
        assert(!kmp_res.first_occurrence.has_value());
      }
    }
  }

  // Test with ternary alphabet {a, b, c}
  {
    std::uniform_int_distribution<int> char_dist('a', 'c');
    std::uniform_int_distribution<std::size_t> len_dist(1, 50);

    for (int iter = 0; iter < 200; ++iter) {
      const std::size_t n = len_dist(rng);
      const std::size_t m = len_dist(rng);

      std::string text(n, ' ');
      for (auto &c : text)
        c = static_cast<char>(char_dist(rng));

      std::string pattern(m, ' ');
      for (auto &c : pattern)
        c = static_cast<char>(char_dist(rng));

      auto oracle = brute_force_oracle(text, pattern);
      auto kmp_res = dsa::find_longest_prefix_substring(text, pattern);

      assert(kmp_res.length == oracle.length);
      assert(kmp_res.prefix == oracle.prefix);
      assert(kmp_res.occurrences == oracle.occurrences);
    }
  }

  std::cout << "    [+] Brute-force equivalence verified across 400 randomized "
               "test cases.\n";
}

void test_stress_and_performance() {
  std::cout << "[*] Running stress and performance benchmark...\n";

  // Text size n = 1,000,000, Pattern size m = 100,000
  // Highly repetitive structure designed to trigger maximal KMP failure function traversals
  const std::size_t n = 1'000'000;
  const std::size_t m = 100'000;

  std::string text(n, 'a');
  text[n / 2] = 'b'; // Single disruption

  std::string pattern(m, 'a');
  pattern.back() = 'b'; // 'a' repeated m-1 times, followed by 'b'

  const auto start = std::chrono::high_resolution_clock::now();
  auto res = dsa::find_longest_prefix_substring(text, pattern);
  const auto end = std::chrono::high_resolution_clock::now();

  const auto duration_ms =
      std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
          end - start)
          .count();

  // Since text has "aaaa...b" ending at n/2, the match should match up to 'b'
  // Pattern has m-1 'a's and then 'b'.
  // At index n/2, there are n/2 'a's preceding 'b'. Since n/2 >= m-1, the full pattern appears!
  assert(res.length == m);
  assert(res.prefix == pattern);
  assert(res.first_occurrence.has_value());
  assert(res.first_occurrence.value() == (n / 2) - (m - 1));

  std::cout << "    -> Text N = " << n << ", Pattern M = " << m
            << " | Matched Length = " << res.length
            << " | Elapsed Time = " << std::fixed << std::setprecision(2)
            << duration_ms << " ms\n";

  std::cout << "    [+] Stress and benchmark tests completed successfully.\n";
}

} // namespace

int main() {
  std::cout << "========================================================\n";
  std::cout << "   C-12.9: Longest Prefix of P that is Substring of T   \n";
  std::cout << "========================================================\n\n";

  test_edge_cases();
  test_functional_cases();
  test_oracle_brute_force_equivalence();
  test_stress_and_performance();

  std::cout
      << "\n[SUCCESS] All unit tests, assertions, and benchmarks passed!\n";
  return 0;
}
