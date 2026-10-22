/**
 * @file paren_tree.hpp
 * @brief Convert a fully balanced parenthesis string into a tree (C-7.2).
 *
 * A *fully balanced parenthesis string* is defined recursively:
 *  - The empty string represents nothing.
 *  - A string of the form @c (S₁S₂…Sₖ), where each @c Sᵢ is itself a fully
 *    balanced parenthesis string, represents a node whose children are the
 *    trees of @c S₁, S₂, …, Sₖ (left-to-right).
 *
 */
#ifndef C72_PAREN_TREE_HPP
#define C72_PAREN_TREE_HPP

#include <cstddef>
#include <memory>
#include <string_view>
#include <vector>

namespace c72 {

// ---------------------------------------------------------------------------
// Data types
// ---------------------------------------------------------------------------

/**
 * @brief A node in the parenthesis tree.
 *
 * Each node carries an automatically assigned integer label (its DFS
 * discovery order, 1-based) so that tree shapes can be compared in tests.
 * Children are ordered left-to-right to preserve the order from the input.
 */
struct Node {
  int label{};                                 ///< 1-based DFS discovery order.
  std::vector<std::unique_ptr<Node>> children; ///< Ordered child list.

  /** @brief Convenience constructor. */
  explicit Node(int lbl) : label{lbl} {}
};

// ---------------------------------------------------------------------------
// Algorithm
// ---------------------------------------------------------------------------

/**
 * @brief Convert a fully balanced parenthesis string into a tree.
 *
 * @par Algorithm
 * Use an explicit stack whose entries are the *currently open* nodes (i.e.
 * nodes for which the matching @c ')' has not yet been seen).
 * -# Scan the input left-to-right.
 * -# On @c '(' : create a new node, push it onto the stack.  If the stack
 *    was non-empty before the push, the new node is a child of the previous
 *    top.
 * -# On @c ')' : pop the top node.  The popped node is now complete.  If
 *    the stack is still non-empty, append the completed node as the
 *    rightmost child of the new top.  Otherwise the popped node is the root.
 *
 * @par Complexity
 * - Time  : O(n), where n = @p s.size() — each character is visited once.
 * - Space : O(h) auxiliary stack depth, where h is the nesting depth of @p s.
 *
 * @param s  A fully balanced parenthesis string.  Every @c '(' must have a
 *           matching @c ')'.
 * @return   Owning pointer to the root of the resulting tree, or @c nullptr
 *           for an empty input string.
 *
 * @throws std::invalid_argument if @p s is not a fully balanced string
 *         (unmatched parentheses detected).
 */
[[nodiscard]] std::unique_ptr<Node> parseParenTree(std::string_view s);

// ---------------------------------------------------------------------------
// Utilities (useful for testing / display)
// ---------------------------------------------------------------------------

/**
 * @brief Returns the total number of nodes in the (sub-)tree rooted at @p n.
 * @param n  Root of the subtree (may be @c nullptr → returns 0).
 */
[[nodiscard]] std::size_t countNodes(const Node *n) noexcept;

/**
 * @brief Returns the height of the (sub-)tree rooted at @p n.
 *        A single node has height 0; @c nullptr returns -1.
 * @param n  Root of the subtree.
 */
[[nodiscard]] int height(const Node *n) noexcept;

} // namespace c72

#endif // C72_PAREN_TREE_HPP
