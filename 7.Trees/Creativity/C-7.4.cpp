// File : C-7.4.cpp

/*
    C-7.4 Let T be a tree whose nodes store strings. Give an algorithm that
    computes and prints, for every internal node v of T, the string stored at v
    and the height of the subtree rooted at v.

   ─────────────────────────────────────────────────────────────────────────────
   Solution
   ─────────────────────────────────────────────────────────────────────────────

   Definitions:
     * Internal node — a node with at least one child.
     * Height of a subtree rooted at v — the number of edges on the longest
       downward path from v to a leaf:
         height(leaf)  = 0
         height(v)     = 1 + max{ height(c) : c is a child of v }

   Algorithm (single-pass, iterative post-order):

     Post-order traversal ensures a node is processed *after* all its
     descendants, so when we reach an internal node v we already know the
     heights of all its children.

     Two-stack technique to simulate post-order without recursion:
       1. Push root onto Stack_1.
       2. While Stack_1 is non-empty:
            pop p from Stack_1 → push p onto Stack_2.
            push all children of p onto Stack_1 (left-to-right).
       3. Stack_2 now contains nodes in reverse post-order.
          While Stack_2 is non-empty:
            pop p.
            if p is a leaf        → height[p] = 0  (no output).
            if p is internal      → height[p] = 1 + max(height[c] for c in
   children(p)). Print (element(p), height[p]).

   Complexity Analysis:
     * Running time : O(n) — every node is pushed/popped twice.
     * Space usage  : O(n) — two stacks + height map, all bounded by n.
*/

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

#include "general_tree.hpp"
#include "subtree_heights.hpp"

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

#define CHECK_CONTAINS(haystack, needle)                                       \
  do {                                                                         \
    const std::string h_ = (haystack);                                         \
    const std::string n_ = (needle);                                           \
    if (h_.find(n_) == std::string::npos) {                                    \
      ++failures;                                                              \
      std::cerr << "FAIL line " << __LINE__ << ": output does not contain \""  \
                << n_ << "\"\nFull output:\n"                                  \
                << h_ << '\n';                                                 \
    }                                                                          \
  } while (0)

#define CHECK_NOT_CONTAINS(haystack, needle)                                   \
  do {                                                                         \
    const std::string h_ = (haystack);                                         \
    const std::string n_ = (needle);                                           \
    if (h_.find(n_) != std::string::npos) {                                    \
      ++failures;                                                              \
      std::cerr << "FAIL line " << __LINE__                                    \
                << ": output unexpectedly contains \"" << n_                   \
                << "\"\nFull output:\n"                                        \
                << h_ << '\n';                                                 \
    }                                                                          \
  } while (0)
/// @endcond

using Tree = adt::GeneralTree<std::string>;

// ─────────────────────────────────────────────────────────────────────────────
// Helper: run printInternalHeights and capture output as a string
// ─────────────────────────────────────────────────────────────────────────────

std::string capture(const Tree &t) {
  std::ostringstream oss;
  algo::printInternalHeights(t, oss);
  return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// Test cases
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Empty tree produces no output. */
void testEmpty() {
  Tree t;
  CHECK_EQ(capture(t), std::string{});
}

/**
 * @brief Single-node tree — the root is a leaf, so nothing is printed.
 *
 * Tree:  "root"   (leaf, no children)
 */
void testSingleLeaf() {
  Tree t;
  t.insertRoot("root");
  CHECK_EQ(capture(t), std::string{});
}

/**
 * @brief Root with two leaf children.
 *
 * Tree:   "A"
 *         / \.
 *        "B" "C"
 *
 * Expected: A is internal with height 1.
 */
void testRootWithTwoChildren() {
  Tree t;
  auto r = t.insertRoot("A");
  t.addChild(r, "B");
  t.addChild(r, "C");
  const std::string out = capture(t);
  CHECK_CONTAINS(out, "\"A\"");
  CHECK_CONTAINS(out, "subtree height: 1");
  CHECK_NOT_CONTAINS(out, "\"B\"");
  CHECK_NOT_CONTAINS(out, "\"C\"");
}

/**
 * @brief Linear chain (path graph), depth 3.
 *
 * Tree:  "R" → "A" → "B" → "C"
 *
 * Heights: R=3, A=2, B=1; C is a leaf (not printed).
 */
void testLinearChain() {
  Tree t;
  auto r = t.insertRoot("R");
  auto a = t.addChild(r, "A");
  auto b = t.addChild(a, "B");
  t.addChild(b, "C");
  const std::string out = capture(t);
  CHECK_CONTAINS(out, "\"R\"");
  CHECK_CONTAINS(out, "subtree height: 3");
  CHECK_CONTAINS(out, "\"A\"");
  CHECK_CONTAINS(out, "subtree height: 2");
  CHECK_CONTAINS(out, "\"B\"");
  CHECK_CONTAINS(out, "subtree height: 1");
  CHECK_NOT_CONTAINS(out, "\"C\""); // leaf
}

/**
 * @brief Unbalanced tree — left subtree is deeper than right.
 *
 * Tree:
 *         "root"          h=3
 *         /    \.
 *       "L"   "R"         L: h=2, R: leaf (h=0, not printed)
 *       / \.
 *     "LL" "LR"           both leaves (not printed)
 *     /
 *   "LLL"                 leaf (not printed)
 */
void testUnbalancedTree() {
  Tree t;
  auto root = t.insertRoot("root");
  auto L = t.addChild(root, "L");
  t.addChild(root, "R");
  auto LL = t.addChild(L, "LL");
  t.addChild(L, "LR");
  t.addChild(LL, "LLL");

  const std::string out = capture(t);
  CHECK_CONTAINS(out, "\"root\"");
  CHECK_CONTAINS(out, "\"L\"");
  CHECK_CONTAINS(out, "\"LL\"");
  CHECK_CONTAINS(out, "subtree height: 3");
  CHECK_CONTAINS(out, "subtree height: 2");
  CHECK_CONTAINS(out, "subtree height: 1");
  CHECK_NOT_CONTAINS(out, "\"R\"");
  CHECK_NOT_CONTAINS(out, "\"LR\"");
  CHECK_NOT_CONTAINS(out, "\"LLL\"");
}

/**
 * @brief Ternary tree (each internal node has 3 children).
 *
 * Tree:
 *         "root"               h=2
 *        /  |  \.
 *      "A" "B" "C"            each h=1
 *       |   |   |
 *      "a" "b" "c"            leaves (not printed)
 */
void testTernaryTree() {
  Tree t;
  auto root = t.insertRoot("root");
  for (const char *name : {"A", "B", "C"}) {
    auto child = t.addChild(root, name);
    t.addChild(child, std::string(1, static_cast<char>(name[0] + 32)));
  }
  const std::string out = capture(t);
  CHECK_CONTAINS(out, "\"root\"");
  CHECK_CONTAINS(out, "subtree height: 2");
  CHECK_CONTAINS(out, "\"A\"");
  CHECK_CONTAINS(out, "\"B\"");
  CHECK_CONTAINS(out, "\"C\"");
  CHECK_CONTAINS(out, "subtree height: 1");
  CHECK_NOT_CONTAINS(out, "\"a\"");
  CHECK_NOT_CONTAINS(out, "\"b\"");
  CHECK_NOT_CONTAINS(out, "\"c\"");
}

/**
 * @brief Perfect binary tree with 7 nodes.
 *
 *           "1"             h=2
 *          /   \.
 *        "2"   "3"          h=1 each
 *       / \   / \.
 *      4   5 6   7          leaves (not printed)
 */
void testPerfectBinaryTree() {
  Tree t;
  auto n1 = t.insertRoot("1");
  auto n2 = t.addChild(n1, "2");
  auto n3 = t.addChild(n1, "3");
  t.addChild(n2, "4");
  t.addChild(n2, "5");
  t.addChild(n3, "6");
  t.addChild(n3, "7");

  const std::string out = capture(t);
  CHECK_CONTAINS(out, "\"1\"");
  CHECK_CONTAINS(out, "subtree height: 2");
  CHECK_CONTAINS(out, "\"2\"");
  CHECK_CONTAINS(out, "\"3\"");

  // Count how many times "subtree height: 1" appears (should be exactly 2).
  std::size_t pos = 0, count = 0;
  const std::string needle = "subtree height: 1";
  while ((pos = out.find(needle, pos)) != std::string::npos) {
    ++count;
    pos += needle.size();
  }
  CHECK_EQ(count, std::size_t{2});

  for (const char *leaf : {"\"4\"", "\"5\"", "\"6\"", "\"7\""}) {
    CHECK_NOT_CONTAINS(out, leaf);
  }
}

/**
 * @brief Deep tree (1 000 levels) — verifies no stack overflow in the
 *        iterative implementation.
 */
void testDeepTree() {
  Tree t;
  auto cur = t.insertRoot("node_0");
  for (int i = 1; i < 1'000; ++i) {
    cur = t.addChild(cur, "node_" + std::to_string(i));
  }
  // Root has subtree height 999.
  const std::string out = capture(t);
  CHECK_CONTAINS(out, "\"node_0\"");
  CHECK_CONTAINS(out, "subtree height: 999");
}

/**
 * @brief Wide tree (1 000 children of root) — root is the only internal node.
 */
void testWideTree() {
  Tree t;
  auto root = t.insertRoot("root");
  for (int i = 0; i < 1'000; ++i) {
    t.addChild(root, "leaf_" + std::to_string(i));
  }
  const std::string out = capture(t);
  CHECK_CONTAINS(out, "\"root\"");
  CHECK_CONTAINS(out, "subtree height: 1");
  CHECK_NOT_CONTAINS(out, "\"leaf_");
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Entry point
// ─────────────────────────────────────────────────────────────────────────────

int main() {
  testEmpty();
  testSingleLeaf();
  testRootWithTwoChildren();
  testLinearChain();
  testUnbalancedTree();
  testTernaryTree();
  testPerfectBinaryTree();
  testDeepTree();
  testWideTree();

  if (failures == 0) {
    std::cout << "All tests passed.\n";
    return EXIT_SUCCESS;
  }
  std::cerr << failures << " check(s) failed.\n";
  return EXIT_FAILURE;
}
