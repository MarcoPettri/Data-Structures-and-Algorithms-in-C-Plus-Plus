// File : R-7.1.cpp

/*
    R-7.1 Describe an algorithm for counting the number of left external nodes
   in a binary tree, using the Binary tree ADT.

Solution:

    Method: Explicit Stack Traversal (Iterative DFS):

    1. Avoid recursion to prevent stack overflow on deep trees by using an explicit stack that stores pairs of node positions and boolean flags (`isLeftChild`).
    2. Initialize the stack with the tree's root node (flagged as `false` since the root has no parent), and perform an iterative Depth-First Search (DFS).
    3. Pop nodes from the stack: if a node is an external node (leaf) and `isLeftChild` is true, increment the counter; otherwise, push its right child (with `false`) and left child (with `true`) onto the stack.

    Complexity Analysis:
        * Running time: O(n) — visits every node in the binary tree exactly once.
        * Space usage: O(h) auxiliary space, where h is the tree height, bounded by the maximum stack depth.

*/
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <utility>

#include "left_external.hpp"

namespace {

int failures = 0;

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

using Tree = adt::BinaryTree<int>;

void testEmpty() {
  Tree t;
  CHECK_EQ(algo::countLeftExternal(t), std::size_t{0});
}

void testRootOnly() {
  Tree t;
  t.insertRoot(1);
  CHECK_EQ(algo::countLeftExternal(t),
           std::size_t{0}); // root is not a left child
}

void testOnlyLeftLeaf() {
  Tree t;
  auto r = t.insertRoot(1);
  t.insertLeft(r, 2);
  CHECK_EQ(algo::countLeftExternal(t), std::size_t{1});
}

void testOnlyRightLeaf() {
  Tree t;
  auto r = t.insertRoot(1);
  t.insertRight(r, 2);
  CHECK_EQ(algo::countLeftExternal(t), std::size_t{0});
}

void testPerfectTree7() {
  //         1
  //       /   \
  //      2     3
  //     / \   / \
  //    4   5 6   7      left leaves: 4, 6
  Tree t;
  auto n1 = t.insertRoot(1);
  auto n2 = t.insertLeft(n1, 2);
  auto n3 = t.insertRight(n1, 3);
  t.insertLeft(n2, 4);
  t.insertRight(n2, 5);
  t.insertLeft(n3, 6);
  t.insertRight(n3, 7);
  CHECK_EQ(algo::countLeftExternal(t), std::size_t{2});
}

void testLeftSkewed() {
  Tree t;
  auto p = t.insertRoot(0);
  for (int i = 1; i <= 5; ++i)
    p = t.insertLeft(p, i);
  CHECK_EQ(algo::countLeftExternal(t), std::size_t{1}); // only the deepest node
}

void testRightSkewed() {
  Tree t;
  auto p = t.insertRoot(0);
  for (int i = 1; i <= 5; ++i)
    p = t.insertRight(p, i);
  CHECK_EQ(algo::countLeftExternal(t), std::size_t{0});
}

void testMixed() {
  //        1
  //       / \
  //      2   3        2 has only a right leaf (5): not counted
  //       \   \       3 has only a right child 6, which has left leaf 7:
  //       counted
  //        5   6
  //           /
  //          7
  Tree t;
  auto n1 = t.insertRoot(1);
  auto n2 = t.insertLeft(n1, 2);
  auto n3 = t.insertRight(n1, 3);
  t.insertRight(n2, 5);
  auto n6 = t.insertRight(n3, 6);
  t.insertLeft(n6, 7);
  CHECK_EQ(algo::countLeftExternal(t), std::size_t{1});
}

void testMoveKeepsResult() {
  Tree a;
  auto r = a.insertRoot(1);
  a.insertLeft(r, 2);
  Tree b(std::move(a));
  CHECK_EQ(algo::countLeftExternal(b), std::size_t{1});
  CHECK_EQ(algo::countLeftExternal(a), std::size_t{0}); // moved-from is empty
}

void testVeryDeepTree() {
  // A recursive traversal or recursive destructor would likely overflow here.
  Tree t;
  auto p = t.insertRoot(0);
  for (int i = 1; i <= 1'000'000; ++i)
    p = t.insertLeft(p, i);
  CHECK_EQ(algo::countLeftExternal(t), std::size_t{1});
} // t is destroyed here via the iterative clear()

} // namespace

int main() {
  testEmpty();
  testRootOnly();
  testOnlyLeftLeaf();
  testOnlyRightLeaf();
  testPerfectTree7();
  testLeftSkewed();
  testRightSkewed();
  testMixed();
  testMoveKeepsResult();
  testVeryDeepTree();

  if (failures == 0) {
    std::cout << "All tests passed.\n";
    return EXIT_SUCCESS;
  }
  std::cerr << failures << " check(s) failed.\n";
  return EXIT_FAILURE;
}