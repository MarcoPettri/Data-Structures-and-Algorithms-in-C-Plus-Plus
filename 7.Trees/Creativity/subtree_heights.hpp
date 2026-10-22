/**
 * @file subtree_heights.hpp
 * @brief Algorithm: print string and subtree height for every internal node
 *        of a general tree.
 *
 * For an internal node @c v the *height of its subtree* is defined as the
 * length of the longest path from @c v down to a leaf:
 *   - A leaf has height 0.
 *   - An internal node has height = 1 + max(height of children).
 *
 * The algorithm walks the tree in a **single post-order pass** so that every
 * node's height is known before its parent inspects it.
 * All traversal is done iteratively (explicit stack) — safe for deep trees.
 */
#ifndef C74_SUBTREE_HEIGHTS_HPP
#define C74_SUBTREE_HEIGHTS_HPP

#include <ostream>
#include <stack>
#include <string>
#include <unordered_map>

#include "general_tree.hpp"

namespace algo {

/**
 * @brief Prints, for every internal node, its stored string and the height of
 *        its subtree.
 *
 * @par Algorithm (iterative post-order)
 * Post-order traversal is simulated with two stacks:
 *  1. **Discovery stack** — nodes are pushed in pre-order (root first, then
 *     children right-to-left so the left-most child is processed first).
 *  2. **Height map** — keyed by Position; each node's height is computed once
 *     all its children's heights are available (i.e. on pop from the result
 *     stack).
 *
 * In practice a single stack suffices if we push a node twice: once to
 * descend and once to process (two-pass trick).  We use a cleaner "parent
 * table" approach: push every node with its parent pointer; after all
 * children are processed, update the parent's running maximum height.
 *
 * The implementation chosen here uses a **reverse-post-order** pass
 * (two-stack method): the first stack drives exploration, the second collects
 * nodes in reverse post-order; then we process the second stack front-to-back
 * (= post-order) to compute heights bottom-up.
 *
 * @par Complexity
 * - Time  : O(n)  — each node is pushed/popped twice.
 * - Space : O(n)  — height map + two stacks, at most 2n entries total.
 *
 * @tparam T  Element type; must be streamable via @c operator<<.
 * @param tree Tree to traverse.
 * @param out  Output stream that receives the report lines.
 *
 * Output format (one line per internal node, pre-order of discovery):
 * @code
 *   node "<string>" — subtree height: <h>
 * @endcode
 */
template <typename T>
void printInternalHeights(const adt::GeneralTree<T> &tree, std::ostream &out) {
  using Position = typename adt::GeneralTree<T>::Position;

  if (tree.isEmpty()) {
    return;
  }

  // ── Phase 1: collect nodes in post-order using the two-stack trick ────────
  // Stack 1 drives exploration; stack 2 accumulates reverse-post-order.
  std::stack<Position> explore;
  std::stack<Position> postorder; // will be processed LIFO → post-order

  explore.push(tree.root());

  while (!explore.empty()) {
    Position p = explore.top();
    explore.pop();
    postorder.push(p);

    // Push children left-to-right so rightmost is processed last (LIFO).
    for (const auto &child : tree.children(p)) {
      explore.push(child.get());
    }
  }

  // ── Phase 2: compute heights bottom-up, print internal nodes ─────────────
  std::unordered_map<Position, int> heightOf;
  heightOf.reserve(tree.size());

  while (!postorder.empty()) {
    Position p = postorder.top();
    postorder.pop();

    const auto &kids = tree.children(p);

    if (kids.empty()) {
      // Leaf node: height = 0, nothing to print.
      heightOf[p] = 0;
    } else {
      // Internal node: height = 1 + max child height.
      int maxChild = 0;
      for (const auto &child : kids) {
        maxChild = std::max(maxChild, heightOf.at(child.get()));
      }
      const int h = 1 + maxChild;
      heightOf[p] = h;

      out << "node \"" << p->element() << "\" — subtree height: " << h << '\n';
    }
  }
}

} // namespace algo

#endif // C74_SUBTREE_HEIGHTS_HPP
