/**
 * @file test_union_find.cpp
 * @brief Self-contained unit tests for algo::UnionFind and algo::Partition.

 */

#include "partition.hpp"
#include "union_find.hpp"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace test_union_find {

int g_checks = 0;
int g_failures = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    ++g_checks;                                                                \
    if (!(cond)) {                                                             \
      ++g_failures;                                                            \
      std::cerr << "  FAIL " << __FILE__ << ':' << __LINE__ << "  " << #cond   \
                << '\n';                                                       \
    }                                                                          \
  } while (false)

#define CHECK_THROWS_AS(expr, ExType)                                          \
  do {                                                                         \
    ++g_checks;                                                                \
    bool caught_ = false;                                                      \
    try {                                                                      \
      static_cast<void>(expr);                                                 \
    } catch (const ExType &) {                                                 \
      caught_ = true;                                                          \
    } catch (...) {                                                            \
    }                                                                          \
    if (!caught_) {                                                            \
      ++g_failures;                                                            \
      std::cerr << "  FAIL " << __FILE__ << ':' << __LINE__                    \
                << "  expected " #ExType " from " #expr "\n";                  \
    }                                                                          \
  } while (false)

/// Runs one test function and reports it.
void run(const char *name, void (*fn)()) {
  const int before = g_failures;
  fn();
  std::cout << (g_failures == before ? "[ OK ] " : "[FAIL] ") << name << '\n';
}

using algo::UnionFind;

// ---------------------------------------------------------------------------
// UnionFind: basics
// ---------------------------------------------------------------------------

void test_empty_and_singletons() {
  UnionFind empty;
  CHECK(empty.empty());
  CHECK(empty.size() == 0);
  CHECK(empty.numSets() == 0);

  UnionFind uf(5);
  CHECK(uf.size() == 5);
  CHECK(uf.numSets() == 5);
  for (std::size_t i = 0; i < 5; ++i) {
    CHECK(uf.find(i) == i);
    CHECK(uf.setSize(i) == 1);
    CHECK(uf.depth(i) == 0);
  }
  CHECK(!uf.connected(0, 1));
}

void test_unite_and_connected() {
  UnionFind uf(6);
  CHECK(uf.unite(0, 1));
  CHECK(uf.unite(2, 3));
  CHECK(!uf.connected(1, 2));
  CHECK(uf.unite(1, 3));
  CHECK(uf.connected(0, 2));
  CHECK(uf.connected(0, 3));
  CHECK(!uf.connected(0, 4));
  CHECK(!uf.unite(0, 3));  // already together
  CHECK(!uf.unite(2, 2));  // same element
  CHECK(uf.numSets() == 3);
  CHECK(uf.setSize(0) == 4);
  CHECK(uf.setSize(3) == 4);
  CHECK(uf.setSize(4) == 1);
  CHECK(uf.find(0) == uf.find(3));
  CHECK(uf.find(4) != uf.find(5));
}

void test_makeSet_growth() {
  UnionFind uf;
  CHECK(uf.makeSet() == 0);
  CHECK(uf.makeSet() == 1);
  CHECK(uf.makeSet() == 2);
  CHECK(uf.size() == 3 && uf.numSets() == 3);
  uf.unite(0, 2);
  CHECK(uf.makeSet() == 3);  // new singleton, existing sets unaffected
  CHECK(uf.numSets() == 3);
  CHECK(uf.connected(0, 2));
  CHECK(!uf.connected(3, 0));
  uf.clear();
  CHECK(uf.empty() && uf.numSets() == 0);
}

void test_out_of_range() {
  UnionFind uf(3);
  CHECK_THROWS_AS(uf.find(3), std::out_of_range);
  CHECK_THROWS_AS(uf.unite(0, 7), std::out_of_range);
  CHECK_THROWS_AS(uf.unite(9, 0), std::out_of_range);
  CHECK_THROWS_AS(uf.connected(3, 0), std::out_of_range);
  CHECK_THROWS_AS(uf.setSize(100), std::out_of_range);
  CHECK_THROWS_AS(uf.depth(3), std::out_of_range);
  CHECK(uf.numSets() == 3);  // untouched
}

// ---------------------------------------------------------------------------
// UnionFind: the two heuristics
// ---------------------------------------------------------------------------

void test_union_by_size_attaches_small_under_large() {
  UnionFind uf(5);
  uf.unite(0, 1);
  uf.unite(0, 2);
  uf.unite(0, 3);  // {0,1,2,3}, root 0
  const std::size_t bigRoot = uf.find(0);
  // Singleton 4 passed as FIRST argument must still be hung under the big tree.
  uf.unite(4, 0);
  CHECK(uf.find(4) == bigRoot);
  CHECK(uf.find(0) == bigRoot);
  CHECK(uf.setSize(4) == 5);
}

void test_union_by_size_bounds_height() {
  constexpr std::size_t n = 1024;  // 2^10
  UnionFind uf(n);
  // Balanced pairwise merging: the worst case for tree height.
  for (std::size_t step = 1; step < n; step *= 2) {
    for (std::size_t i = 0; i < n; i += 2 * step) {
      CHECK(uf.unite(i, i + step));
    }
  }
  CHECK(uf.numSets() == 1);
  std::size_t maxDepth = 0;
  for (std::size_t i = 0; i < n; ++i)
    maxDepth = std::max(maxDepth, uf.depth(i));
  CHECK(maxDepth <= 10);  // floor(log2 1024)
}

void test_path_compression_flattens_path() {
  // Tie-breaking rule used here: on equal sizes, the root of the first argument stays root.
  UnionFind uf(8);
  uf.unite(0, 1);
  uf.unite(2, 3);
  uf.unite(4, 5);
  uf.unite(6, 7);
  uf.unite(0, 2);
  uf.unite(4, 6);
  uf.unite(0, 4);

  // Path 7 -> 6 -> 4 -> 0 has not been compressed yet.
  CHECK(uf.depth(7) == 3);
  CHECK(uf.depth(6) == 2);

  CHECK(uf.find(7) == 0);

  // After a single find every node on that path points straight at the root.
  CHECK(uf.depth(7) == 1);
  CHECK(uf.depth(6) == 1);
  CHECK(uf.depth(4) == 1);
  CHECK(uf.depth(0) == 0);
}

// ---------------------------------------------------------------------------
// UnionFind: randomised and large inputs
// ---------------------------------------------------------------------------

void test_randomised_against_naive_labels() {
  std::mt19937 gen(12345);
  for (int round = 0; round < 50; ++round) {
    const std::size_t n = 1 + gen() % 200;
    UnionFind uf(n);

    std::vector<std::size_t> label(n);
    std::iota(label.begin(), label.end(), std::size_t{0});
    std::size_t sets = n;

    std::uniform_int_distribution<std::size_t> pick(0, n - 1);
    for (int op = 0; op < 400; ++op) {
      const std::size_t a = pick(gen), b = pick(gen);
      const bool merged = label[a] != label[b];
      CHECK(uf.unite(a, b) == merged);
      if (merged) {
        const std::size_t from = label[b], to = label[a];
        std::replace(label.begin(), label.end(), from, to);
        --sets;
      }
      CHECK(uf.numSets() == sets);
    }
    for (int q = 0; q < 200; ++q) {
      const std::size_t a = pick(gen), b = pick(gen);
      CHECK(uf.connected(a, b) == (label[a] == label[b]));
    }
    for (std::size_t x = 0; x < n; ++x) {
      const auto expected = static_cast<std::size_t>(
          std::count(label.begin(), label.end(), label[x]));
      CHECK(uf.setSize(x) == expected);
    }
  }
}

void test_large_chain_no_deep_recursion() {
  constexpr std::size_t n = 1'000'000;
  UnionFind uf(n);
  for (std::size_t i = 0; i + 1 < n; ++i)
    uf.unite(i, i + 1);       // forward chain
  CHECK(uf.numSets() == 1);
  CHECK(uf.setSize(n - 1) == n);
  CHECK(uf.connected(0, n - 1));

  UnionFind rev(n);
  for (std::size_t i = n - 1; i > 0; --i)
    rev.unite(i, i - 1);      // reverse chain
  CHECK(rev.numSets() == 1);
  CHECK(rev.find(0) == rev.find(n - 1));
}

// ---------------------------------------------------------------------------
// Partition<T>
// ---------------------------------------------------------------------------

void test_partition_strings() {
  algo::Partition<std::string> p;
  CHECK(p.empty());
  CHECK(p.makeSet("a"));
  CHECK(p.makeSet("b"));
  CHECK(p.makeSet("c"));
  CHECK(p.makeSet("d"));
  CHECK(!p.makeSet("a"));  // duplicate
  CHECK(p.size() == 4);
  CHECK(p.numSets() == 4);
  CHECK(p.contains("c") && !p.contains("z"));

  CHECK(p.unite("a", "b"));
  CHECK(p.unite("c", "d"));
  CHECK(!p.connected("a", "c"));
  CHECK(p.unite("b", "d"));
  CHECK(!p.unite("a", "c"));
  CHECK(p.connected("a", "c"));
  CHECK(p.numSets() == 1);
  CHECK(p.setSize("d") == 4);

  const std::string rep =
      p.find("a");  // copy: reference may dangle after makeSet
  CHECK(p.find("b") == rep && p.find("c") == rep && p.find("d") == rep);
  CHECK(rep == "a" || rep == "b" || rep == "c" || rep == "d");
}

void test_partition_unknown_element_and_integers() {
  algo::Partition<int> p;
  p.makeSet(10);
  p.makeSet(20);
  CHECK_THROWS_AS(p.find(30), std::out_of_range);
  CHECK_THROWS_AS(p.unite(10, 30), std::out_of_range);
  CHECK_THROWS_AS(p.connected(30, 10), std::out_of_range);
  CHECK_THROWS_AS(p.setSize(30), std::out_of_range);
  CHECK(p.numSets() == 2);  // failed unite changed nothing
  CHECK(p.unite(10, 20));
  CHECK(p.setSize(20) == 2);
}

void test_partition_singleton_representative() {
  algo::Partition<std::string> p;
  p.makeSet("only");
  CHECK(p.find("only") == "only");
  CHECK(p.setSize("only") == 1);
}

}  // namespace test_union_find
