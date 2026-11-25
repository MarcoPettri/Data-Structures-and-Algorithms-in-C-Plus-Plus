// File : C-14.10.cpp

/*
    Consider the page caching problem where the memory cache can hold m
    pages, and we are given a sequence P of n requests taken from a pool
    of m + 1 possible pages. Describe the optimal strategy for the offline
    algorithm and show that it causes at most m + n/m page misses in total,
    starting from an empty cache.
*/

#include "optimal_offline_cache.hpp"

#include <deque>
#include <iostream>
#include <random>
#include <stdexcept>
#include <unordered_set>
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

// Reference FIFO simulator to verify Belady's optimality.
std::size_t simulate_fifo(const std::vector<int> &requests, std::size_t m) {
  std::unordered_set<int> in_cache;
  std::deque<int> order;
  std::size_t misses = 0;
  for (int p : requests) {
    if (in_cache.count(p))
      continue;
    ++misses;
    if (in_cache.size() == m) {
      in_cache.erase(order.front());
      order.pop_front();
    }
    in_cache.insert(p);
    order.push_back(p);
  }
  return misses;
}

void testEmptyAndInvalid() {
  auto r = ds::simulate_belady_optimal({}, 4);
  CHECK(r.misses == 0);
  CHECK(r.hits == 0);
  CHECK(r.evicted_pages.empty());

  bool thrown = false;
  try {
    (void)ds::simulate_belady_optimal({1, 2}, 0);
  } catch (const std::invalid_argument &) {
    thrown = true;
  }
  CHECK(thrown);
}

void testWarmupOnly() {
  // Pool of m+1=5 pages, cache m=4, only 4 distinct pages requested.
  std::vector<int> reqs{0, 1, 2, 3, 0, 1, 2, 3};
  auto r = ds::simulate_belady_optimal(reqs, 4);
  CHECK(r.misses == 4);
  CHECK(r.hits == 4);
  CHECK(r.evicted_pages.empty());
}

void testSingleCapacityCache() {
  // m = 1, pool = {0, 1}. Bound: 1 + n/1 = n + 1.
  std::vector<int> reqs{0, 1, 0, 1, 0, 1};
  auto r = ds::simulate_belady_optimal(reqs, 1);
  CHECK(r.misses == 6);
  CHECK(r.misses <= 1 + reqs.size() / 1);
}

void testCyclicWorstCaseForPoolMPlus1() {
  // Cyclic sequence 0, 1, ..., m, 0, 1, ..., m, ... is the classic worst case
  // for FIFO/LRU (100% miss rate = n misses), whereas Belady's optimal achieves
  // at most m + floor(n / m) misses.
  constexpr std::size_t m = 5;
  constexpr std::size_t n = 1000;
  std::vector<int> reqs(n);
  for (std::size_t i = 0; i < n; ++i)
    reqs[i] = static_cast<int>(i % (m + 1));

  auto opt = ds::simulate_belady_optimal(reqs, m);
  std::size_t fifo_misses = simulate_fifo(reqs, m);

  CHECK(fifo_misses == n); // FIFO misses on every single request!
  CHECK(opt.misses <= m + n / m);
  CHECK(opt.misses + opt.hits == n);
}

void testRandomizedPoolMPlus1BoundAndOptimality() {
  std::mt19937 rng(42);
  for (std::size_t m : {2u, 4u, 8u, 16u}) {
    std::uniform_int_distribution<int> page_dist(0, static_cast<int>(m));
    for (int trial = 0; trial < 50; ++trial) {
      const std::size_t n = 500;
      std::vector<int> reqs(n);
      for (std::size_t i = 0; i < n; ++i)
        reqs[i] = page_dist(rng);

      auto opt = ds::simulate_belady_optimal(reqs, m);
      std::size_t fifo_misses = simulate_fifo(reqs, m);

      CHECK(opt.misses <= m + n / m);
      CHECK(opt.misses <= fifo_misses);
      CHECK(opt.misses + opt.hits == n);
    }
  }
}

void testPerformanceSanity() {
  constexpr std::size_t m = 64;
  constexpr std::size_t n = 200'000;
  std::mt19937 rng(99);
  std::uniform_int_distribution<int> page_dist(0, static_cast<int>(m));

  std::vector<int> reqs(n);
  for (std::size_t i = 0; i < n; ++i)
    reqs[i] = page_dist(rng);

  auto opt = ds::simulate_belady_optimal(reqs, m);
  CHECK(opt.misses <= m + n / m);
  CHECK(opt.misses + opt.hits == n);
}

} // namespace

int main() {
  RUN(testEmptyAndInvalid);
  RUN(testWarmupOnly);
  RUN(testSingleCapacityCache);
  RUN(testCyclicWorstCaseForPoolMPlus1);
  RUN(testRandomizedPoolMPlus1BoundAndOptimality);
  RUN(testPerformanceSanity);

  std::cout << '\n'
            << (g_checks - g_failures) << '/' << g_checks << " checks passed\n";
  return g_failures;
}
