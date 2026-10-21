/**
 * @file left_external.hpp
 * @brief Algorithm: count the left external nodes of a binary tree (iterative).
 */
#ifndef ALGO_LEFT_EXTERNAL_HPP
#define ALGO_LEFT_EXTERNAL_HPP

#include <cstddef>
#include <stack>
#include <utility>

#include "binary_tree.hpp"

namespace algo {

/**
 * @brief Counts the left external nodes (leaves that are left children).
 *
 * Uses an explicit stack instead of recursion, so it is safe for very deep
 * trees.
 *
 * @tparam T Element type of the tree.
 * @param tree Tree to inspect.
 * @return The number of left external nodes; 0 for an empty tree or a lone
 * root.
 *
 * @par Complexity
 * Time O(n); auxiliary space O(h) on the heap, with n the number of nodes
 * and h the height.
 */
template <typename T>
std::size_t countLeftExternal(const adt::BinaryTree<T> &tree) {
  using Position = typename adt::BinaryTree<T>::Position;

  if (tree.isEmpty())
    return 0;

  std::size_t count = 0;
  std::stack<std::pair<Position, bool>> pending; // (node, isLeftChild)
  pending.push({tree.root(), false});

  while (!pending.empty()) {
    const auto [p, isLeft] = pending.top();
    pending.pop();

    if (tree.isExternal(p)) {
      if (isLeft)
        ++count;
      continue;
    }
    if (Position r = tree.right(p))
      pending.push({r, false});
    if (Position l = tree.left(p))
      pending.push({l, true});
  }
  return count;
}

} // namespace algo

#endif // ALGO_LEFT_EXTERNAL_HPP