/**
 * @file ab_tree_test.cpp
 * @brief Self-contained unit and stress tests for ds::ABTree (no external
 *        framework). Included by R-14.2.cpp; do not compile directly.
 */

#include "ab_tree.hpp"

#include <algorithm>
#include <iostream>
#include <numeric>
#include <random>
#include <set>
#include <string>
#include <vector>

namespace ABTree_Test {

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

// ---- Type aliases used throughout the tests --------------------------------
using Tree24 = ds::ABTree<int, 2, 4>; ///< classic 2-3-4 tree
using Tree35 = ds::ABTree<int, 3, 5>;

// ---- Helper: assert sorted contents match a reference vector ---------------
template <typename Tree>
void checkContents(const Tree &t, std::vector<int> expected,
                   const char *label) {
  auto got = t.to_sorted_vector();
  if (got != expected) {
    ++g_failures;
    std::cerr << "checkContents [" << label << "] mismatch\n";
  }
  ++g_checks;
}

// ============================================================================
// Test functions
// ============================================================================

void testEmpty() {
  Tree24 t;
  CHECK(t.empty());
  CHECK(t.size() == 0);
  CHECK(t.height() == 0);
  CHECK(!t.contains(42));
  CHECK(t.validate());
  CHECK(t.to_sorted_vector().empty());
}

void testSingleInsertAndDuplicate() {
  Tree24 t;
  CHECK(t.insert(10));
  CHECK(!t.insert(10)); // duplicate must be rejected
  CHECK(t.size() == 1);
  CHECK(t.contains(10));
  CHECK(!t.contains(0));
  CHECK(t.validate());
}

void testInsertNoSplit() {
  // Up to b-1 = 3 keys fit in a single leaf without splitting.
  Tree24 t;
  for (int k : {5, 3, 7})
    t.insert(k);
  CHECK(t.size() == 3);
  CHECK(t.height() == 0);
  CHECK(t.validate());
  checkContents(t, {3, 5, 7}, "no_split");
}

void testFirstRootSplit() {
  // The 4th insertion forces the first root split in a (2,4)-tree.
  Tree24 t;
  for (int k : {1, 2, 3, 4})
    t.insert(k);
  CHECK(t.size() == 4);
  CHECK(t.height() == 1);
  CHECK(t.validate());
  checkContents(t, {1, 2, 3, 4}, "first_split");
}

void testSequentialAscendingInsert() {
  Tree24 t;
  constexpr int N = 100;
  for (int i = 1; i <= N; ++i)
    t.insert(i);
  CHECK(t.size() == N);
  CHECK(t.validate());
  auto v = t.to_sorted_vector();
  for (int i = 0; i < N; ++i)
    CHECK(v[i] == i + 1);
}

void testSequentialDescendingInsert() {
  Tree24 t;
  constexpr int N = 100;
  for (int i = N; i >= 1; --i)
    t.insert(i);
  CHECK(t.size() == N);
  CHECK(t.validate());
  auto v = t.to_sorted_vector();
  for (int i = 0; i < N; ++i)
    CHECK(v[i] == i + 1);
}

void testEraseFromSingleElementTree() {
  Tree24 t;
  t.insert(42);
  CHECK(t.erase(42));
  CHECK(t.empty());
  CHECK(!t.contains(42));
  CHECK(t.validate());
}

void testEraseAbsentKey() {
  Tree24 t;
  for (int k : {1, 2, 3})
    t.insert(k);
  CHECK(!t.erase(99));
  CHECK(t.size() == 3);
  CHECK(t.validate());
}

void testEraseCausingMerge() {
  // Removing all elements forces repeated merges and eventual root collapse.
  Tree24 t;
  for (int k : {1, 2, 3, 4, 5})
    t.insert(k);
  CHECK(t.validate());
  for (int k : {1, 2, 3, 4, 5}) {
    CHECK(t.erase(k));
    CHECK(t.validate());
  }
  CHECK(t.empty());
}

void testEraseInternalNodeKey() {
  // Erasing a key stored at an internal node exercises the successor-swap path.
  Tree24 t;
  for (int k : {10, 20, 30, 40, 50, 60, 70, 80})
    t.insert(k);
  CHECK(t.validate());
  CHECK(t.erase(40));
  CHECK(t.validate());
  CHECK(!t.contains(40));
  CHECK(t.size() == 7);
}

void testEraseAllSequential() {
  Tree24 t;
  constexpr int N = 50;
  for (int i = 1; i <= N; ++i)
    t.insert(i);
  for (int i = 1; i <= N; ++i) {
    CHECK(t.erase(i));
    CHECK(t.validate());
  }
  CHECK(t.empty());
}

void testEraseAllReverse() {
  Tree24 t;
  constexpr int N = 50;
  for (int i = 1; i <= N; ++i)
    t.insert(i);
  for (int i = N; i >= 1; --i) {
    CHECK(t.erase(i));
    CHECK(t.validate());
  }
  CHECK(t.empty());
}

void testEraseShuffled() {
  std::vector<int> keys(60);
  std::iota(keys.begin(), keys.end(), 1);
  std::mt19937 rng(99);

  Tree24 t;
  for (int k : keys)
    t.insert(k);

  std::shuffle(keys.begin(), keys.end(), rng);
  for (int k : keys) {
    CHECK(t.erase(k));
    CHECK(t.validate());
  }
  CHECK(t.empty());
}

void testTree35Correctness() {
  Tree35 t;
  constexpr int N = 200;
  for (int i = 1; i <= N; ++i)
    t.insert(i);
  CHECK(t.size() == N);
  CHECK(t.validate());
  for (int i = 1; i <= N; i += 2) {
    CHECK(t.erase(i));
    CHECK(t.validate());
  }
  CHECK(t.size() == N / 2);
}

void testCopyAndSwap() {
  Tree24 a;
  for (int k : {1, 2, 3, 4, 5, 6, 7, 8})
    a.insert(k);

  Tree24 b(a); // deep copy
  b.erase(1);
  CHECK(a.contains(1) && !b.contains(1));
  CHECK(a.size() == 8 && b.size() == 7);
  CHECK(a.validate() && b.validate());

  swap(a, b); // non-member swap
  CHECK(!a.contains(1) && b.contains(1));
  CHECK(a.size() == 7 && b.size() == 8);

  a = b; // copy assignment
  CHECK(a.size() == 8 && a.contains(1));
  CHECK(a.validate());
}

void testHeightBound() {
  // For a (2,4)-tree with n keys: log4(n+1) <= height <= log2(n+1).
  // We just verify height stays within a loose but safe bound.
  Tree24 t;
  constexpr int N = 10000;
  for (int i = 0; i < N; ++i)
    t.insert(i);
  CHECK(t.size() == static_cast<std::size_t>(N));
  // log2(10000) ≈ 13; allow generous margin.
  CHECK(t.height() <= 25);
  CHECK(t.validate());
}

void testRandomizedAgainstStdSet() {
  constexpr int OPS = 4000;
  constexpr unsigned SEED = 42;
  std::mt19937 rng(SEED);
  std::uniform_int_distribution<int> dist_ins(1, OPS / 2);
  std::uniform_int_distribution<int> dist_era(1, OPS);

  Tree24 tree;
  std::set<int> ref;

  // Insert phase: compare results against std::set.
  for (int i = 0; i < OPS; ++i) {
    int k = dist_ins(rng);
    [[maybe_unused]] bool t_res = tree.insert(k);
    [[maybe_unused]] bool r_res = ref.insert(k).second;
    CHECK(t_res == r_res);
    CHECK(tree.size() == ref.size());
  }
  CHECK(tree.validate());

  // contains check for every possible key.
  for (int v = 1; v <= OPS; ++v)
    CHECK(tree.contains(v) == (ref.count(v) > 0));

  // Erase phase: compare results against std::set.
  for (int i = 0; i < OPS; ++i) {
    int k = dist_era(rng);
    [[maybe_unused]] bool t_res = tree.erase(k);
    [[maybe_unused]] bool r_res = ref.erase(k) > 0;
    CHECK(t_res == r_res);
    CHECK(tree.size() == ref.size());
    if (i % 100 == 0)
      CHECK(tree.validate());
  }
  CHECK(tree.validate());

  // Sorted-order check.
  auto got = tree.to_sorted_vector();
  std::vector<int> exp(ref.begin(), ref.end());
  CHECK(got == exp);
}

void testPerformanceSanity() {
  // 500 000 insertions then 500 000 deletions: confirms O(log n) behaviour
  // by simply not hanging / exhausting memory.
  constexpr int N = 500'000;
  Tree24 t;
  for (int i = 1; i <= N; ++i)
    t.insert(i);
  CHECK(t.size() == static_cast<std::size_t>(N));
  CHECK(t.height() <= 30); // log2(500k) ≈ 19; ample margin
  for (int i = 1; i <= N; ++i)
    t.erase(i);
  CHECK(t.empty());
}

} // namespace ABTree_Test
