// File : C-12.10.cpp

/*
    C-12.10 Say that a pattern P of length m is a circular substring of a text T of
    length n if there is an index 0 \leq i < m, such that P = T [n - m + i..n - 1] +
    T [0..i - 1], that is, if P is a (normal) substring of T or P is equal to the
    concatenation of a suffix of T and a prefix of T . Give an O(n + m)-time
    algorithm for determining whether P is a circular substring of T.
*/

#include "circular_substring_matcher.hpp"

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
 * @brief Independent brute-force oracle verifying the exact mathematical definition from C-12.10.
 * Checks whether P appears as a normal substring of T or as T[n - m + i..n - 1] + T[0..i - 1].
 */
bool oracle_is_circular_substring(std::string_view text,
                                  std::string_view pattern) {
  const std::size_t n = text.size();
  const std::size_t m = pattern.size();

  if (m == 0)
    return true;
  if (n == 0)
    return false;

  // Normal substring check
  if (text.find(pattern) != std::string_view::npos) {
    return true;
  }

  // Wrap check: P = T[n - m + i .. n - 1] + T[0 .. i - 1] for 0 <= i < m
  for (std::size_t i = 0; i < m; ++i) {
    // Condition: (n - m + i >= 0) and (i <= n)
    if (n + i >= m && i <= n) {
      const std::size_t suffix_start = n + i - m;
      const std::size_t suffix_len = n - suffix_start;
      const std::size_t prefix_len = i;

      if (suffix_start <= n && suffix_len + prefix_len == m) {
        std::string candidate =
            std::string(text.substr(suffix_start, suffix_len)) +
            std::string(text.substr(0, prefix_len));
        if (candidate == pattern) {
          return true;
        }
      }
    }
  }

  return false;
}

void test_edge_cases() {
  std::cout << "[*] Running edge case tests...\n";

  // Edge Case 1: Empty pattern
  {
    auto res1 = dsa::is_circular_substring("", "");
    assert(res1.is_circular_substring);

    auto res2 = dsa::is_circular_substring("algorithm", "");
    assert(res2.is_circular_substring);
  }

  // Edge Case 2: Empty text, non-empty pattern
  {
    auto res = dsa::is_circular_substring("", "pattern");
    assert(!res.is_circular_substring);
    assert(!res.first_occurrence.has_value());
  }

  // Edge Case 3: Single character match
  {
    auto res = dsa::is_circular_substring("a", "a");
    assert(res.is_circular_substring);
    assert(res.first_occurrence.has_value());
    assert(res.first_occurrence->start_index == 0);
    assert(!res.first_occurrence->is_wrapped);
  }

  // Edge Case 4: Single character mismatch
  {
    auto res = dsa::is_circular_substring("a", "b");
    assert(!res.is_circular_substring);
  }

  // Edge Case 5: Pattern length m > 2n under strict SuffixPrefixOnly mode
  {
    // Text length 3, pattern length 7 -> impossible with single suffix + single prefix
    auto res = dsa::is_circular_substring(
        "abc", "abcabca", dsa::CircularMatchMode::SuffixPrefixOnly);
    assert(!res.is_circular_substring);
  }

  // Edge Case 6: Pattern length m > 2n under AllowArbitraryWrap mode
  {
    // T = "abc", P = "abcabcab" (periodic wrap)
    auto res = dsa::is_circular_substring(
        "abc", "abcabcab", dsa::CircularMatchMode::AllowArbitraryWrap);
    assert(res.is_circular_substring);
    assert(res.first_occurrence->start_index == 0);

    // Disrupted periodic wrap
    auto res_bad = dsa::is_circular_substring(
        "abc", "abcabcax", dsa::CircularMatchMode::AllowArbitraryWrap);
    assert(!res_bad.is_circular_substring);
  }

  std::cout << "    [+] Edge cases passed.\n";
}

void test_functional_cases() {
  std::cout << "[*] Running functional and formula validation tests...\n";

  // Test 1: Normal (linear) substring
  {
    auto res = dsa::is_circular_substring("algorithm", "rithm");
    assert(res.is_circular_substring);
    assert(res.first_occurrence.has_value());
    assert(res.first_occurrence->start_index == 4);
    assert(!res.first_occurrence->is_wrapped);
    assert(res.first_occurrence->suffix_length == 5);
    assert(res.first_occurrence->prefix_length == 0);
  }

  // Test 2: Wrapped circular substring: T = "algorithm" (n = 9), P = "thmalgo" (m = 7)
  // Suffix T[6..8] = "thm" (length 3), Prefix T[0..3] = "algo" (length 4)
  // Formula verification: i = 4. n - m + i = 9 - 7 + 4 = 6.
  // T[6..8] + T[0..3] = "thmalgo"
  {
    std::string text = "algorithm";
    std::string pattern = "thmalgo";
    auto res = dsa::is_circular_substring(text, pattern);

    assert(res.is_circular_substring);
    assert(res.first_occurrence.has_value());

    const auto &occ = res.first_occurrence.value();
    assert(occ.start_index == 6);
    assert(occ.is_wrapped);
    assert(occ.suffix_length == 3);
    assert(occ.prefix_length == 4);
    assert(occ.split_index_i == 4);

    // Verify exact formula from problem statement:
    // P == T[n - m + i .. n - 1] + T[0 .. i - 1]
    const std::size_t n = text.size();
    const std::size_t m = pattern.size();
    const std::size_t i = occ.split_index_i;
    std::string reconstructed =
        text.substr(n - m + i, n - (n - m + i)) + text.substr(0, i);
    assert(reconstructed == pattern);
  }

  // Test 3: Circular permutations (shifts) of the entire text (m = n)
  {
    std::string text = "banana";
    // All circular shifts of "banana":
    // "banana", "ananab", "nanaba", "anaban", "nabana", "abanan"
    std::vector<std::string> valid_shifts = {"banana", "ananab", "nanaba",
                                             "anaban", "nabana", "abanan"};

    for (const auto &shift : valid_shifts) {
      auto res = dsa::is_circular_substring(text, shift);
      assert(res.is_circular_substring);
      assert(res.first_occurrence.has_value());
    }

    // Invalid shift (reversed characters)
    auto res_invalid = dsa::is_circular_substring(text, "ananba");
    assert(!res_invalid.is_circular_substring);
  }

  // Test 4: Multiple circular occurrences
  {
    // T = "abab", P = "ba"
    // Occurrences:
    // 1. start_index 1: linear "ba" (T[1..2])
    // 2. start_index 3: wrapped "ba" (T[3] + T[0] = 'b' + 'a')
    auto res = dsa::is_circular_substring("abab", "ba");
    assert(res.is_circular_substring);
    assert(res.occurrences.size() == 2);

    assert(res.occurrences[0].start_index == 1);
    assert(!res.occurrences[0].is_wrapped);

    assert(res.occurrences[1].start_index == 3);
    assert(res.occurrences[1].is_wrapped);
    assert(res.occurrences[1].suffix_length == 1);
    assert(res.occurrences[1].prefix_length == 1);
    assert(res.occurrences[1].split_index_i == 1);
  }

  // Test 5: Reusable matcher class for querying multiple texts
  {
    dsa::CircularSubstringMatcher matcher("cycle");

    assert(matcher.is_circular_substring("motorcycle")); // linear substring
    assert(
        matcher.is_circular_substring("le_motor_cyc")); // wrapped: "cyc" + "le"
    assert(matcher.is_circular_substring("tricycle_broken"));
    assert(matcher.is_circular_substring("recycled"));
    assert(!matcher.is_circular_substring("no_match_here")); // no match
  }

  std::cout << "    [+] Functional and formula validation tests passed.\n";
}

void test_oracle_brute_force_equivalence() {
  std::cout << "[*] Verifying against brute-force oracle across randomized "
               "datasets...\n";

  std::mt19937_64 rng(42);

  // Test across binary alphabet {0, 1}
  {
    std::uniform_int_distribution<int> char_dist('0', '1');
    std::uniform_int_distribution<std::size_t> len_dist(1, 30);

    for (int iter = 0; iter < 300; ++iter) {
      const std::size_t n = len_dist(rng);
      const std::size_t m = len_dist(rng);

      std::string text(n, ' ');
      for (auto &c : text)
        c = static_cast<char>(char_dist(rng));

      std::string pattern(m, ' ');
      for (auto &c : pattern)
        c = static_cast<char>(char_dist(rng));

      const bool oracle_res = oracle_is_circular_substring(text, pattern);
      auto kmp_res = dsa::is_circular_substring(text, pattern);

      assert(kmp_res.is_circular_substring == oracle_res);
    }
  }

  // Test across alphabet {a, b, c}
  {
    std::uniform_int_distribution<int> char_dist('a', 'c');
    std::uniform_int_distribution<std::size_t> len_dist(1, 40);

    for (int iter = 0; iter < 300; ++iter) {
      const std::size_t n = len_dist(rng);
      const std::size_t m = len_dist(rng);

      std::string text(n, ' ');
      for (auto &c : text)
        c = static_cast<char>(char_dist(rng));

      std::string pattern(m, ' ');
      for (auto &c : pattern)
        c = static_cast<char>(char_dist(rng));

      const bool oracle_res = oracle_is_circular_substring(text, pattern);
      auto kmp_res = dsa::is_circular_substring(text, pattern);

      assert(kmp_res.is_circular_substring == oracle_res);
    }
  }

  std::cout << "    [+] Brute-force equivalence verified across 600 randomized "
               "test cases.\n";
}

void test_stress_and_performance() {
  std::cout << "[*] Running stress and performance benchmark...\n";

  // Text size n = 1,000,000, Pattern size m = 200,000
  // Pattern constructed by wrapping suffix of T and prefix of T
  const std::size_t n = 1'000'000;
  const std::size_t m = 200'000;

  std::string text(n, 'a');
  text[n - 1] = 'x'; // Marker at the end
  text[0] = 'y'; // Marker at the beginning

  // P = suffix of length 100,000 + prefix of length 100,000
  // Suffix ends with 'x', Prefix starts with 'y'
  std::string pattern =
      text.substr(n - 100'000, 100'000) + text.substr(0, 100'000);

  const auto start = std::chrono::high_resolution_clock::now();
  auto res = dsa::is_circular_substring(text, pattern);
  const auto end = std::chrono::high_resolution_clock::now();

  const auto duration_ms =
      std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
          end - start)
          .count();

  assert(res.is_circular_substring);
  assert(res.first_occurrence.has_value());
  assert(res.first_occurrence->is_wrapped);
  assert(res.first_occurrence->start_index == n - 100'000);
  assert(res.first_occurrence->suffix_length == 100'000);
  assert(res.first_occurrence->prefix_length == 100'000);

  std::cout << "    -> Text N = " << n << ", Pattern M = " << m
            << " | Is Circular Substring: " << std::boolalpha
            << res.is_circular_substring << " | Elapsed Time = " << std::fixed
            << std::setprecision(2) << duration_ms << " ms\n";

  std::cout << "    [+] Stress and benchmark tests completed successfully.\n";
}

} // namespace

int main() {
  std::cout << "========================================================\n";
  std::cout << "    C-12.10: Circular Substring Detection in O(n + m)   \n";
  std::cout << "========================================================\n\n";

  test_edge_cases();
  test_functional_cases();
  test_oracle_brute_force_equivalence();
  test_stress_and_performance();

  std::cout
      << "\n[SUCCESS] All unit tests, assertions, and benchmarks passed!\n";
  return 0;
}
