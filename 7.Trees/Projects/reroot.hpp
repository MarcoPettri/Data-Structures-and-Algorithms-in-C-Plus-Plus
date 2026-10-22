/**
 * @file reroot.hpp
 * @brief Re-rooting of an adt::GeneralTree at an arbitrary node.
 */
#ifndef C7_REROOT_HPP
#define C7_REROOT_HPP

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

#include "../Creativity/general_tree.hpp"

namespace adt {

namespace detail {

/**
 * @brief Path from the root of @p tree down to @p target, inclusive.
 *
 * Iterative DFS whose explicit stack always holds the root-to-current-node
 * path, so when @p target is reached the stack is the answer.
 *
 * @return The path (front = root, back = @p target), or an empty vector if
 *         @p target is not a node of @p tree.
 *
 * @par Complexity
 * O(n) time, O(h) space.
 */
template <typename T>
std::vector<typename GeneralTree<T>::Position>
rootPath(const GeneralTree<T> &tree, typename GeneralTree<T>::Position target) {
  using Position = typename GeneralTree<T>::Position;

  /// One level of the current root-to-node path.
  struct Frame {
    Position node;
    std::size_t nextChild; ///< index of the next child to explore
  };

  std::vector<Frame> stack;
  if (!tree.isEmpty()) {
    stack.push_back(Frame{tree.root(), 0});
  }
  while (!stack.empty()) {
    Frame &top = stack.back();
    if (top.node == target) {
      std::vector<Position> path;
      path.reserve(stack.size());
      for (const Frame &f : stack) {
        path.push_back(f.node);
      }
      return path;
    }
    const auto &kids = tree.children(top.node);
    if (top.nextChild < kids.size()) {
      const Position child = kids[top.nextChild++].get();
      stack.push_back(Frame{child, 0}); // invalidates `top`; not used after
    } else {
      stack.pop_back();
    }
  }
  return {};
}

} // namespace detail

/**
 * @brief Converts @p tree into a tree with the same node adjacencies, rooted
 *        at @p v.
 *
 * Let root = p_0, p_1, ..., p_k = v be the path from the old root to @p v.
 * Rooting at @p v only reverses the direction of the edges (p_i, p_{i+1});
 * every node off that path keeps its parent. The result therefore has exactly
 * the same set of undirected edges, and the depth of each node in it equals
 * that node's distance from @p v in @p tree.
 *
 * Child order of the result: every node keeps its remaining old children in
 * their original order, and its old parent (if it had one) is appended as its
 * last child. If @p v is the old root, the result is a copy of @p tree.
 *
 * Elements are copied, @p tree is left untouched, and positions of @p tree
 * are not valid in the result (find a node there by searching for its
 * element).
 *
 * @tparam T Element type (copy-constructible).
 * @param tree Tree to re-root.
 * @param v Node of @p tree that becomes the root.
 * @return A new tree with the same adjacencies, rooted at a copy of @p v.
 * @throws std::invalid_argument if @p v is null or does not belong to
 *         @p tree.
 *
 * @par Complexity
 * - Time:  Theta(n): one O(n) search for the path, then every node is created
 *          once and every child link is read once.
 * - Space: O(h) for the path plus O(n) for the pending-copy stack (worst
 *          case, e.g. a star), besides the O(n) output.
 *          Iterative throughout, so arbitrarily deep trees are safe.
 */
template <typename T>
GeneralTree<T> rerootAt(const GeneralTree<T> &tree,
                        typename GeneralTree<T>::Position v) {
  using Position = typename GeneralTree<T>::Position;

  if (v == nullptr) {
    throw std::invalid_argument("rerootAt: null position");
  }

  const std::vector<Position> path = detail::rootPath(tree, v);
  if (path.empty()) {
    throw std::invalid_argument(
        "rerootAt: position does not belong to this tree");
  }

  GeneralTree<T> result;
  // (old node, its copy in `result`) whose subtree still has to be copied.
  std::vector<std::pair<Position, Position>> pending;

  // Walk the path from v up to the old root, chaining the copies.
  Position current = result.insertRoot(v->element());
  for (std::size_t i = path.size(); i-- > 0;) {
    const Position x = path[i];
    const Position skip = (i + 1 < path.size()) ? path[i + 1] : nullptr;

    for (const auto &kid : tree.children(x)) {
      if (kid.get() == skip) {
        continue; // already above x in the new tree
      }
      pending.emplace_back(kid.get(), result.addChild(current, kid->element()));
    }
    if (i > 0) {
      // The old parent becomes the last child, and the walk continues there.
      current = result.addChild(current, path[i - 1]->element());
    }
  }

  // Copy the untouched subtrees hanging off the path.
  while (!pending.empty()) {
    const auto [oldNode, newNode] = pending.back();
    pending.pop_back();
    for (const auto &kid : tree.children(oldNode)) {
      pending.emplace_back(kid.get(), result.addChild(newNode, kid->element()));
    }
  }
  return result;
}

} // namespace adt

#endif // C7_REROOT_HPP