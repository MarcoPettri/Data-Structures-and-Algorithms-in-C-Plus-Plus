// File : C-14.1.cpp

/*
    Describe an efficient external-memory algorithm for removing all the du-
    plicate entries in a vector of size n.
*/

#include "external_dedup.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
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

template <typename T, typename Compare = std::less<T>>
std::vector<T> reference_dedup(std::vector<T> v, Compare comp = Compare{}) {
  std::sort(v.begin(), v.end(), comp);
  auto it = std::unique(v.begin(), v.end(), [&comp](const T &a, const T &b) {
    return !comp(a, b) && !comp(b, a);
  });
  v.erase(it, v.end());
  return v;
}

void testEmpty() {
  ds::ExternalVector<int> ev(4);
  ds::ExternalMemoryStats stats;
  auto out = ds::external_deduplicate(ev, 12, &stats);
  CHECK(out.empty());
  CHECK(out.size() == 0);
  CHECK(stats.total_ios() == 0);
}

void testSingleElement() {
  ds::ExternalVector<int> ev(std::vector<int>{42}, 4);
  ds::ExternalMemoryStats stats;
  auto out = ds::external_deduplicate(ev, 12, &stats);
  CHECK(out.to_vector() == std::vector<int>{42});
  CHECK(stats.initial_runs == 1);
  CHECK(stats.merge_passes == 0);
}

void testAllDuplicates() {
  std::vector<int> data(100, 7);
  ds::ExternalVector<int> ev(data, 4);
  ds::ExternalMemoryStats stats;
  auto out = ds::external_deduplicate(ev, 12, &stats);
  CHECK(out.to_vector() == std::vector<int>{7});
  CHECK(out.size() == 1);
}

void testAllUnique() {
  std::vector<int> data;
  for (int i = 50; i >= 1; --i)
    data.push_back(i);

  ds::ExternalVector<int> ev(data, 4);
  ds::ExternalMemoryStats stats;
  auto out = ds::external_deduplicate(ev, 12, &stats);
  CHECK(out.to_vector() == reference_dedup(data));
  CHECK(out.size() == 50);
  CHECK(stats.merge_passes > 0);
}

void testFitsInMemory() {
  std::vector<int> data{5, 1, 5, 2, 3, 2, 4, 1, 3, 5};
  ds::ExternalVector<int> ev(data, 4);
  ds::ExternalMemoryStats stats;
  auto out = ds::external_deduplicate(ev, 16, &stats);
  CHECK(out.to_vector() == (std::vector<int>{1, 2, 3, 4, 5}));
  CHECK(stats.initial_runs == 1);
  CHECK(stats.merge_passes == 0);
}

void testInvalidArguments() {
  bool thrown = false;
  try {
    ds::ExternalVector<int> bad(0);
  } catch (const std::invalid_argument &) {
    thrown = true;
  }
  CHECK(thrown);

  thrown = false;
  try {
    ds::ExternalVector<int> ev(std::vector<int>{1, 2, 3}, 4);
    (void)ds::external_deduplicate(ev, 8); // M < 3*B (8 < 12)
  } catch (const std::invalid_argument &) {
    thrown = true;
  }
  CHECK(thrown);
}

void testStringsAndCustomComparator() {
  std::vector<std::string> words{"pear",  "apple", "fig",
                                 "apple", "pear",  "kiwi"};
  ds::ExternalVector<std::string> ev(words, 2);
  auto out =
      ds::external_deduplicate(ev, 6, nullptr, std::greater<std::string>{});
  auto expected = reference_dedup(words, std::greater<std::string>{});
  CHECK(out.to_vector() == expected);
}

void testRandomizedAgainstReference() {
  constexpr int N = 5000;
  constexpr std::size_t B = 16;
  constexpr std::size_t M = 64; // k = 64/16 - 1 = 3-way merge
  std::mt19937 rng(12345);
  std::uniform_int_distribution<int> dist(0, 499);

  std::vector<int> data(N);
  for (int &x : data)
    x = dist(rng);

  ds::ExternalVector<int> ev(data, B);
  ds::ExternalMemoryStats stats;
  auto out = ds::external_deduplicate(ev, M, &stats);

  CHECK(out.to_vector() == reference_dedup(data));
  CHECK(stats.initial_runs == (N + M - 1) / M);
  CHECK(stats.merge_passes > 0);
}

void testPerformanceAndIOBound() {
  // N = 100,000, B = 64, M = 1024 (16 blocks -> k = 15-way merge)
  constexpr std::size_t N = 100'000;
  constexpr std::size_t B = 64;
  constexpr std::size_t M = 1024;
  std::mt19937 rng(99);
  std::uniform_int_distribution<int> dist(0, 20'000);

  std::vector<int> data(N);
  for (std::size_t i = 0; i < N; ++i)
    data[i] = dist(rng);

  ds::ExternalVector<int> ev(data, B);
  ds::ExternalMemoryStats stats;
  auto out = ds::external_deduplicate(ev, M, &stats);

  auto got = out.to_vector();
  CHECK(std::is_sorted(got.begin(), got.end()));
  CHECK(std::adjacent_find(got.begin(), got.end()) == got.end());
  CHECK(got.size() == std::set<int>(data.begin(), data.end()).size());

  // Verify I/O count matches O((N/B) * (1 + log_k(N/M)))
  const std::size_t blocks = (N + B - 1) / B;
  // Phase 1: <= 2 * blocks; Phase 2: merge_passes * 2 * blocks
  const std::size_t max_expected_ios = 2 * blocks * (1 + stats.merge_passes);
  CHECK(stats.total_ios() <= max_expected_ios);
  CHECK(stats.merge_passes <=
        2); // ceil(log_15(100000/1024)) = ceil(log_15(98)) = 2
}

} // namespace

int main() {
  RUN(testEmpty);
  RUN(testSingleElement);
  RUN(testAllDuplicates);
  RUN(testAllUnique);
  RUN(testFitsInMemory);
  RUN(testInvalidArguments);
  RUN(testStringsAndCustomComparator);
  RUN(testRandomizedAgainstReference);
  RUN(testPerformanceAndIOBound);

  std::cout << '\n'
            << (g_checks - g_failures) << '/' << g_checks << " checks passed\n";
  return g_failures;
}
