/**
 * @file node_depths.hpp
 * @brief Algorithm: compute the depth of every node in a general tree in O(n).
 *
 * The *depth* of a node @c v is the number of edges on the unique path from
 * the root down to @c v:
 *   - depth(root) = 0
 *   - depth(v)    = depth(parent(v)) + 1
 *
 * The algorithm propagates depths top-down with a single BFS (level-order)
 * scan, visiting each node exactly once → O(n) time, O(n) space.
 */
#ifndef C76_NODE_DEPTHS_HPP
#define C76_NODE_DEPTHS_HPP

#include <queue>
#include <unordered_map>
#include <utility>

#include "general_tree.hpp"

namespace algo {

/**
 * @brief Compute the depth of every node in @p tree.
 *
 * @par Algorithm (iterative BFS)
 * A FIFO queue stores pairs (Position, depth).  The root is enqueued with
 * depth 0.  For each dequeued node, its depth is recorded, and every child is
 * enqueued with depth + 1.  Because BFS visits nodes level-by-level, a
 * parent is always dequeued before any of its children, so the parent's depth
 * is already known when the child is enqueued.
 *
 * @par Complexity
 * - Time  : O(n) — each node is enqueued and dequeued exactly once.
 * - Space : O(n) — queue holds at most one full level; map holds n entries.
 *
 * @tparam T  Element type of the tree.
 * @param  tree  Tree to inspect (may be empty).
 * @return An unordered_map mapping each Position to its depth.
 *         Returns an empty map for an empty tree.
 */
template <typename T>
[[nodiscard]] std::unordered_map<typename adt::GeneralTree<T>::Position, int>
computeDepths(const adt::GeneralTree<T> &tree) {
  using Position = typename adt::GeneralTree<T>::Position;
  using Entry    = std::pair<Position, int>; // (node, depth)

  std::unordered_map<Position, int> depthOf;
  depthOf.reserve(tree.size());

  if (tree.isEmpty()) {
    return depthOf;
  }

  std::queue<Entry> bfsQueue;
  bfsQueue.push({tree.root(), 0});

  while (!bfsQueue.empty()) {
    auto [p, d] = bfsQueue.front();
    bfsQueue.pop();

    depthOf[p] = d;

    for (const auto &child : tree.children(p)) {
      bfsQueue.push({child.get(), d + 1});
    }
  }

  return depthOf;
}

} // namespace algo

#endif // C76_NODE_DEPTHS_HPP
