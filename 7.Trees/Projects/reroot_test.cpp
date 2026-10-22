/**
 * @file test_reroot.cpp
 * @brief Unit tests for adt::rerootAt (reroot.hpp).
 *
 * Uses a minimal self-contained test harness (no external dependencies).
 */
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

#include "../Creativity/general_tree.hpp"
#include "reroot.hpp"

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
using Edge = std::pair<int, int>; ///< (smaller, larger) element
using Shape =
    std::vector<std::pair<int, std::size_t>>; ///< preorder (element, depth)

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
};

/// A node together with its depth, as produced by preorder().
struct Visit {
  Pos node;
  std::size_t depth;
};

/// Iterative preorder traversal (children left to right).
std::vector<Visit> preorder(const Tree &tree) {
  std::vector<Visit> visits;
  std::vector<Visit> stack;
  if (!tree.isEmpty()) {
    stack.push_back(Visit{tree.root(), 0});
  }
  while (!stack.empty()) {
    const Visit visit = stack.back();
    stack.pop_back();
    visits.push_back(visit);
    const auto &kids = tree.children(visit.node);
    for (std::size_t i = kids.size(); i-- > 0;) {
      stack.push_back(Visit{kids[i].get(), visit.depth + 1});
    }
  }
  return visits;
}

/// (element, depth) in preorder: this determines an ordered tree uniquely.
Shape shapeOf(const Tree &tree) {
  Shape shape;
  for (const Visit &visit : preorder(tree)) {
    shape.emplace_back(visit.node->element(), visit.depth);
  }
  return shape;
}

/// Undirected edges of @p tree as (smaller, larger) element pairs.
std::set<Edge> edgesOf(const Tree &tree) {
  std::set<Edge> edges;
  std::vector<int>
      chain; // chain[d] = element of the current ancestor at depth d
  for (const Visit &visit : preorder(tree)) {
    const int element = visit.node->element();
    chain.resize(visit.depth);
    if (visit.depth > 0) {
      const int up = chain[visit.depth - 1];
      edges.emplace(std::min(up, element), std::max(up, element));
    }
    chain.push_back(element);
  }
  return edges;
}

/// Undirected edges of the model, straight from the parent array.
std::set<Edge> modelEdges(const std::vector<std::size_t> &par) {
  std::set<Edge> edges;
  for (std::size_t i = 1; i < par.size(); ++i) {
    const int a = static_cast<int>(par[i]);
    const int b = static_cast<int>(i);
    edges.emplace(std::min(a, b), std::max(a, b));
  }
  return edges;
}

/// BFS distances (in edges) from @p source in the model, ignoring direction.
std::vector<std::size_t> distancesFrom(const std::vector<std::size_t> &par,
                                       std::size_t source) {
  const std::size_t n = par.size();
  std::vector<std::vector<std::size_t>> adjacent(n);
  for (std::size_t i = 1; i < n; ++i) {
    adjacent[i].push_back(par[i]);
    adjacent[par[i]].push_back(i);
  }

  std::vector<std::size_t> dist(n, kNone);
  std::vector<std::size_t> queue{source};
  dist[source] = 0;
  for (std::size_t head = 0; head < queue.size(); ++head) {
    const std::size_t x = queue[head];
    for (const std::size_t y : adjacent[x]) {
      if (dist[y] == kNone) {
        dist[y] = dist[x] + 1;
        queue.push_back(y);
      }
    }
  }
  return dist;
}

/// Position holding @p element, or null. Linear scan; test helper only.
Pos findElement(const Tree &tree, int element) {
  for (const Visit &visit : preorder(tree)) {
    if (visit.node->element() == element) {
      return visit.node;
    }
  }
  return nullptr;
}

/// Full check of rerootAt(m.tree, node v) against the model's oracles.
bool rerootIsCorrect(const Model &m, std::size_t v,
                     const std::set<Edge> &edges) {
  const std::size_t n = m.pos.size();
  const Tree out = adt::rerootAt(m.tree, m.pos[v]);
  const std::vector<Visit> visits = preorder(out);

  if (out.size() != n || visits.size() != n) {
    return false;
  }
  if (out.root()->element() != static_cast<int>(v)) {
    return false;
  }

  // Every node exactly once, at depth = its distance from v in the old tree.
  const std::vector<std::size_t> dist = distancesFrom(m.par, v);
  std::vector<bool> seen(n, false);
  for (const Visit &visit : visits) {
    const int element = visit.node->element();
    if (element < 0 || static_cast<std::size_t>(element) >= n) {
      return false;
    }
    const std::size_t id = static_cast<std::size_t>(element);
    if (seen[id] || visit.depth != dist[id]) {
      return false;
    }
    seen[id] = true;
  }

  // Same adjacencies.
  if (edgesOf(out) != edges) {
    return false;
  }

  // Rerooting back at the old root gives the same adjacencies again.
  const Tree back = adt::rerootAt(out, findElement(out, 0));
  return back.root()->element() == 0 && edgesOf(back) == edges;
}

/// Runs rerootIsCorrect for every node of @p m.
bool allRerootsCorrect(const Model &m) {
  const std::set<Edge> edges = modelEdges(m.par);
  if (edgesOf(m.tree) != edges) {
    return false; // the helpers themselves must agree on the input
  }
  for (std::size_t v = 0; v < m.pos.size(); ++v) {
    if (!rerootIsCorrect(m, v, edges)) {
      return false;
    }
  }
  return true;
}

// ----------------------------------------------------------------- tests ---

void testSingleNode() {
  Model m;
  m.addRoot();
  const Tree out = adt::rerootAt(m.tree, m.pos[0]);
  CHECK(out.size() == 1);
  CHECK(out.root()->element() == 0);
  CHECK(out.isExternal(out.root()));
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

  const Shape original = {{0, 0}, {1, 1}, {4, 2}, {5, 2},
                          {2, 1}, {3, 1}, {6, 2}, {7, 3}};
  CHECK(shapeOf(m.tree) == original);

  // v is the old root: the result is a copy.
  CHECK(shapeOf(adt::rerootAt(m.tree, m.pos[0])) == original);

  // v = 6, path 0-3-6: 6(7 3(0(1(4 5) 2))).
  CHECK(
      shapeOf(adt::rerootAt(m.tree, m.pos[6])) ==
      (Shape{{6, 0}, {7, 1}, {3, 1}, {0, 2}, {1, 3}, {4, 4}, {5, 4}, {2, 3}}));

  // v = 4, path 0-1-4: 4(1(5 0(2 3(6(7))))).
  CHECK(
      shapeOf(adt::rerootAt(m.tree, m.pos[4])) ==
      (Shape{{4, 0}, {1, 1}, {5, 2}, {0, 2}, {2, 3}, {3, 3}, {6, 4}, {7, 5}}));

  // v = 1, path 0-1: 1(4 5 0(2 3(6(7)))).
  CHECK(
      shapeOf(adt::rerootAt(m.tree, m.pos[1])) ==
      (Shape{{1, 0}, {4, 1}, {5, 1}, {0, 1}, {2, 2}, {3, 2}, {6, 3}, {7, 4}}));

  CHECK(shapeOf(m.tree) == original); // the input tree is untouched
  CHECK(allRerootsCorrect(m));
}

void testStar() {
  Model m;
  m.addRoot();
  for (int i = 0; i < 500; ++i) {
    m.add(0);
  }
  CHECK(allRerootsCorrect(m)); // at the centre and at every leaf
}

void testDeepPath() {
  constexpr std::size_t kN = 200000; // deep: everything must be iterative
  Model m;
  std::size_t last = m.addRoot();
  for (std::size_t i = 1; i < kN; ++i) {
    last = m.add(last);
  }

  // Rooting a path at its far end reverses it.
  const Tree out = adt::rerootAt(m.tree, m.pos[kN - 1]);
  CHECK(out.size() == kN);

  Pos cur = out.root();
  std::size_t visited = 0;
  while (cur != nullptr &&
         cur->element() == static_cast<int>(kN - 1 - visited)) {
    ++visited;
    const auto &kids = out.children(cur);
    cur = kids.empty() ? nullptr : kids[0].get();
  }
  CHECK(visited == kN);
  // Both deep trees are destroyed iteratively when they leave scope.
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
    CHECK(allRerootsCorrect(m));
  }
}

void testSkewedRandomTreesAgainstOracle() {
  // Attaching mostly to recent nodes gives long, thin trees.
  for (std::uint32_t seed = 1; seed <= 40; ++seed) {
    std::mt19937 rng(seed * 7919u);
    const std::size_t n = 2 + (seed * 17) % 100;

    Model m;
    m.addRoot();
    for (std::size_t i = 1; i < n; ++i) {
      const std::size_t lo = i > 3 ? i - 3 : 0;
      std::uniform_int_distribution<std::size_t> pick(lo, i - 1);
      m.add(pick(rng));
    }
    CHECK(allRerootsCorrect(m));
  }
}

void testInvalidPositionsThrow() {
  Model m;
  m.addRoot();
  m.add(0);

  Model other;
  other.addRoot();

  CHECK_THROWS_AS(adt::rerootAt(m.tree, Pos{}), std::invalid_argument);
  CHECK_THROWS_AS(adt::rerootAt(m.tree, other.pos[0]), std::invalid_argument);

  Tree empty;
  CHECK_THROWS_AS(adt::rerootAt(empty, m.pos[0]), std::invalid_argument);
  CHECK_THROWS_AS(adt::rerootAt(empty, Pos{}), std::invalid_argument);
}

struct TestCase {
  const char *name;
  void (*fn)();
};

} // namespace

int main() {
  const TestCase tests[] = {
      {"single node", testSingleNode},
      {"small example (exact shapes)", testSmallExample},
      {"star", testStar},
      {"deep path (no stack overflow)", testDeepPath},
      {"random trees vs. oracle", testRandomTreesAgainstOracle},
      {"skewed random trees vs. oracle", testSkewedRandomTreesAgainstOracle},
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