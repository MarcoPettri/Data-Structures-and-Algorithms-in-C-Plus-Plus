// File : C-7.26.cpp

/*
    C-7.26 Suppose each node v of a binary tree T is labeled with its value f
   (v) in a level numbering of T . Design a fast method for determining f (u)
   for the lowest common ancestor (LCA), u, of two nodes v and w in T , given f
   (v) and f (w). You do not need to find node u, just compute its level-
    numbering label.

    /////////////////////////////////
    Summary:

    - Method: lcaLevelNumberByClimbing (repeat (f−1)/2 on the larger label)
        - Time: O(depth(v) + depth(w)) = O(h)
        - Space: O(1)

    - Method: lcaLevelNumber (shift to equal length, XOR, shift)
        - Time: O(1) word operations
        - Space: O(1)

    /////////////////////////////////
*/

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

#include "level_lca.hpp"

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

// The functions are constexpr, so the basic cases are also checked at compile
// time.
static_assert(adt::lcaLevelNumber(3, 4) == 1, "siblings under node 1");
static_assert(adt::lcaLevelNumber(3, 6) == 0, "different subtrees of the root");
static_assert(adt::lcaLevelNumber(5, 5) == 5, "v == w");
static_assert(adt::lcaLevelNumberByClimbing(4, 2) == 0, "climbing variant");
static_assert(adt::levelNumberDepth(6) == 2, "depth of node 6");

constexpr std::size_t kNone = static_cast<std::size_t>(-1);
constexpr std::uint64_t kMax = std::numeric_limits<std::uint64_t>::max();

/// A binary tree built slot by slot, plus a model of it: node i has level
/// number label[i] and parent par[i] (kNone for the root).
struct Model {
  std::vector<std::uint64_t> label;
  std::vector<std::size_t> par;
  std::vector<std::pair<std::size_t, bool>> freeSlots; ///< (parent, isLeft)

  Model() {
    label.push_back(0);
    par.push_back(kNone);
    freeSlots.emplace_back(0, true);
    freeSlots.emplace_back(0, false);
  }

  /// Attaches a new node to the free slot at index @p k of freeSlots.
  void attach(std::size_t k) {
    const auto [p, isLeft] = freeSlots[k];
    freeSlots[k] = freeSlots.back();
    freeSlots.pop_back();

    const std::size_t id = label.size();
    label.push_back(2 * label[p] + (isLeft ? 1 : 2));
    par.push_back(p);
    freeSlots.emplace_back(id, true);
    freeSlots.emplace_back(id, false);
  }
};

/// Oracle on the real tree: mark ancestors of v (inclusive), climb from w.
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

/// True if both implementations agree with the model for every ordered pair.
bool allPairsAgree(const Model &m) {
  const std::size_t n = m.label.size();
  for (std::size_t v = 0; v < n; ++v) {
    for (std::size_t w = 0; w < n; ++w) {
      const std::uint64_t expected = m.label[oracleLca(m.par, v, w)];
      if (adt::lcaLevelNumber(m.label[v], m.label[w]) != expected ||
          adt::lcaLevelNumberByClimbing(m.label[v], m.label[w]) != expected) {
        return false;
      }
    }
  }
  return true;
}

// ----------------------------------------------------------------- tests ---

/// Both implementations share this signature. (A noexcept function pointer
/// converts implicitly to this type; deducing one type from both does not
/// work.)
using LcaFn = std::uint64_t (*)(std::uint64_t, std::uint64_t);

void testSmallExample() {
  //          0
  //        /   \
  //       1     2
  //      / \   / \
  //     3   4 5   6
  const LcaFn implementations[] = {&adt::lcaLevelNumber,
                                   &adt::lcaLevelNumberByClimbing};

  for (const LcaFn lca : implementations) {
    CHECK(lca(3, 4) == 1); // siblings
    CHECK(lca(5, 6) == 2);
    CHECK(lca(3, 6) == 0); // different subtrees of the root
    CHECK(lca(4, 5) == 0);
    CHECK(lca(3, 1) == 1); // one node is an ancestor of the other
    CHECK(lca(6, 2) == 2);
    CHECK(lca(6, 0) == 0); // the root is involved
    CHECK(lca(0, 0) == 0);
    CHECK(lca(4, 4) == 4); // v == w
    CHECK(lca(1, 2) == 0);
  }
}
void testDepth() {
  CHECK(adt::levelNumberDepth(0) == 0);
  CHECK(adt::levelNumberDepth(1) == 1);
  CHECK(adt::levelNumberDepth(2) == 1);
  for (std::uint64_t f = 3; f <= 6; ++f) {
    CHECK(adt::levelNumberDepth(f) == 2);
  }
  CHECK(adt::levelNumberDepth(7) == 3);
  CHECK(adt::levelNumberDepth(14) == 3);
  CHECK(adt::levelNumberDepth(15) == 4);
  CHECK(adt::levelNumberDepth(kMax - 1) == 63);
}

void testRandomTreesAgainstOracle() {
  for (std::uint32_t seed = 1; seed <= 60; ++seed) {
    std::mt19937 rng(seed);
    const std::size_t n = 1 + (seed * 29) % 150;

    Model m;
    while (m.label.size() < n) {
      std::uniform_int_distribution<std::size_t> pick(0,
                                                      m.freeSlots.size() - 1);
      m.attach(pick(rng));
    }
    CHECK(allPairsAgree(m));
  }
}

void testSkewedTreesAgainstOracle() {
  // Always attaching to one of the three newest slots gives deep, thin trees
  // (depth up to about 60, so labels still fit in 64 bits).
  for (std::uint32_t seed = 1; seed <= 60; ++seed) {
    std::mt19937 rng(seed * 7919u);
    const std::size_t n = 2 + (seed * 17) % 60;

    Model m;
    while (m.label.size() < n) {
      const std::size_t count = m.freeSlots.size();
      const std::size_t lo = count > 3 ? count - 3 : 0;
      std::uniform_int_distribution<std::size_t> pick(lo, count - 1);
      m.attach(pick(rng));
    }
    CHECK(allPairsAgree(m));
  }
}

void testFullWidthLabels() {
  // All-right path: f_0 = 0, f_{d+1} = 2 f_d + 2; f_63 is the largest usable
  // label.
  std::vector<std::uint64_t> f(64);
  f[0] = 0;
  for (std::size_t d = 1; d < f.size(); ++d) {
    f[d] = 2 * f[d - 1] + 2;
  }
  CHECK(f[63] == kMax - 1);

  bool ok = true;
  for (std::size_t d = 0; d < f.size(); ++d) {
    // On a path the LCA is the shallower node.
    ok = ok && (adt::lcaLevelNumber(f[63], f[d]) == f[d]);
    ok = ok && (adt::lcaLevelNumber(f[d], f[63]) == f[d]);
    ok = ok && (adt::lcaLevelNumberByClimbing(f[63], f[d]) == f[d]);
  }
  CHECK(ok);

  // Two leaves at depth 63 that split at depth 62 (children of f[62]).
  CHECK(adt::lcaLevelNumber(kMax - 2, kMax - 1) == f[62]);
  CHECK(adt::lcaLevelNumberByClimbing(kMax - 2, kMax - 1) == f[62]);
  // Deepest leaf against a node on the other side of the root.
  CHECK(adt::lcaLevelNumber(kMax - 1, 1) == 0);
}

void testOverflowThrows() {
  CHECK_THROWS_AS(adt::lcaLevelNumber(kMax, 0), std::invalid_argument);
  CHECK_THROWS_AS(adt::lcaLevelNumber(0, kMax), std::invalid_argument);
  CHECK_THROWS_AS(adt::levelNumberDepth(kMax), std::invalid_argument);
}

struct TestCase {
  const char *name;
  void (*fn)();
};

} // namespace

int main() {
  const TestCase tests[] = {
      {"small example", testSmallExample},
      {"depth from level number", testDepth},
      {"random trees vs. oracle", testRandomTreesAgainstOracle},
      {"skewed trees vs. oracle", testSkewedTreesAgainstOracle},
      {"full 64-bit labels (depth 63)", testFullWidthLabels},
      {"overflow throws", testOverflowThrows},
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