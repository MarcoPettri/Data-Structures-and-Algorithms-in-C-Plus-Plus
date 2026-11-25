// File : C-14.13.cpp

/*
    Describe an efficient external-memory algorithm that determines whether
    an array of n integers contains a value occurring more than n/2 times.
*/

#include "external_majority.hpp"

#include <iostream>
#include <limits>
#include <optional>
#include <random>
#include <stdexcept>
#include <unordered_map>
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

std::optional<int> reference_majority(const std::vector<int> &v) {
  std::unordered_map<int, std::size_t> counts;
  for (int x : v)
    ++counts[x];
  for (const auto &[val, cnt] : counts) {
    if (cnt > v.size() / 2)
      return val;
  }
  return std::nullopt;
}

void testEmptyAndInvalid() {
  ds::ExternalArray<int> empty_arr({}, 4);
  ds::MajorityIOStats stats;
  CHECK(!ds::find_majority_element(empty_arr, &stats).has_value());
  CHECK(stats.total_ios() == 0);

  bool thrown = false;
  try {
    ds::ExternalArray<int> bad({1, 2}, 0);
  } catch (const std::invalid_argument &) {
    thrown = true;
  }
  CHECK(thrown);
}

void testSingleAndTwoElements() {
  ds::ExternalArray<int> one({42}, 4);
  CHECK(ds::find_majority_element(one) == 42);

  ds::ExternalArray<int> two_eq({7, 7}, 4);
  CHECK(ds::find_majority_element(two_eq) == 7);

  ds::ExternalArray<int> two_diff({7, 8}, 4);
  CHECK(!ds::find_majority_element(two_diff).has_value());
}

void testBoundaryHalfVsStrictMajority() {
  // n = 6: 3 occurrences is exactly n/2 -> NOT strictly > n/2.
  ds::ExternalArray<int> exact_half({1, 2, 1, 3, 1, 4}, 2);
  CHECK(!ds::find_majority_element(exact_half).has_value());

  // n = 6: 4 occurrences > 3 -> IS majority.
  ds::ExternalArray<int> strict_even({1, 2, 1, 3, 1, 1}, 2);
  CHECK(ds::find_majority_element(strict_even) == 1);

  // n = 7: 3 occurrences < 7/2 -> NOT majority; 4 occurrences -> IS majority.
  ds::ExternalArray<int> odd_no({5, 5, 5, 1, 2, 3, 4}, 3);
  CHECK(!ds::find_majority_element(odd_no).has_value());

  ds::ExternalArray<int> odd_yes({1, 2, 3, 5, 5, 5, 5}, 3);
  CHECK(ds::find_majority_element(odd_yes) == 5);
}

void testExtremeValues() {
  const int lo = std::numeric_limits<int>::min();
  const int hi = std::numeric_limits<int>::max();
  ds::ExternalArray<int> arr({lo, hi, lo, 0, lo}, 2);
  CHECK(ds::find_majority_element(arr) == lo);
}

void testExactIOBound() {
  constexpr std::size_t B = 8;
  constexpr std::size_t N = 1000;
  std::vector<int> data(N, 99);
  for (std::size_t i = 0; i < 400; ++i)
    data[i] = static_cast<int>(i);

  ds::ExternalArray<int> arr(data, B);
  ds::MajorityIOStats stats;
  auto res = ds::find_majority_element(arr, &stats);
  CHECK(res == 99);
  // Strictly 2 * ceil(N / B) reads and 0 writes.
  CHECK(stats.block_reads == 2 * ((N + B - 1) / B));
  CHECK(stats.block_writes == 0);
}

void testRandomizedAgainstReference() {
  constexpr std::size_t B = 7;
  std::mt19937 rng(42);

  for (int trial = 0; trial < 200; ++trial) {
    std::uniform_int_distribution<std::size_t> n_dist(1, 200);
    const std::size_t n = n_dist(rng);
    std::vector<int> data(n);

    if (trial % 2 == 0) {
      // Plant a majority element with probability 1/2.
      std::uniform_int_distribution<int> val_dist(-50, 50);
      int maj = val_dist(rng);
      std::size_t maj_cnt = n / 2 + 1;
      for (std::size_t i = 0; i < maj_cnt; ++i)
        data[i] = maj;
      for (std::size_t i = maj_cnt; i < n; ++i)
        data[i] = val_dist(rng);
      std::shuffle(data.begin(), data.end(), rng);
    } else {
      std::uniform_int_distribution<int> val_dist(1, 5);
      for (std::size_t i = 0; i < n; ++i)
        data[i] = val_dist(rng);
    }

    ds::ExternalArray<int> arr(data, B);
    CHECK(ds::find_majority_element(arr) == reference_majority(data));
  }
}

void testPerformanceSanity() {
  constexpr std::size_t N = 200'000;
  constexpr std::size_t B = 64;
  std::vector<int> data(N, 7);
  for (std::size_t i = 0; i < N / 2 - 1; ++i)
    data[i * 2] = static_cast<int>(i);

  ds::ExternalArray<int> arr(data, B);
  ds::MajorityIOStats stats;
  CHECK(ds::find_majority_element(arr, &stats) == 7);
  CHECK(stats.total_ios() == 2 * ((N + B - 1) / B));
}

} // namespace

int main() {
  RUN(testEmptyAndInvalid);
  RUN(testSingleAndTwoElements);
  RUN(testBoundaryHalfVsStrictMajority);
  RUN(testExtremeValues);
  RUN(testExactIOBound);
  RUN(testRandomizedAgainstReference);
  RUN(testPerformanceSanity);

  std::cout << '\n'
            << (g_checks - g_failures) << '/' << g_checks << " checks passed\n";
  return g_failures;
}
