// File : C-7.25.cpp

/*
    C-7.25 Let T be a tree with n nodes, and, for any node v in T , let dv
   denote the depth of v in T . The distance between two nodes v and w in T is
    d_v + d_w − 2d_u, where u is the LCA u of v and w (as defined in the
   previous exercise). The diameter of T is the maximum distance between two
   nodes in T . Describe an efficient algorithm for finding the diameter of T .
   What is the running time of your method?

    ─────────────────────────────────────────────────────────────────────────────
    The complexity analysis is described in the comments for the function in
    the diameter.hpp file.
    ─────────────────────────────────────────────────────────────────────────────
*/

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

#include "diameter.hpp"
#include "general_tree.hpp"

namespace {

int g_failures = 0; ///< Number of failed checks so far.

/// Records a failure (with location) if @p cond is false.
#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      ++g_failures;                                                            \
      std::cerr << __FILE__ << ':' << __LINE__                                 \
                << ": CHECK failed: " #cond "\n";                              \
    }                                                                          \
  } while (false)

using Tree = adt::GeneralTree<int>;
using Pos = Tree::Position;

constexpr std::size_t kNone = static_cast<std::size_t>(-1);

/// A tree plus an index-based model of it (node i stores element i).
/// Invariant: par[i] < i, since a node is added after its parent.
struct Model {
  Tree tree;
  std::vector<Pos> pos;         ///< pos[i] = position of node i
  std::vector<std::size_t> par; ///< par[i] = index of i's parent (kNone: root)

  std::size_t addRoot() {
    pos.push_back(tree.insertRoot(0));
    par.push_back(kNone);
    return 0;
  }

  std::size_t add(std::size_t parent) {
    const std::size_t id = pos.size();
    pos.push_back(tree.addChild(pos[parent], static_cast<int>(id)));
    par.push_back(parent);
    return id;
  }

  std::size_t diameter() const { return adt::diameter(tree); }
};

/// Brute-force oracle straight from the definition: the maximum over all
/// pairs (v, w) of d_v + d_w - 2 * d_u, with u = LCA(v, w). Theta(n^2 * h).
std::size_t oracleDiameter(const std::vector<std::size_t> &par) {
  const std::size_t n = par.size();
  std::vector<std::size_t> depth(n, 0);
  for (std::size_t i = 1; i < n; ++i) {
    depth[i] = depth[par[i]] + 1;
  }

  std::size_t best = 0;
  for (std::size_t v = 0; v < n; ++v) {
    std::vector<bool> isAncestor(n, false); // ancestors of v, inclusive
    for (std::size_t x = v; x != kNone; x = par[x]) {
      isAncestor[x] = true;
    }
    for (std::size_t w = 0; w < n; ++w) {
      std::size_t u = w; // climb from w to the first ancestor of v
      while (!isAncestor[u]) {
        u = par[u];
      }
      best = std::max(best, depth[v] + depth[w] - 2 * depth[u]);
    }
  }
  return best;
}

// ----------------------------------------------------------------- tests ---

void testEmptyAndTiny() {
  const Tree empty;
  CHECK(adt::diameter(empty) == 0);

  Model one;
  one.addRoot();
  CHECK(one.diameter() == 0);

  Model two;
  two.addRoot();
  two.add(0);
  CHECK(two.diameter() == 1);
}

void testSmallExample() {
  //         0
  //       / | \
  //      1  2  3
  //     / \     \
  //    4   5     6
  //               \
  //                7
  // Longest path: 7-6-3-0-1-4 (5 edges).
  Model m;
  m.addRoot(); // 0
  m.add(0);    // 1
  m.add(0);    // 2
  m.add(0);    // 3
  m.add(1);    // 4
  m.add(1);    // 5
  m.add(3);    // 6
  m.add(6);    // 7
  CHECK(m.diameter() == 5);
  CHECK(m.diameter() == oracleDiameter(m.par));
}

void testPathAvoidsRoot() {
  // The root has a single child; the longest path 4-2-1-3-5 lies below it.
  Model m;
  m.addRoot(); // 0
  m.add(0);    // 1
  m.add(1);    // 2
  m.add(1);    // 3
  m.add(2);    // 4
  m.add(3);    // 5
  CHECK(m.diameter() == 4);
  CHECK(m.diameter() == oracleDiameter(m.par));
}

void testTwoUnequalChains() {
  // Root with chains of length 3 and 4: the path joins their ends (7 edges).
  Model m;
  m.addRoot(); // 0
  m.add(0);    // 1
  m.add(1);    // 2
  m.add(2);    // 3
  m.add(0);    // 4
  m.add(4);    // 5
  m.add(5);    // 6
  m.add(6);    // 7
  CHECK(m.diameter() == 7);
  CHECK(m.diameter() == oracleDiameter(m.par));
}

void testStar() {
  Model m;
  m.addRoot();
  for (int i = 0; i < 500; ++i) {
    m.add(0);
  }
  CHECK(m.diameter() == 2); // leaf - root - leaf
}

void testDeepPath() {
  constexpr std::size_t kN = 200000; // deep: everything must be iterative
  Model m;
  std::size_t last = m.addRoot();
  for (std::size_t i = 1; i < kN; ++i) {
    last = m.add(last);
  }
  CHECK(m.diameter() == kN - 1);
  // The deep tree is also destroyed (iteratively) when `m` leaves scope.
}

void testRandomTreesAgainstOracle() {
  for (std::uint32_t seed = 1; seed <= 60; ++seed) {
    std::mt19937 rng(seed);
    const std::size_t n = 1 + (seed * 29) % 120;

    Model m;
    m.addRoot();
    for (std::size_t i = 1; i < n; ++i) {
      std::uniform_int_distribution<std::size_t> pick(0, i - 1);
      m.add(pick(rng));
    }
    CHECK(m.diameter() == oracleDiameter(m.par));
  }
}

void testSkewedRandomTreesAgainstOracle() {
  // Attaching mostly to recent nodes gives long, thin trees.
  for (std::uint32_t seed = 1; seed <= 60; ++seed) {
    std::mt19937 rng(seed * 7919u);
    const std::size_t n = 2 + (seed * 17) % 100;

    Model m;
    m.addRoot();
    for (std::size_t i = 1; i < n; ++i) {
      const std::size_t lo = i > 3 ? i - 3 : 0;
      std::uniform_int_distribution<std::size_t> pick(lo, i - 1);
      m.add(pick(rng));
    }
    CHECK(m.diameter() == oracleDiameter(m.par));
  }
}

struct TestCase {
  const char *name;
  void (*fn)();
};

} // namespace

int main() {
  const TestCase tests[] = {
      {"empty and tiny trees", testEmptyAndTiny},
      {"small example", testSmallExample},
      {"path avoids the root", testPathAvoidsRoot},
      {"two unequal chains", testTwoUnequalChains},
      {"star", testStar},
      {"deep path (no stack overflow)", testDeepPath},
      {"random trees vs. oracle", testRandomTreesAgainstOracle},
      {"skewed random trees vs. oracle", testSkewedRandomTreesAgainstOracle},
  };

  int failedTests = 0;
  for (const auto &tc : tests) {
    const int before = g_failures;
    tc.fn();
    const bool passed = (g_failures == before);
    std::cout << (passed ? "[ PASS ] " : "[ FAIL ] ") << tc.name << '\n';
    if (!passed) {
      ++failedTests;
    }
  }

  std::cout << (failedTests == 0 ? "\nAll tests passed.\n"
                                 : "\nSome tests FAILED.\n");
  return failedTests == 0 ? 0 : 1;
}