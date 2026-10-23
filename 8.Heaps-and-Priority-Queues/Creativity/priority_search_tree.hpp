/**
 * @file priority_search_tree.hpp
 * @brief Linear-time algorithm to convert a complete binary tree into a
 * priority search tree.
 */

#pragma once

#include <algorithm>
#include <optional>
#include <vector>

/**
 * @brief Represents a 2D point with distinct integer x and y coordinates.
 */
struct Point {
  int x;
  int y;

  bool operator==(const Point &other) const {
    return x == other.x && y == other.y;
  }
};

/**
 * @struct PSTNode
 * @brief Represents a node in the binary tree T.
 */
struct PSTNode {
  Point top;            // The assigned top(v) point for this node
  bool has_top = false; // Flag indicating if top is valid

  Point m1; // M1(v): Point in S(v) with maximum y-coordinate
  bool has_m1 = false;

  Point m2; // M2(v): Point in S(v) with second-maximum y-coordinate
  bool has_m2 = false;

  PSTNode *left = nullptr;  // Left child
  PSTNode *right = nullptr; // Right child
};

/**
 * @brief Helper function to combine M1 and M2 from children into parent M1 and
 * M2.
 */
inline void computeSubtreeExtremes(PSTNode *node) {
  if (!node)
    return;

  std::vector<Point> candidates;
  if (node->left) {
    if (node->left->has_m1)
      candidates.push_back(node->left->m1);
    if (node->left->has_m2)
      candidates.push_back(node->left->m2);
  }
  if (node->right) {
    if (node->right->has_m1)
      candidates.push_back(node->right->m1);
    if (node->right->has_m2)
      candidates.push_back(node->right->m2);
  }

  if (candidates.empty()) {
    return;
  }

  // Sort candidates descending by y-coordinate
  std::sort(candidates.begin(), candidates.end(),
            [](const Point &a, const Point &b) { return a.y > b.y; });

  if (!candidates.empty()) {
    node->m1 = candidates[0];
    node->has_m1 = true;
  }
  if (candidates.size() > 1) {
    node->m2 = candidates[1];
    node->has_m2 = true;
  }
}

/**
 * @brief Phase 1: Bottom-up pass to compute M1(v) and M2(v) for all nodes.
 */
inline void bottomUpPass(PSTNode *node) {
  if (!node)
    return;

  bottomUpPass(node->left);
  bottomUpPass(node->right);

  computeSubtreeExtremes(node);
}

/**
 * @brief Phase 2: Top-down pass to assign top(v) labels.
 *
 * @param node Current node
 * @param parent_top The top(u) of the parent node u (if any)
 */
inline void topDownPass(PSTNode *node, const std::optional<Point> &parent_top) {
  if (!node)
    return;

  if (!parent_top.has_value()) {
    // Root node: top(r) = M1(r)
    if (node->has_m1) {
      node->top = node->m1;
      node->has_top = true;
    }
  } else {
    // Non-root node v with parent u:
    // top(v) = M2(v) if top(u) == M1(v), otherwise M1(v)
    if (node->has_m1 && node->m1 == parent_top.value()) {
      if (node->has_m2) {
        node->top = node->m2;
        node->has_top = true;
      }
    } else {
      if (node->has_m1) {
        node->top = node->m1;
        node->has_top = true;
      }
    }
  }

  std::optional<Point> current_top =
      node->has_top ? std::optional<Point>(node->top) : std::nullopt;
  topDownPass(node->left, current_top);
  topDownPass(node->right, current_top);
}

/**
 * @brief Turns a complete binary tree T into a priority search tree in O(n)
 * time.
 *
 * @param root Pointer to the root of the complete binary tree.
 *
 * @section complexity Complexity Analysis
 * - **Time Complexity:** $O(n)$, where $n$ is the number of nodes in tree $T$.
 *   The bottom-up pass visits every node once to compute extreme values,
 *   and the top-down pass visits every node once to assign `top` labels.
 * - **Space Complexity:** $O(h)$ auxiliary stack space for recursion,
 *   where $h = \log n$ for a complete binary tree.
 */
inline void convertToPrioritySearchTree(PSTNode *root) {
  if (!root)
    return;
  bottomUpPass(root);
  topDownPass(root, std::nullopt);
}