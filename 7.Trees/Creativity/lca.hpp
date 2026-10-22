/**
 * @file lca.hpp
 * @brief Lowest common ancestor (LCA) for adt::GeneralTree.
 */
#ifndef C7_LCA_HPP
#define C7_LCA_HPP

#include <cstddef>
#include <stdexcept>
#include <vector>

#include "general_tree.hpp"

namespace adt {

/**
 * @brief Lowest common ancestor of @p v and @p w.
 *
 * The LCA is the lowest node having both @p v and @p w as descendants, where
 * a node counts as a descendant of itself. GeneralTree nodes have no parent
 * pointers, so the answer is found with a single iterative DFS that keeps the
 * current root-to-node path on an explicit stack.
 *
 * Algorithm: the first time the DFS enters @p v or @p w, remember that node's
 * stack index (the "candidate"). Whenever the candidate's subtree has been
 * fully explored without meeting the other target, the candidate moves to its
 * parent. When the second target is entered, the candidate is the LCA.
 *
 * @tparam T Element type of the tree.
 * @param tree Tree containing both nodes.
 * @param v First node.
 * @param w Second node (may equal @p v).
 * @return Position of the lowest common ancestor.
 * @throws std::invalid_argument if @p v or @p w is null, or if either does
 *         not belong to @p tree.
 *
 * @par Complexity
 * - Time:  O(n) (Theta(n) worst case): every node is pushed and popped at
 *          most once and every child link is read at most once. The search
 *          stops as soon as the second target is found.
 * - Space: O(h) for the explicit stack, h = height of @p tree. Iterative, so
 *          arbitrarily deep trees cannot overflow the call stack.
 */
template <typename T>
typename GeneralTree<T>::Position
lowestCommonAncestor(const GeneralTree<T> &tree,
                     typename GeneralTree<T>::Position v,
                     typename GeneralTree<T>::Position w) {
  using Position = typename GeneralTree<T>::Position;

  if (v == nullptr || w == nullptr) {
    throw std::invalid_argument("lowestCommonAncestor: null position");
  }

  /// One level of the current root-to-node path.
  struct Frame {
    Position node;
    std::size_t nextChild; ///< index of the next child to explore
  };

  constexpr std::size_t kNone = static_cast<std::size_t>(-1);

  std::vector<Frame> path;
  std::size_t candidate = kNone;   // deepest on-path ancestor-or-self of the
                                   // first target found; kNone = none found yet
  Position entering = tree.root(); // node about to be pushed (null = none)

  while (entering != nullptr || !path.empty()) {
    if (entering != nullptr) {
      path.push_back(Frame{entering, 0});
      entering = nullptr;

      const int hits = (path.back().node == v) + (path.back().node == w);
      if (hits == 2) {
        return v; // v == w, and it is this node
      }
      if (hits == 1) {
        if (candidate == kNone) {
          candidate = path.size() - 1; // first target found
        } else {
          return path[candidate].node; // second target found
        }
      }
      continue;
    }

    Frame &top = path.back();
    const auto &kids = tree.children(top.node);
    if (top.nextChild < kids.size()) {
      entering = kids[top.nextChild++].get();
    } else {
      // Candidate's subtree is exhausted: its parent becomes the candidate.
      // Popping the root wraps candidate to kNone, but then the loop ends.
      if (candidate == path.size() - 1) {
        --candidate;
      }
      path.pop_back();
    }
  }

  throw std::invalid_argument(
      "lowestCommonAncestor: position does not belong to this tree");
}

} // namespace adt

#endif // C7_LCA_HPP