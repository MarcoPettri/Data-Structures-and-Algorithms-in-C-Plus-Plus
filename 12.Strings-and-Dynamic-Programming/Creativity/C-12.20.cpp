// File : C-12.20.cpp

/*
    C-12.20 Define the edit distance between two strings X and Y of length n and m,
    respectively, to be the number of edits that it takes to change X into Y . An
    edit consists of a character insertion, a character deletion, or a character
    replacement. For example, the strings "algorithm" and "rhythm" have
    edit distance 6. Design an O(nm)-time algorithm for computing the edit
    distance between X and Y .
*/

#include "edit_distance.hpp"

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
 * @brief Independent recursive brute-force oracle for small strings.
 * Used to mathematically verify the dynamic programming solution.
 */
std::size_t brute_force_oracle(std::string_view x, std::string_view y) {
  if (x.empty())
    return y.size();
  if (y.empty())
    return x.size();

  if (x.back() == y.back()) {
    return brute_force_oracle(x.substr(0, x.size() - 1),
                              y.substr(0, y.size() - 1));
  }

  const std::size_t replace_cost =
      brute_force_oracle(x.substr(0, x.size() - 1), y.substr(0, y.size() - 1)) +
      1;
  const std::size_t delete_cost =
      brute_force_oracle(x.substr(0, x.size() - 1), y) + 1;
  const std::size_t insert_cost =
      brute_force_oracle(x, y.substr(0, y.size() - 1)) + 1;

  return std::min({replace_cost, delete_cost, insert_cost});
}

void test_edge_cases() {
  std::cout << "[*] Running edge case tests...\n";

  // Edge Case 1: Both empty
  {
    assert(dsa::compute_edit_distance("", "") == 0);
    auto res = dsa::compute_edit_distance_with_script("", "");
    assert(res.distance == 0);
    assert(res.script.empty());
    assert(res.verify_transformation("", ""));
  }

  // Edge Case 2: Source empty (pure insertions)
  {
    assert(dsa::compute_edit_distance("", "abc") == 3);
    auto res = dsa::compute_edit_distance_with_script("", "abc");
    assert(res.distance == 3);
    assert(res.script.size() == 3);
    assert(res.verify_transformation("", "abc"));
  }

  // Edge Case 3: Target empty (pure deletions)
  {
    assert(dsa::compute_edit_distance("xyz", "") == 3);
    auto res = dsa::compute_edit_distance_with_script("xyz", "");
    assert(res.distance == 3);
    assert(res.script.size() == 3);
    assert(res.verify_transformation("xyz", ""));
  }

  // Edge Case 4: Single character identical
  {
    assert(dsa::compute_edit_distance("a", "a") == 0);
    auto res = dsa::compute_edit_distance_with_script("a", "a");
    assert(res.distance == 0);
    assert(res.verify_transformation("a", "a"));
  }

  // Edge Case 5: Single character replacement
  {
    assert(dsa::compute_edit_distance("a", "b") == 1);
    auto res = dsa::compute_edit_distance_with_script("a", "b");
    assert(res.distance == 1);
    assert(res.verify_transformation("a", "b"));
  }

  // Edge Case 6: Identical longer strings
  {
    std::string s = "dynamic_programming_with_modern_cpp17";
    assert(dsa::compute_edit_distance(s, s) == 0);
    auto res = dsa::compute_edit_distance_with_script(s, s);
    assert(res.distance == 0);
    assert(res.verify_transformation(s, s));
  }

  std::cout << "    [+] Edge cases passed.\n";
}

void test_textbook_and_canonical_cases() {
  std::cout << "[*] Running textbook and canonical test cases...\n";

  // Test 1: Textbook Example from Problem C-12.20
  // "algorithm" and "rhythm" have edit distance 6
  {
    const std::string x = "algorithm";
    const std::string y = "rhythm";

    const std::size_t dist = dsa::compute_edit_distance(x, y);
    assert(dist == 6);

    auto res = dsa::compute_edit_distance_with_script(x, y);
    assert(res.distance == 6);
    assert(res.verify_transformation(x, y));

    std::cout << "    [i] Textbook Alignment:\n"
              << "        X: " << res.aligned_x << "\n"
              << "        Y: " << res.aligned_y << "\n";
  }

  // Test 2: Classic "kitten" -> "sitting" (distance 3)
  // replace k->s, replace e->i, insert g
  {
    const std::string x = "kitten";
    const std::string y = "sitting";

    assert(dsa::compute_edit_distance(x, y) == 3);
    auto res = dsa::compute_edit_distance_with_script(x, y);
    assert(res.distance == 3);
    assert(res.verify_transformation(x, y));
  }

  // Test 3: Classic "sunday" -> "saturday" (distance 3)
  {
    const std::string x = "sunday";
    const std::string y = "saturday";

    assert(dsa::compute_edit_distance(x, y) == 3);
    auto res = dsa::compute_edit_distance_with_script(x, y);
    assert(res.distance == 3);
    assert(res.verify_transformation(x, y));
  }

  // Test 4: Completely disjoint strings ("abc" -> "def")
  {
    assert(dsa::compute_edit_distance("abc", "def") == 3);
    auto res = dsa::compute_edit_distance_with_script("abc", "def");
    assert(res.distance == 3);
    assert(res.verify_transformation("abc", "def"));
  }

  // Test 5: Symmetry property: dist(X, Y) == dist(Y, X)
  {
    const std::string x = "intention";
    const std::string y = "execution";
    const std::size_t d1 = dsa::compute_edit_distance(x, y);
    const std::size_t d2 = dsa::compute_edit_distance(y, x);
    assert(d1 == d2);
    assert(d1 == 5);

    auto res_xy = dsa::compute_edit_distance_with_script(x, y);
    auto res_yx = dsa::compute_edit_distance_with_script(y, x);
    assert(res_xy.distance == 5);
    assert(res_yx.distance == 5);
    assert(res_xy.verify_transformation(x, y));
    assert(res_yx.verify_transformation(y, x));
  }

  std::cout << "    [+] Textbook and canonical cases passed.\n";
}

void test_oracle_brute_force_equivalence() {
  std::cout << "[*] Verifying against recursive brute-force oracle across "
               "randomized datasets...\n";

  std::mt19937_64 rng(999);
  std::uniform_int_distribution<int> char_dist('a', 'd');
  std::uniform_int_distribution<std::size_t> len_dist(0, 7);

  for (int iter = 0; iter < 400; ++iter) {
    const std::size_t n = len_dist(rng);
    const std::size_t m = len_dist(rng);

    std::string x(n, ' ');
    for (auto &c : x)
      c = static_cast<char>(char_dist(rng));

    std::string y(m, ' ');
    for (auto &c : y)
      c = static_cast<char>(char_dist(rng));

    const std::size_t oracle_dist = brute_force_oracle(x, y);
    const std::size_t dp_dist = dsa::compute_edit_distance(x, y);
    assert(dp_dist == oracle_dist);

    auto script_res = dsa::compute_edit_distance_with_script(x, y);
    assert(script_res.distance == oracle_dist);
    assert(script_res.verify_transformation(x, y));
  }

  std::cout << "    [+] Brute-force equivalence verified across 400 randomized "
               "test cases.\n";
}

void test_stress_and_performance() {
  std::cout << "[*] Running stress and performance benchmark...\n";

  // Matrix size: 3000 x 3000 = 9,000,000 subproblem evaluations
  const std::size_t n = 3000;
  const std::size_t m = 3000;

  std::mt19937_64 rng(42);
  std::uniform_int_distribution<int> char_dist('a', 'z');

  std::string x(n, ' ');
  for (auto &c : x)
    c = static_cast<char>(char_dist(rng));

  std::string y(m, ' ');
  for (auto &c : y)
    c = static_cast<char>(char_dist(rng));

  // Benchmark space-optimized O(min(n, m)) computation
  const auto start = std::chrono::high_resolution_clock::now();
  const std::size_t dist = dsa::compute_edit_distance(x, y);
  const auto end = std::chrono::high_resolution_clock::now();

  const auto duration_ms =
      std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
          end - start)
          .count();

  assert(dist > 0 && dist <= std::max(n, m));

  std::cout << "    -> |X| = " << n << ", |Y| = " << m
            << " | Total Cells: 9M | Edit Distance = " << dist
            << " | Elapsed Time = " << std::fixed << std::setprecision(2)
            << duration_ms << " ms\n";

  std::cout << "    [+] Stress and benchmark tests completed successfully.\n";
}

} // namespace

int main() {
  std::cout << "========================================================\n";
  std::cout << "       C-12.20: Levenshtein Edit Distance O(nm)         \n";
  std::cout << "========================================================\n\n";

  test_edge_cases();
  test_textbook_and_canonical_cases();
  test_oracle_brute_force_equivalence();
  test_stress_and_performance();

  std::cout
      << "\n[SUCCESS] All unit tests, assertions, and benchmarks passed!\n";
  return 0;
}
