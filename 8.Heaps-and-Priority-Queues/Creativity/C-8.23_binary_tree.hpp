/**
 * @file C-8.23_binary_tree.hpp
 * @brief Pointer-based complete binary tree ADT and next insertion point
 * algorithm.
 */

#pragma once

#include <cstddef>

/**
 * @struct BinaryTreeNode
 * @brief Represents a node in a binary tree with parent and child pointers.
 *
 * @tparam T The element type stored in the node.
 */
template <typename T> struct BinaryTreeNode {
  T data;
  BinaryTreeNode *parent = nullptr;
  BinaryTreeNode *left = nullptr;
  BinaryTreeNode *right = nullptr;

  [[nodiscard]] bool isRoot() const { return parent == nullptr; }

  [[nodiscard]] bool isLeftChild() const {
    return parent && parent->left == this;
  }

  [[nodiscard]] bool isRightChild() const {
    return parent && parent->right == this;
  }
};

/**
 * @brief Finds the next insertion point (new last node) in a complete binary
 * tree given a pointer to the current last node v, using only Binary Tree ADT
 * operations.
 *
 * @tparam T The element type stored in the tree node.
 * @param v Pointer to the current last node.
 * @return BinaryTreeNode<T>* Pointer to the node where the new element should
 * be inserted.
 *
 * @section complexity Complexity Analysis
 * - **Time Complexity:** $O(\log n)$, where $n$ is the number of nodes in the
 * tree. In the worst case, the algorithm climbs up the right spine to an
 * ancestor or root
 *   ($O(\log n)$ steps) and then down the left spine to the bottom level
 * ($O(\log n)$ steps).
 * - **Space Complexity:** $O(1)$ auxiliary space, using only a fixed number of
 * pointer variables.
 */
template <typename T>
BinaryTreeNode<T> *findNextInsertionPoint(BinaryTreeNode<T> *v) {
  if (!v) {
    return nullptr;
  }

  // Case 1: v is the root (tree has only 1 node)
  if (v->isRoot()) {
    return v->left; // nullptr, indicating the left child of the root
  }

  // Case 2: v is a left child, its sibling (right child of parent) is the next
  // insertion point
  if (v->isLeftChild()) {
    return v->parent->right;
  }

  // Case 3: v is a right child. Climb up the right spine.
  BinaryTreeNode<T> *curr = v;
  while (curr->isRightChild()) {
    curr = curr->parent;
  }

  // If we hit the root, the tree of height h is full.
  // Otherwise, curr is a left child, so we move to its sibling (right child of
  // its parent).
  if (!curr->isRoot()) {
    curr = curr->parent->right;
  }

  // Traverse down the left spine from the target subtree root to find the
  // leftmost leaf position
  while (curr && curr->left) {
    curr = curr->left;
  }

  return curr ? curr->left : nullptr;
}