/**
 * @file diameter.hpp
 * @brief Diameter (longest path length) of an adt::GeneralTree.
 */
#ifndef C7_DIAMETER_HPP
#define C7_DIAMETER_HPP

#include <algorithm>
#include <cstddef>
#include <vector>

#include "general_tree.hpp"

namespace adt {

/**
 * @brief Diameter of @p tree: the maximum distance between two of its nodes.
 *
 * The distance between nodes v and w is `d_v + d_w - 2 * d_u`, where d_x is
 * the depth of x and u is the lowest common ancestor of v and w; that is the
 * number of edges on the path joining them.
 *
 * Algorithm: one iterative post-order traversal. For every node u, the
 * longest path whose topmost node is u has length
 * `best1 + best2`, where best1 >= best2 are the two largest values of
 * `height(child) + 1` over the children of u (missing values count as 0).
 * The diameter is the maximum of that quantity over all nodes. The height of
 * u itself is best1, which is passed up to u's parent.
 *
 * @tparam T Element type of the tree.
 * @param tree Tree to analyse.
 * @return Diameter measured in edges: 0 for an empty tree or a single node.
 *
 * @par Complexity
 * - Time:  Theta(n): every node is pushed and popped once and every child
 *          link is read once.
 * - Space: O(h) for the explicit stack, h = height of @p tree. Iterative, so
 *          arbitrarily deep trees cannot overflow the call stack.
 */
template <typename T> std::size_t diameter(const GeneralTree<T> &tree) {
  using Position = typename GeneralTree<T>::Position;

  /// One level of the current root-to-node path.
  struct Frame {
    Position node;
    std::size_t nextChild; ///< index of the next child to explore
    std::size_t best1;     ///< largest height(child) + 1 seen so far
    std::size_t best2;     ///< second largest height(child) + 1 seen so far
  };

  if (tree.isEmpty()) {
    return 0;
  }

  std::size_t result = 0;
  std::vector<Frame> path;
  path.push_back(Frame{tree.root(), 0, 0, 0});

  while (!path.empty()) {
    Frame &top = path.back();
    const auto &kids = tree.children(top.node);

    if (top.nextChild < kids.size()) {
      Position child = kids[top.nextChild++].get();
      path.push_back(Frame{child, 0, 0, 0}); // invalidates `top`
      continue;
    }

    // All children done: top.best1 is height(top.node), and the longest path
    // with top.node as its highest node has top.best1 + top.best2 edges.
    const std::size_t height = top.best1;
    result = std::max(result, top.best1 + top.best2);
    path.pop_back();

    if (!path.empty()) { // report height + 1 to the parent
      Frame &parent = path.back();
      const std::size_t up = height + 1;
      if (up > parent.best1) {
        parent.best2 = parent.best1;
        parent.best1 = up;
      } else if (up > parent.best2) {
        parent.best2 = up;
      }
    }
  }
  return result;
}

} // namespace adt

#endif // C7_DIAMETER_HPP