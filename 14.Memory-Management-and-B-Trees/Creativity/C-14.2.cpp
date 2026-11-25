// File : C-14.2.cpp

/*
    Show how to implement a map in external memory using an unordered se-
    quence so that insertions require only O(1) transfers and searches require
    O(n/B) transfers in the worst case, where n is the number of elements and
    B is the number of list nodes that can fit into a disk block.
*/

#include "external_unordered_map.hpp"

#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_map>

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

void testEmpty() {
  ds::ExternalUnorderedMap<int, std::string> m(4);
  ds::UnorderedMapIOStats stats;
  CHECK(m.empty());
  CHECK(m.record_count() == 0);
  CHECK(!m.contains(10, &stats));
  CHECK(!m.find(10, &stats).has_value());
  CHECK(!m.erase(10, &stats));
  CHECK(stats.total_ios() == 0);
}

void testInvalidBlockSize() {
  bool thrown = false;
  try {
    ds::ExternalUnorderedMap<int, int> bad(0);
  } catch (const std::invalid_argument &) {
    thrown = true;
  }
  CHECK(thrown);
}

void testSingleAndTailBuffer() {
  ds::ExternalUnorderedMap<int, std::string> m(4);
  ds::UnorderedMapIOStats stats;

  m.put(1, "one", &stats);
  m.put(2, "two", &stats);
  m.put(3, "three", &stats);
  // All 3 fit in the internal tail buffer -> 0 disk writes so far.
  CHECK(stats.total_ios() == 0);
  CHECK(m.record_count() == 3);

  auto v = m.find(2, &stats);
  CHECK(v.has_value() && *v == "two");
  CHECK(stats.total_ios() == 0); // Found in tail buffer!

  // 4th insert fills block of size 4 and triggers 1 flush write.
  m.put(4, "four", &stats);
  CHECK(stats.block_writes == 1);
  CHECK(m.num_disk_blocks() == 1);
}

void testOverwriteNewestWins() {
  ds::ExternalUnorderedMap<int, std::string> m(3);
  m.put(10, "v1");
  m.put(20, "a");
  m.put(30, "b"); // flushes block 0
  m.put(10, "v2");
  m.put(40, "c");
  m.put(10, "v3"); // flushes block 1

  ds::UnorderedMapIOStats stats;
  auto v = m.find(10, &stats);
  CHECK(v.has_value() && *v == "v3");
  // Found in the newest disk block (1 read) without scanning block 0.
  CHECK(stats.block_reads == 1);
}

void testInsertO1AndSearchOBound() {
  constexpr std::size_t B = 8;
  constexpr int N = 1000;
  ds::ExternalUnorderedMap<int, int> m(B);

  // Every single insertion must take <= 1 block transfer (O(1) worst-case).
  for (int i = 0; i < N; ++i) {
    ds::UnorderedMapIOStats stats;
    m.put(i, i * 10, &stats);
    CHECK(stats.total_ios() <= 1);
    CHECK(stats.block_reads == 0);
  }
  CHECK(m.record_count() == static_cast<std::size_t>(N));

  // Worst-case search (absent key) reads at most ceil(n / B) blocks.
  ds::UnorderedMapIOStats search_stats;
  CHECK(!m.find(-999, &search_stats).has_value());
  CHECK(search_stats.block_reads == m.num_disk_blocks());
  CHECK(search_stats.block_reads <= (N + B - 1) / B);
  CHECK(search_stats.block_writes == 0);
}

void testEraseSingleAndDuplicateRecords() {
  ds::ExternalUnorderedMap<int, std::string> m(3);
  m.put(1, "old1");
  m.put(2, "two");
  m.put(3, "three");
  m.put(1, "new1");
  m.put(4, "four");
  m.put(1, "newest1");
  m.put(5, "five");

  CHECK(m.erase(1));
  CHECK(!m.contains(1));
  CHECK(!m.erase(1)); // already gone
  CHECK(m.record_count() == 4);
  CHECK(m.find(2) == "two");
  CHECK(m.find(3) == "three");
  CHECK(m.find(4) == "four");
  CHECK(m.find(5) == "five");

  // Erase remaining keys until empty.
  CHECK(m.erase(2));
  CHECK(m.erase(3));
  CHECK(m.erase(4));
  CHECK(m.erase(5));
  CHECK(m.empty());
  CHECK(m.num_disk_blocks() == 0);
}

void testRandomizedAgainstStdUnorderedMap() {
  constexpr std::size_t B = 5;
  constexpr int OPS = 2000;
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> key_dist(0, 99);
  std::uniform_int_distribution<int> op_dist(0, 99);

  ds::ExternalUnorderedMap<int, int> m(B);
  std::unordered_map<int, int> ref;

  for (int step = 0; step < OPS; ++step) {
    int k = key_dist(rng);
    int op = op_dist(rng);

    if (op < 55) {
      ds::UnorderedMapIOStats stats;
      m.put(k, step, &stats);
      CHECK(stats.total_ios() <= 1);
      ref[k] = step;
    } else if (op < 80) {
      auto got = m.find(k);
      auto it = ref.find(k);
      CHECK(got.has_value() == (it != ref.end()));
      if (it != ref.end())
        CHECK(*got == it->second);
    } else {
      bool removed = m.erase(k);
      bool ref_removed = ref.erase(k) > 0;
      CHECK(removed == ref_removed);
    }
  }

  for (int k = 0; k < 100; ++k) {
    auto got = m.find(k);
    auto it = ref.find(k);
    CHECK(got.has_value() == (it != ref.end()));
    if (it != ref.end())
      CHECK(*got == it->second);
  }
}

void testPerformanceSanity() {
  constexpr std::size_t B = 64;
  constexpr int N = 50'000;
  ds::ExternalUnorderedMap<int, int> m(B);
  ds::UnorderedMapIOStats insert_stats;

  for (int i = 0; i < N; ++i)
    m.put(i, i, &insert_stats);

  // Total I/Os for N inserts <= ceil(N / B)
  CHECK(insert_stats.block_writes <= (N + B - 1) / B);
  CHECK(insert_stats.block_reads == 0);

  ds::UnorderedMapIOStats search_stats;
  CHECK(m.find(0, &search_stats) == 0); // oldest element -> worst-case scan
  CHECK(search_stats.block_reads <= (N + B - 1) / B);
}

} // namespace

int main() {
  RUN(testEmpty);
  RUN(testInvalidBlockSize);
  RUN(testSingleAndTailBuffer);
  RUN(testOverwriteNewestWins);
  RUN(testInsertO1AndSearchOBound);
  RUN(testEraseSingleAndDuplicateRecords);
  RUN(testRandomizedAgainstStdUnorderedMap);
  RUN(testPerformanceSanity);

  std::cout << '\n'
            << (g_checks - g_failures) << '/' << g_checks << " checks passed\n";
  return g_failures;
}
