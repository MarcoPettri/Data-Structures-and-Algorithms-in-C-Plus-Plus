// File : C-7.12.cpp

/*
    C-7.12 Two ordered trees T′ and T″ are said to be isomorphic if one of the
   following holds: • Both T′ and T″ consist of a single node, or • Both T′ and
   T″ have the same number k of subtrees, and the ith subtree of T′ is
   isomorphic to the ith subtree of T″, for i = 1, . . . , k. Design an
   algorithm that tests whether two given ordered trees are iso- morphic. What
   is the running time of your algorithm?

   ─────────────────────────────────────────────────────────────────────────────
   Solution
   ─────────────────────────────────────────────────────────────────────────────

   Algorithm (Iterative Simultaneous Depth-First Search):
     To avoid call-stack overflow on arbitrarily deep trees (e.g., 10,000+
   levels), the algorithm uses an explicit stack of position pairs (p1, p2):

       1. Handle empty cases:
          - If both trees are empty, return true.
          - If exactly one tree is empty, return false.
       2. Push (T′.root(), T″.root()) onto an explicit stack.
       3. While the stack is non-empty:
          a. Pop (p1, p2).
          b. Let c1 = children(p1) and c2 = children(p2).
          c. If |c1| ≠ |c2|, return false (mismatched number of subtrees).
          d. Push matching child pairs (c1[i], c2[i]) for i from |c1|-1 down to
   0 so that earlier children are processed first (standard DFS order).
       4. If the stack is emptied without encountering any discrepancy,
          return true.

   Correctness:
     - Base case: If both consist of a single node, |c1| = |c2| = 0; no pairs
       are pushed, the loop finishes, and the algorithm returns true.
     - Inductive step: By induction on tree size, if both roots have k children,
       each child pair (c1[i], c2[i]) is visited in order. A mismatch in child
   count at any corresponding node triggers an immediate return of false. If all
       corresponding nodes match in degree and ordering, every node in T′ is
   mapped one-to-one to T″ preserving child order, returning true.

   Complexity Analysis:
     Let n = |T′| and m = |T″|.
     * Running time : O(min(n, m))
       - When the trees are isomorphic: n = m, and every node is visited,
   pushed, and popped exactly once. The degrees sum to n - 1 edges, taking O(n)
   time.
       - When the trees are not isomorphic: The algorithm terminates immediately
         upon encountering the first structural mismatch in preorder. In the
   best case (e.g. roots have different degrees), this takes O(1) time. In the
         worst non-isomorphic case, it visits at most min(n, m) nodes → O(min(n,
   m)).
     * Space usage  : O(h) where h is the maximum height of the traversed tree
       portion, bounded by O(min(n, m)) in the worst case (e.g., linear chain or
       flat tree where stack holds all children of a node).
*/

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "general_tree.hpp"
#include "tree_isomorphism.hpp"

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

using TreeStr = adt::GeneralTree<std::string>;
using TreeInt = adt::GeneralTree<int>;

// ─────────────────────────────────────────────────────────────────────────────
// Test cases
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Two empty trees are isomorphic. */
void testBothEmpty() {
  TreeStr t1;
  TreeStr t2;
  CHECK_EQ(algo::areIsomorphic(t1, t2), true);
}

/** @brief An empty tree and a non-empty tree are not isomorphic. */
void testOneEmpty() {
  TreeStr t1;
  TreeStr t2;
  t2.insertRoot("root");
  CHECK_EQ(algo::areIsomorphic(t1, t2), false);
  CHECK_EQ(algo::areIsomorphic(t2, t1), false);
}

/** @brief Both trees consist of a single node (base case). */
void testSingleNodes() {
  TreeStr t1;
  t1.insertRoot("A");

  TreeStr t2;
  t2.insertRoot("B");

  CHECK_EQ(algo::areIsomorphic(t1, t2), true);
}

/** @brief Single node vs root with children. */
void testSingleNodeVsMultiNode() {
  TreeStr t1;
  t1.insertRoot("A");

  TreeStr t2;
  auto r2 = t2.insertRoot("A");
  t2.addChild(r2, "B");

  CHECK_EQ(algo::areIsomorphic(t1, t2), false);
  CHECK_EQ(algo::areIsomorphic(t2, t1), false);
}

/**
 * @brief Identical tree structures.
 *
 * Tree:       root
 *            /    \
 *         child1 child2
 */
void testIdenticalTrees() {
  TreeStr t1;
  auto r1 = t1.insertRoot("R1");
  t1.addChild(r1, "C1");
  t1.addChild(r1, "C2");

  TreeStr t2;
  auto r2 = t2.insertRoot("R2");
  t2.addChild(r2, "C1");
  t2.addChild(r2, "C2");

  CHECK_EQ(algo::areIsomorphic(t1, t2), true);
}

/**
 * @brief Structural isomorphism holds even with different element types and
 * values.
 *
 * t1 stores std::string, t2 stores int.
 * Both have shape: root -> 2 children, 2nd child has 1 child.
 */
void testDifferentElementTypes() {
  TreeStr t1;
  auto r1 = t1.insertRoot("Alpha");
  t1.addChild(r1, "Beta");
  auto c2 = t1.addChild(r1, "Gamma");
  t1.addChild(c2, "Delta");

  TreeInt t2;
  auto r2 = t2.insertRoot(100);
  t2.addChild(r2, 200);
  auto d2 = t2.addChild(r2, 300);
  t2.addChild(d2, 400);

  CHECK_EQ(algo::areIsomorphic(t1, t2), true);
}

/**
 * @brief Same total number of nodes (4 nodes), but different shapes.
 *
 * T1: root has 2 children; child 0 has 1 child.
 * T2: root has 1 child; that child has 2 children.
 */
void testSameNodeCountDifferentShapes() {
  TreeStr t1;
  auto r1 = t1.insertRoot("R");
  auto a = t1.addChild(r1, "A");
  t1.addChild(r1, "B");
  t1.addChild(a, "C");

  TreeStr t2;
  auto r2 = t2.insertRoot("R");
  auto x = t2.addChild(r2, "X");
  t2.addChild(x, "Y");
  t2.addChild(x, "Z");

  CHECK_EQ(algo::areIsomorphic(t1, t2), false);
}

/**
 * @brief Ordered tree property: child order matters.
 *
 * T1: root has 2 children (C0, C1); C0 has 1 child, C1 has 0.
 * T2: root has 2 children (C0, C1); C0 has 0 children, C1 has 1.
 *
 * For unordered trees these would be isomorphic, but for ordered trees
 * subtree 1 of T1 is NOT isomorphic to subtree 1 of T2 -> false.
 */
void testOrderedDifference() {
  TreeStr t1;
  auto r1 = t1.insertRoot("R");
  auto c1_0 = t1.addChild(r1, "C0");
  t1.addChild(r1, "C1");
  t1.addChild(c1_0, "Leaf");

  TreeStr t2;
  auto r2 = t2.insertRoot("R");
  t2.addChild(r2, "C0");
  auto c2_1 = t2.addChild(r2, "C1");
  t2.addChild(c2_1, "Leaf");

  CHECK_EQ(algo::areIsomorphic(t1, t2), false);
}

/**
 * @brief Multi-way trees with 3 children each, identical structure.
 *
 *             R
 *          /  |  \
 *         A   B   C
 *        / \      |
 *       D   E     F
 */
void testMultiWayTrees() {
  TreeStr t1;
  auto r1 = t1.insertRoot("R");
  auto a1 = t1.addChild(r1, "A");
  t1.addChild(r1, "B");
  auto c1 = t1.addChild(r1, "C");
  t1.addChild(a1, "D");
  t1.addChild(a1, "E");
  t1.addChild(c1, "F");

  TreeStr t2;
  auto r2 = t2.insertRoot("Root");
  auto a2 = t2.addChild(r2, "1");
  t2.addChild(r2, "2");
  auto c2 = t2.addChild(r2, "3");
  t2.addChild(a2, "4");
  t2.addChild(a2, "5");
  t2.addChild(c2, "6");

  CHECK_EQ(algo::areIsomorphic(t1, t2), true);
}

/**
 * @brief Multi-way trees with subtle difference deep down in the 3rd child.
 */
void testMultiWayTreesSubtleDifference() {
  TreeStr t1;
  auto r1 = t1.insertRoot("R");
  auto a1 = t1.addChild(r1, "A");
  t1.addChild(r1, "B");
  auto c1 = t1.addChild(r1, "C");
  t1.addChild(a1, "D");
  t1.addChild(a1, "E");
  t1.addChild(c1, "F"); // 1 child under C

  TreeStr t2;
  auto r2 = t2.insertRoot("R");
  auto a2 = t2.addChild(r2, "A");
  t2.addChild(r2, "B");
  auto c2 = t2.addChild(r2, "C");
  t2.addChild(a2, "D");
  t2.addChild(a2, "E");
  t2.addChild(c2, "F1");
  t2.addChild(c2, "F2"); // 2 children under C

  CHECK_EQ(algo::areIsomorphic(t1, t2), false);
}

/**
 * @brief Direct tests for areSubtreesIsomorphic on subtrees and null handles.
 */
void testSubtreesIsomorphic() {
  TreeStr t1;
  auto r1 = t1.insertRoot("R");
  auto sub1 = t1.addChild(r1, "S1");
  t1.addChild(sub1, "L1");

  TreeStr t2;
  auto r2 = t2.insertRoot("R");
  auto sub2 = t2.addChild(r2, "S2");
  t2.addChild(sub2, "L2");

  // Both valid isomorphic subtrees
  CHECK_EQ(algo::areSubtreesIsomorphic(t1, sub1, t2, sub2), true);

  // Both null subtrees
  CHECK_EQ(algo::areSubtreesIsomorphic(t1, nullptr, t2, nullptr), true);

  // One null, one non-null
  CHECK_EQ(algo::areSubtreesIsomorphic(t1, sub1, t2, nullptr), false);
  CHECK_EQ(algo::areSubtreesIsomorphic(t1, nullptr, t2, sub2), false);
}

/**
 * @brief Wide trees: root with 500 children.
 */
void testWideTrees() {
  TreeStr t1;
  auto r1 = t1.insertRoot("R");
  for (int i = 0; i < 500; ++i) {
    t1.addChild(r1, std::to_string(i));
  }

  TreeStr t2;
  auto r2 = t2.insertRoot("R");
  for (int i = 0; i < 500; ++i) {
    t2.addChild(r2, std::to_string(i));
  }

  CHECK_EQ(algo::areIsomorphic(t1, t2), true);

  // Add one more child to t2 -> 500 vs 501
  t2.addChild(r2, "extra");
  CHECK_EQ(algo::areIsomorphic(t1, t2), false);
}

/**
 * @brief Deep tree (10 000 levels) — verifies no call-stack overflow and
 *        correctness on deep structures.
 */
void testDeepTrees() {
  constexpr int N = 10'000;

  TreeStr t1;
  auto cur1 = t1.insertRoot("0");
  for (int i = 1; i < N; ++i) {
    cur1 = t1.addChild(cur1, std::to_string(i));
  }

  TreeStr t2;
  auto cur2 = t2.insertRoot("0");
  for (int i = 1; i < N; ++i) {
    cur2 = t2.addChild(cur2, std::to_string(i));
  }

  // Exactly identical depth
  CHECK_EQ(algo::areIsomorphic(t1, t2), true);

  // Extend t2 by one node -> N vs N+1
  t2.addChild(cur2, "extra");
  CHECK_EQ(algo::areIsomorphic(t1, t2), false);
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Entry point
// ─────────────────────────────────────────────────────────────────────────────

int main() {
  testBothEmpty();
  testOneEmpty();
  testSingleNodes();
  testSingleNodeVsMultiNode();
  testIdenticalTrees();
  testDifferentElementTypes();
  testSameNodeCountDifferentShapes();
  testOrderedDifference();
  testMultiWayTrees();
  testMultiWayTreesSubtleDifference();
  testSubtreesIsomorphic();
  testWideTrees();
  testDeepTrees();

  if (failures == 0) {
    std::cout << "All tests passed.\n";
    return EXIT_SUCCESS;
  }
  std::cerr << failures << " check(s) failed.\n";
  return EXIT_FAILURE;
}
