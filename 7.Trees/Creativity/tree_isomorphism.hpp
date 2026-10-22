/**
 * @file tree_isomorphism.hpp
 * @brief Algorithm: test whether two ordered trees are isomorphic.
 *
 * Two ordered trees T' and T'' are isomorphic if one of the following holds:
 *   1. Both T' and T'' consist of a single node, or
 *   2. Both T' and T'' have the same number k of subtrees, and the i-th
 *      subtree of T' is isomorphic to the i-th subtree of T'', for i = 1, ..., k.
 *
 * Tree isomorphism is purely structural: the elements/labels stored at nodes
 * are not compared (the two trees may even store different data types).
 *
 * All traversals are performed iteratively using an explicit stack to prevent
 * call-stack overflow on deep trees.
 */
#ifndef TREE_ISOMORPHISM_HPP
#define TREE_ISOMORPHISM_HPP

#include <cstddef>
#include <utility>
#include <vector>

#include "general_tree.hpp"

namespace algo {

/**
 * @brief Tests whether the subtrees rooted at @p root1 and @p root2 are isomorphic.
 *
 * Two ordered subtrees are isomorphic if and only if:
 * - Both are null (or empty), OR
 * - Neither is null, both roots have the same number of children k, and the
 *   i-th child subtree of root1 is isomorphic to the i-th child subtree of root2
 *   for every i in {0, ..., k - 1}.
 *
 * @par Algorithm (Iterative simultaneous DFS)
 * An explicit stack of pairs (Position1, Position2) is maintained.
 * At each step, a pair (p1, p2) is popped:
 * 1. Compare the number of children: |children(p1)| == |children(p2)|.
 *    If they differ, return false immediately.
 * 2. Push corresponding child pairs (child1[i], child2[i]) onto the stack.
 *    Pushing right-to-left ensures left-to-right traversal order.
 * If the stack becomes empty without mismatch, the subtrees are isomorphic.
 *
 * @par Complexity
 * - Time:  O(min(n, m)) where n and m are the sizes of the two subtrees.
 *          If isomorphic, n == m and time is O(n). If not isomorphic,
 *          the algorithm aborts at the first structural discrepancy,
 *          taking O(1) in the best case.
 * - Space: O(h) where h is the maximum height of the traversed subtrees
 *          (O(min(n, m)) in the worst case for skewed or flat trees).
 *
 * @tparam T1 Element type stored in tree1.
 * @tparam T2 Element type stored in tree2.
 * @param  tree1  First tree.
 * @param  root1  Root position of the subtree in tree1 (may be nullptr).
 * @param  tree2  Second tree.
 * @param  root2  Root position of the subtree in tree2 (may be nullptr).
 * @return true if the two subtrees are isomorphic; false otherwise.
 */
template <typename T1, typename T2>
[[nodiscard]] bool areSubtreesIsomorphic(
    const adt::GeneralTree<T1> &tree1,
    typename adt::GeneralTree<T1>::Position root1,
    const adt::GeneralTree<T2> &tree2,
    typename adt::GeneralTree<T2>::Position root2) {
  using Pos1 = typename adt::GeneralTree<T1>::Position;
  using Pos2 = typename adt::GeneralTree<T2>::Position;

  if (root1 == nullptr && root2 == nullptr) {
    return true;
  }
  if (root1 == nullptr || root2 == nullptr) {
    return false;
  }

  std::vector<std::pair<Pos1, Pos2>> stack;
  stack.emplace_back(root1, root2);

  while (!stack.empty()) {
    auto [p1, p2] = stack.back();
    stack.pop_back();

    const auto &c1 = tree1.children(p1);
    const auto &c2 = tree2.children(p2);

    if (c1.size() != c2.size()) {
      return false;
    }

    // Push children in reverse order so the 1st child is processed first
    for (std::size_t i = c1.size(); i > 0; --i) {
      stack.emplace_back(c1[i - 1].get(), c2[i - 1].get());
    }
  }

  return true;
}

/**
 * @brief Tests whether two ordered trees are isomorphic.
 *
 * Two ordered trees T' and T'' are isomorphic if one of the following holds:
 * - Both consist of a single node, or
 * - Both have the same number k of subtrees, and the i-th subtree of T' is
 *   isomorphic to the i-th subtree of T'', for i = 1, ..., k.
 *
 * Empty trees are treated as isomorphic to each other, and not isomorphic to
 * any non-empty tree.
 *
 * @par Complexity
 * - Time:  O(min(n, m)) where n = tree1.size() and m = tree2.size().
 *          O(n) when trees are isomorphic. Best-case O(1) on early mismatch.
 * - Space: O(h) where h is the maximum depth (bounded by O(min(n, m))).
 *
 * @tparam T1 Element type stored in tree1.
 * @tparam T2 Element type stored in tree2.
 * @param  tree1  First tree.
 * @param  tree2  Second tree.
 * @return true if tree1 and tree2 are isomorphic; false otherwise.
 */
template <typename T1, typename T2>
[[nodiscard]] bool areIsomorphic(const adt::GeneralTree<T1> &tree1,
                                 const adt::GeneralTree<T2> &tree2) {
  return areSubtreesIsomorphic(tree1, tree1.root(), tree2, tree2.root());
}

} // namespace algo

#endif // TREE_ISOMORPHISM_HPP
