// File : C-7.6.cpp

/*
    C-7.6 Give an O(n)-time algorithm for computing the depth of all the nodes
    of a tree T, where n is the number of nodes of T.

   ─────────────────────────────────────────────────────────────────────────────
   Solution
   ─────────────────────────────────────────────────────────────────────────────

   Definition:
     The *depth* of a node v is the number of edges on the path from the root
     down to v:
       depth(root) = 0
       depth(v)    = depth(parent(v)) + 1   for every non-root node v.

   Algorithm (BFS / level-order traversal):

     Observe that if we know the depth of a node p, we immediately know the
     depth of every child c of p: depth(c) = depth(p) + 1.
     A breadth-first scan processes nodes level-by-level, so a parent is
     always dequeued before its children — no re-visits are needed.

       depth[root] = 0; enqueue root.
       While queue is non-empty:
         p, d = dequeue()
         record depth[p] = d
         for each child c of p: enqueue (c, d + 1)

   Correctness:
     Every node is enqueued exactly once (when its parent is dequeued), and
     dequeued exactly once.  At dequeue time the depth value carried by the
     queue entry equals the true depth (proved by induction on depth).

   Complexity Analysis:
     * Running time : O(n) — each node is enqueued and dequeued once;
                     every edge is examined once when the parent is dequeued.
     * Space usage  : O(n) — the BFS queue holds at most one complete level;
                     the output map stores exactly n entries.
*/

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "general_tree.hpp"
#include "node_depths.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// Minimal test harness
// ─────────────────────────────────────────────────────────────────────────────

namespace {

int failures = 0;

/// @cond INTERNAL
#define CHECK_EQ(actual, expected)                                             \
  do {                                                                         \
    const auto a_ = (actual);                                                  \
    const auto e_ = (expected);                                                \
    if (a_ != e_) {                                                            \
      ++failures;                                                              \
      std::cerr << "FAIL line " << __LINE__ << ": " #actual " = " << a_        \
                << ", expected " << e_ << '\n';                                \
    }                                                                          \
  } while (0)
/// @endcond

using Tree     = adt::GeneralTree<std::string>;
using Position = Tree::Position;

// ─────────────────────────────────────────────────────────────────────────────
// Test cases
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Empty tree returns an empty map. */
void testEmpty() {
  Tree t;
  const auto depths = algo::computeDepths<std::string>(t);
  CHECK_EQ(depths.empty(), true);
}

/**
 * @brief Single-node tree — the root has depth 0.
 *
 * Tree:  "root"
 */
void testSingleNode() {
  Tree t;
  auto r = t.insertRoot("root");
  const auto depths = algo::computeDepths<std::string>(t);
  CHECK_EQ(depths.size(), std::size_t{1});
  CHECK_EQ(depths.at(r), 0);
}

/**
 * @brief Root with two leaf children.
 *
 * Tree:   "A"          depth 0
 *         / \.
 *        "B" "C"       depth 1
 */
void testRootWithTwoChildren() {
  Tree t;
  auto r = t.insertRoot("A");
  auto b = t.addChild(r, "B");
  auto c = t.addChild(r, "C");
  const auto depths = algo::computeDepths<std::string>(t);
  CHECK_EQ(depths.size(), std::size_t{3});
  CHECK_EQ(depths.at(r), 0);
  CHECK_EQ(depths.at(b), 1);
  CHECK_EQ(depths.at(c), 1);
}

/**
 * @brief Linear chain — each node is one level deeper than its parent.
 *
 * Tree:  "0" -> "1" -> "2" -> "3" -> "4"
 *        depths:  0     1      2      3     4
 */
void testLinearChain() {
  Tree t;
  auto prev = t.insertRoot("0");
  std::vector<Position> nodes{prev};
  for (int i = 1; i <= 4; ++i) {
    prev = t.addChild(prev, std::to_string(i));
    nodes.push_back(prev);
  }
  const auto depths = algo::computeDepths<std::string>(t);
  CHECK_EQ(depths.size(), std::size_t{5});
  for (int i = 0; i <= 4; ++i) {
    CHECK_EQ(depths.at(nodes[i]), i);
  }
}

/**
 * @brief Perfect binary tree with 7 nodes (3 levels).
 *
 *            "1"            depth 0
 *           /   \.
 *         "2"   "3"         depth 1
 *        / \   / \.
 *       4   5 6   7         depth 2
 */
void testPerfectBinaryTree() {
  Tree t;
  auto n1 = t.insertRoot("1");
  auto n2 = t.addChild(n1, "2");
  auto n3 = t.addChild(n1, "3");
  auto n4 = t.addChild(n2, "4");
  auto n5 = t.addChild(n2, "5");
  auto n6 = t.addChild(n3, "6");
  auto n7 = t.addChild(n3, "7");

  const auto depths = algo::computeDepths<std::string>(t);
  CHECK_EQ(depths.size(), std::size_t{7});
  CHECK_EQ(depths.at(n1), 0);
  CHECK_EQ(depths.at(n2), 1);
  CHECK_EQ(depths.at(n3), 1);
  CHECK_EQ(depths.at(n4), 2);
  CHECK_EQ(depths.at(n5), 2);
  CHECK_EQ(depths.at(n6), 2);
  CHECK_EQ(depths.at(n7), 2);
}

/**
 * @brief Ternary tree — root has 3 children, each with 2 leaf children.
 *
 * Depths: root=0, level-1 nodes=1, level-2 leaves=2.
 * Total nodes: 1 + 3 + 6 = 10.
 */
void testTernaryTree() {
  Tree t;
  auto root = t.insertRoot("root");
  std::vector<Position> lvl1, lvl2;
  for (int i = 0; i < 3; ++i) {
    auto child = t.addChild(root, "c" + std::to_string(i));
    lvl1.push_back(child);
    for (int j = 0; j < 2; ++j) {
      lvl2.push_back(
          t.addChild(child, "l" + std::to_string(i) + std::to_string(j)));
    }
  }
  const auto depths = algo::computeDepths<std::string>(t);
  CHECK_EQ(depths.size(), std::size_t{10});
  CHECK_EQ(depths.at(root), 0);
  for (auto p : lvl1)
    CHECK_EQ(depths.at(p), 1);
  for (auto p : lvl2)
    CHECK_EQ(depths.at(p), 2);
}

/**
 * @brief Asymmetric tree — different branches reach different depths.
 *
 *           "R"            depth 0
 *           /  \.
 *          "A"  "B"        depth 1
 *          /
 *         "C"              depth 2
 *         /
 *        "D"               depth 3
 */
void testAsymmetricTree() {
  Tree t;
  auto R = t.insertRoot("R");
  auto A = t.addChild(R, "A");
  auto B = t.addChild(R, "B");
  auto C = t.addChild(A, "C");
  auto D = t.addChild(C, "D");

  const auto depths = algo::computeDepths<std::string>(t);
  CHECK_EQ(depths.size(), std::size_t{5});
  CHECK_EQ(depths.at(R), 0);
  CHECK_EQ(depths.at(A), 1);
  CHECK_EQ(depths.at(B), 1);
  CHECK_EQ(depths.at(C), 2);
  CHECK_EQ(depths.at(D), 3);
}

/** @brief Every returned depth value must be non-negative. */
void testAllDepthsNonNegative() {
  Tree t;
  auto r = t.insertRoot("r");
  auto a = t.addChild(r, "a");
  t.addChild(a, "b");
  const auto depths = algo::computeDepths<std::string>(t);
  for (const auto &[pos, d] : depths) {
    CHECK_EQ(d >= 0, true);
  }
}

/** @brief Wide tree — 500 leaf children all at depth 1. */
void testWideTree() {
  Tree t;
  auto root = t.insertRoot("root");
  std::vector<Position> leaves;
  leaves.reserve(500);
  for (int i = 0; i < 500; ++i) {
    leaves.push_back(t.addChild(root, std::to_string(i)));
  }
  const auto depths = algo::computeDepths<std::string>(t);
  CHECK_EQ(depths.size(), std::size_t{501});
  CHECK_EQ(depths.at(root), 0);
  for (auto p : leaves) {
    CHECK_EQ(depths.at(p), 1);
  }
}

/**
 * @brief Deep tree (10 000 levels) — verifies no stack overflow and that BFS
 *        correctly assigns depths 0 to 9 999.
 */
void testDeepTree() {
  constexpr int N = 10'000;
  Tree t;
  std::vector<Position> chain;
  chain.reserve(N);
  chain.push_back(t.insertRoot("0"));
  for (int i = 1; i < N; ++i) {
    chain.push_back(t.addChild(chain.back(), std::to_string(i)));
  }
  const auto depths = algo::computeDepths<std::string>(t);
  CHECK_EQ(static_cast<int>(depths.size()), N);
  CHECK_EQ(depths.at(chain.front()), 0);
  CHECK_EQ(depths.at(chain.back()), N - 1);
  CHECK_EQ(depths.at(chain[N / 2]), N / 2); // spot-check middle
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Entry point
// ─────────────────────────────────────────────────────────────────────────────

int main() {
  testEmpty();
  testSingleNode();
  testRootWithTwoChildren();
  testLinearChain();
  testPerfectBinaryTree();
  testTernaryTree();
  testAsymmetricTree();
  testAllDepthsNonNegative();
  testWideTree();
  testDeepTree();

  if (failures == 0) {
    std::cout << "All tests passed.\n";
    return EXIT_SUCCESS;
  }
  std::cerr << failures << " check(s) failed.\n";
  return EXIT_FAILURE;
}
