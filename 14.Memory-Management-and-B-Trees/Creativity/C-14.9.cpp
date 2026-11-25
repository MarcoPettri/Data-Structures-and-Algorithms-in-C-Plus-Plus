// File : C-14.9.cpp

/*
    Suppose we are given a sequence S of n elements with integer keys such
    that some elements in S are colored “blue” and some elements in S are
    colored “red.” In addition, say that a red element e pairs with a blue
    element f if they have the same key value. Describe an efficient external-
    memory algorithm for finding all the red-blue pairs in S. How many disk
    transfers does your algorithm perform?
*/

#include "red_blue_join.hpp"

#include <algorithm>
#include <iostream>
#include <random>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace {

int g_failures = 0;
int g_checks = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    ++g_checks;                                                                \
    if (!(cond)) {                                                             \
      ++g_failures;                                                            \
      std::cerr << __FILE__ << ":" << __LINE__ << " CHECK failed: " #cond      \
                << '\n';                                                       \
    }                                                                          \
  } while (0)

#define RUN(test)                                                              \
  do {                                                                         \
    const int before = g_failures;                                             \
    test();                                                                    \
    std::cout << (g_failures == before ? "[ OK ] " : "[FAIL] ") << #test       \
              << '\n';                                                         \
  } while (0)

using KeyPair =
    std::tuple<int, std::size_t, std::size_t>; // (key, red_id, blue_id)

std::vector<KeyPair> canonicalize(const std::vector<ds::RedBluePair> &pairs) {
  std::vector<KeyPair> out;
  out.reserve(pairs.size());
  for (const auto &p : pairs) {
    out.emplace_back(p.first.key, p.first.id, p.second.id);
  }
  std::sort(out.begin(), out.end());
  return out;
}

std::vector<KeyPair>
brute_force_pairs(const std::vector<ds::ColoredElement> &elems) {
  std::vector<KeyPair> out;
  for (const auto &r : elems) {
    if (r.color != ds::Color::Red)
      continue;
    for (const auto &b : elems) {
      if (b.color == ds::Color::Blue && r.key == b.key)
        out.emplace_back(r.key, r.id, b.id);
    }
  }
  std::sort(out.begin(), out.end());
  return out;
}

void testEmptyAndInvalid() {
  ds::BlockSequence<ds::ColoredElement> empty_seq(4);
  ds::RedBlueIOStats stats;
  auto res = ds::find_red_blue_pairs(empty_seq, 12, &stats);
  CHECK(res.empty());
  CHECK(stats.total_ios() == 0);

  bool thrown = false;
  try {
    (void)ds::find_red_blue_pairs(empty_seq, 8); // M < 3*B
  } catch (const std::invalid_argument &) {
    thrown = true;
  }
  CHECK(thrown);
}

void testMonochromeAndDisjoint() {
  std::vector<ds::ColoredElement> only_red{
      {1, ds::Color::Red, 0}, {2, ds::Color::Red, 1}, {3, ds::Color::Red, 2}};
  ds::BlockSequence<ds::ColoredElement> seq1(only_red, 2);
  CHECK(ds::find_red_blue_pairs(seq1, 6).empty());

  std::vector<ds::ColoredElement> disjoint{{1, ds::Color::Red, 0},
                                           {2, ds::Color::Blue, 1},
                                           {3, ds::Color::Red, 2},
                                           {4, ds::Color::Blue, 3}};
  ds::BlockSequence<ds::ColoredElement> seq2(disjoint, 2);
  CHECK(ds::find_red_blue_pairs(seq2, 6).empty());
}

void testMultiplePairsSameKey() {
  // 3 Red(10) x 2 Blue(10) = 6 pairs, plus 1 Red(5) x 1 Blue(5) = 1 pair.
  std::vector<ds::ColoredElement> elems{
      {10, ds::Color::Blue, 100}, {5, ds::Color::Red, 1},
      {10, ds::Color::Red, 2},    {10, ds::Color::Red, 3},
      {5, ds::Color::Blue, 101},  {10, ds::Color::Blue, 102},
      {10, ds::Color::Red, 4},    {99, ds::Color::Blue, 103}};

  ds::BlockSequence<ds::ColoredElement> seq(elems, 3);
  auto got = canonicalize(ds::find_red_blue_pairs(seq, 9).to_vector());
  auto expected = brute_force_pairs(elems);
  CHECK(got == expected);
  CHECK(got.size() == 7);
}

void testLargeGroupExceedingMemory() {
  // B = 4, M = 12 => red_cap = M - 2B = 4.
  // Put 11 Red(7) and 9 Blue(7) so |Red(7)| > red_cap (exercises chunked
  // block-nested loop across multiple Red chunks).
  std::vector<ds::ColoredElement> elems;
  for (std::size_t i = 0; i < 11; ++i)
    elems.push_back({7, ds::Color::Red, i});
  for (std::size_t i = 0; i < 9; ++i)
    elems.push_back({7, ds::Color::Blue, 100 + i});
  elems.push_back({1, ds::Color::Red, 200});
  elems.push_back({1, ds::Color::Blue, 201});
  elems.push_back({20, ds::Color::Red, 202});
  elems.push_back({20, ds::Color::Blue, 203});

  std::mt19937 rng(77);
  std::shuffle(elems.begin(), elems.end(), rng);

  ds::BlockSequence<ds::ColoredElement> seq(elems, 4);
  auto got = canonicalize(ds::find_red_blue_pairs(seq, 12).to_vector());
  auto expected = brute_force_pairs(elems);
  CHECK(got == expected);
  CHECK(got.size() == 11 * 9 + 2);
}

void testRandomizedAgainstBruteForce() {
  constexpr std::size_t N = 600;
  constexpr std::size_t B = 8;
  constexpr std::size_t M = 32; // k = 3-way merge
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> key_dist(1, 80);
  std::uniform_int_distribution<int> col_dist(0, 1);

  std::vector<ds::ColoredElement> elems;
  elems.reserve(N);
  for (std::size_t i = 0; i < N; ++i) {
    elems.push_back({key_dist(rng),
                     col_dist(rng) == 0 ? ds::Color::Red : ds::Color::Blue, i});
  }

  ds::BlockSequence<ds::ColoredElement> seq(elems, B);
  ds::RedBlueIOStats stats;
  auto res = ds::find_red_blue_pairs(seq, M, &stats);

  CHECK(canonicalize(res.to_vector()) == brute_force_pairs(elems));
  CHECK(stats.merge_passes > 0);
}

void testPerformanceAndIOBound() {
  // N = 50,000, B = 64, M = 1024 (k = 15-way merge), keys in [1, 25000]
  constexpr std::size_t N = 50'000;
  constexpr std::size_t B = 64;
  constexpr std::size_t M = 1024;
  std::mt19937 rng(99);
  std::uniform_int_distribution<int> key_dist(1, 25'000);
  std::uniform_int_distribution<int> col_dist(0, 1);

  std::vector<ds::ColoredElement> elems;
  elems.reserve(N);
  for (std::size_t i = 0; i < N; ++i) {
    elems.push_back({key_dist(rng),
                     col_dist(rng) == 0 ? ds::Color::Red : ds::Color::Blue, i});
  }

  ds::BlockSequence<ds::ColoredElement> seq(elems, B);
  ds::RedBlueIOStats stats;
  auto res = ds::find_red_blue_pairs(seq, M, &stats);

  // Verify every emitted pair has matching key and Red/Blue colors.
  auto pairs = res.to_vector();
  for (const auto &p : pairs) {
    CHECK(p.first.color == ds::Color::Red);
    CHECK(p.second.color == ds::Color::Blue);
    CHECK(p.first.key == p.second.key);
  }
  CHECK(stats.merge_passes <= 2);
}

} // namespace

int main() {
  RUN(testEmptyAndInvalid);
  RUN(testMonochromeAndDisjoint);
  RUN(testMultiplePairsSameKey);
  RUN(testLargeGroupExceedingMemory);
  RUN(testRandomizedAgainstBruteForce);
  RUN(testPerformanceAndIOBound);

  std::cout << '\n'
            << (g_checks - g_failures) << '/' << g_checks << " checks passed\n";
  return g_failures;
}
