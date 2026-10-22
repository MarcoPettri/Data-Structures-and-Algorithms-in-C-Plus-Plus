// File : C-7.24.cpp

/*
    C-7.24 Let T be a tree with n nodes. Define the lowest common ancestor (LCA)
    between two nodes v and w as the lowest node in T that has both v and
    w as descendents (where we allow a node to be a descendent of itself).
    Given two nodes v and w, describe an efficient algorithm for finding the
    LCA of v and w. What is the running time of your method?

    ─────────────────────────────────────────────────────────────────────────────
    Note: The complexity analysis is described in the comments for each function
    in the lca.hpp file.
    ─────────────────────────────────────────────────────────────────────────────
*/
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>

#include "general_tree.hpp"
#include "lca.hpp"

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

/// Checks that evaluating @p expr throws an exception of type @p ExType.
#define CHECK_THROWS_AS(expr, ExType)                                          \
  do {                                                                         \
    bool caught_ = false;                                                      \
    try {                                                                      \
      (void)(expr);                                                            \
    } catch (const ExType &) {                                                 \
      caught_ = true;                                                          \
    } catch (...) {                                                            \
    }                                                                          \
    CHECK(caught_);                                                            \
  } while (false)

using Tree = adt::GeneralTree<int>;
using Pos = Tree::Position;

constexpr std::size_t kNone = static_cast<std::size_t>(-1);

/// A tree plus an index-based model of it (node i stores element i).
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

  Pos lca(std::size_t v, std::size_t w) const {
    return adt::lowestCommonAncestor(tree, pos[v], pos[w]);
  }

  /// True if lca(v, w) and lca(w, v) are both node @p expected.
  bool lcaIs(std::size_t v, std::size_t w, std::size_t expected) const {
    return lca(v, w) == pos[expected] && lca(w, v) == pos[expected];
  }
};

/// Independent oracle: mark the ancestors of v (inclusive), climb from w.
std::size_t oracleLca(const std::vector<std::size_t> &par, std::size_t v,
                      std::size_t w) {
  std::vector<bool> isAncestor(par.size(), false);
  for (std::size_t x = v; x != kNone; x = par[x]) {
    isAncestor[x] = true;
  }
  std::size_t x = w;
  while (!isAncestor[x]) {
    x = par[x];
  }
  return x;
}

// ----------------------------------------------------------------- tests ---

void testSingleNode() {
  Model m;
  m.addRoot();
  CHECK(m.lca(0, 0) == m.tree.root());
}

void testSmallExample() {
  //         0
  //       / | \
  //      1  2  3
  //     / \     \
  //    4   5     6
  //               \
  //                7
  Model m;
  m.addRoot(); // 0
  m.add(0);    // 1
  m.add(0);    // 2
  m.add(0);    // 3
  m.add(1);    // 4
  m.add(1);    // 5
  m.add(3);    // 6
  m.add(6);    // 7

  CHECK(m.lcaIs(4, 5, 1)); // siblings
  CHECK(m.lcaIs(4, 2, 0)); // different subtrees of the root
  CHECK(m.lcaIs(5, 7, 0));
  CHECK(m.lcaIs(7, 3, 3)); // one node is an ancestor of the other
  CHECK(m.lcaIs(7, 6, 6));
  CHECK(m.lcaIs(7, 7, 7)); // v == w
  CHECK(m.lcaIs(0, 7, 0)); // the root is involved
  CHECK(m.lcaIs(1, 4, 1));
  CHECK(m.lca(4, 5)->element() == 1);
}

void testStar() {
  Model m;
  m.addRoot();
  for (int i = 0; i < 500; ++i) {
    m.add(0);
  }
  CHECK(m.lcaIs(17, 400, 0));
  CHECK(m.lcaIs(0, 400, 0));
  CHECK(m.lcaIs(250, 250, 250));
}

void testDeepPath() {
  constexpr std::size_t kN = 200000; // deep: everything must be iterative
  Model m;
  std::size_t last = m.addRoot();
  for (std::size_t i = 1; i < kN; ++i) {
    last = m.add(last);
  }
  // On a path the LCA of two nodes is the shallower (smaller index) one.
  CHECK(m.lcaIs(kN - 1, kN / 2, kN / 2));
  CHECK(m.lcaIs(0, kN - 1, 0));
  // The deep tree is also destroyed (iteratively) when `m` leaves scope.
}

void testRandomTreesAgainstOracle() {
  for (std::uint32_t seed = 1; seed <= 40; ++seed) {
    std::mt19937 rng(seed);
    const std::size_t n = 1 + (seed * 29) % 120;

    Model m;
    m.addRoot();
    for (std::size_t i = 1; i < n; ++i) {
      std::uniform_int_distribution<std::size_t> pick(0, i - 1);
      m.add(pick(rng));
    }

    bool ok = true;
    for (std::size_t v = 0; v < n; ++v) {
      for (std::size_t w = 0; w < n; ++w) {
        ok = ok && (m.lca(v, w) == m.pos[oracleLca(m.par, v, w)]);
      }
    }
    CHECK(ok);
  }
}

void testInvalidPositionsThrow() {
  Model m;
  m.addRoot();
  m.add(0);

  Model other;
  other.addRoot();

  // Null positions.
  CHECK_THROWS_AS(adt::lowestCommonAncestor(m.tree, Pos{}, m.pos[0]),
                  std::invalid_argument);
  CHECK_THROWS_AS(adt::lowestCommonAncestor(m.tree, m.pos[0], Pos{}),
                  std::invalid_argument);

  // Positions belonging to another tree.
  CHECK_THROWS_AS(adt::lowestCommonAncestor(m.tree, other.pos[0], m.pos[1]),
                  std::invalid_argument);
  CHECK_THROWS_AS(adt::lowestCommonAncestor(m.tree, m.pos[1], other.pos[0]),
                  std::invalid_argument);
  CHECK_THROWS_AS(adt::lowestCommonAncestor(m.tree, other.pos[0], other.pos[0]),
                  std::invalid_argument);

  // Empty tree.
  const Tree empty;
  CHECK_THROWS_AS(adt::lowestCommonAncestor(empty, m.pos[0], m.pos[0]),
                  std::invalid_argument);
}

struct TestCase {
  const char *name;
  void (*fn)();
};

} // namespace

int main() {
  const TestCase tests[] = {
      {"single node", testSingleNode},
      {"small example", testSmallExample},
      {"star", testStar},
      {"deep path (no stack overflow)", testDeepPath},
      {"random trees vs. oracle", testRandomTreesAgainstOracle},
      {"invalid positions throw", testInvalidPositionsThrow},
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