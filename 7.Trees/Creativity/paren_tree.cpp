/**
 * @file paren_tree.cpp
 * @brief Implementation of the parenthesis-string-to-tree algorithm.
 */
#include "paren_tree.hpp"

#include <algorithm>
#include <stack>
#include <stdexcept>

namespace c72 {

// ---------------------------------------------------------------------------
// parseParenTree
// ---------------------------------------------------------------------------

std::unique_ptr<Node> parseParenTree(std::string_view s) {
  if (s.empty()) {
    return nullptr;
  }

  // Stack stores owning pointers to nodes that are currently "open"
  // (their matching ')' has not yet been encountered).
  std::stack<std::unique_ptr<Node>> stk;

  int nextLabel = 1; // DFS discovery counter (1-based)

  for (const char ch : s) {
    if (ch == '(') {
      // Open a new node; it becomes a child once its ')' is seen.
      stk.push(std::make_unique<Node>(nextLabel++));
    } else if (ch == ')') {
      if (stk.empty()) {
        throw std::invalid_argument("parseParenTree: unmatched ')' in input");
      }
      // The top node is now complete.
      std::unique_ptr<Node> completed = std::move(stk.top());
      stk.pop();

      if (stk.empty()) {
        // This is the root — but there may still be unread input,
        // meaning the string has more than one top-level tree.
        // We only support a single top-level pair.
        return completed;
      }
      // Append completed node as the next (right-most) child.
      stk.top()->children.push_back(std::move(completed));
    }
    // Any other character (e.g. whitespace) is silently ignored.
  }

  if (!stk.empty()) {
    throw std::invalid_argument("parseParenTree: unmatched '(' in input");
  }

  // Should be unreachable for a valid non-empty string.
  return nullptr;
}

// ---------------------------------------------------------------------------
// countNodes  (iterative post-order count)
// ---------------------------------------------------------------------------

std::size_t countNodes(const Node *n) noexcept {
  if (!n) {
    return 0;
  }
  std::size_t total = 0;
  std::stack<const Node *> stk;
  stk.push(n);
  while (!stk.empty()) {
    const Node *cur = stk.top();
    stk.pop();
    ++total;
    for (const auto &child : cur->children) {
      stk.push(child.get());
    }
  }
  return total;
}

// ---------------------------------------------------------------------------
// height  (iterative BFS / level tracking)
// ---------------------------------------------------------------------------

int height(const Node *n) noexcept {
  if (!n) {
    return -1;
  }
  int h = 0;
  // Stack stores (node, depth)
  std::stack<std::pair<const Node *, int>> stk;
  stk.push({n, 0});
  while (!stk.empty()) {
    auto [cur, depth] = stk.top();
    stk.pop();
    h = std::max(h, depth);
    for (const auto &child : cur->children) {
      stk.push({child.get(), depth + 1});
    }
  }
  return h;
}

} // namespace c72
