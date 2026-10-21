/**
 * @file binary_tree.tpp
 * @brief Implementation of adt::BinaryTree (included by binary_tree.hpp).
 */
#ifndef ADT_BINARY_TREE_TPP
#define ADT_BINARY_TREE_TPP

#include <stdexcept>
#include <utility>

namespace adt {

template <typename T>
BinaryTree<T>::BinaryTree(BinaryTree &&other) noexcept
    : root_(std::move(other.root_)), size_(std::exchange(other.size_, 0)) {}

template <typename T>
BinaryTree<T> &BinaryTree<T>::operator=(BinaryTree &&other) noexcept {
  if (this != &other) {
    clear(); // avoid the recursive destruction of the old tree
    root_ = std::move(other.root_);
    size_ = std::exchange(other.size_, 0);
  }
  return *this;
}

template <typename T>
typename BinaryTree<T>::Position BinaryTree<T>::left(Position p) const {
  if (!p)
    throw std::invalid_argument("left: null position");
  return p->left_.get();
}

template <typename T>
typename BinaryTree<T>::Position BinaryTree<T>::right(Position p) const {
  if (!p)
    throw std::invalid_argument("right: null position");
  return p->right_.get();
}

template <typename T> bool BinaryTree<T>::isExternal(Position p) const {
  if (!p)
    throw std::invalid_argument("isExternal: null position");
  return !p->left_ && !p->right_;
}

template <typename T>
typename BinaryTree<T>::Position BinaryTree<T>::insertRoot(T e) {
  if (root_)
    throw std::logic_error("insertRoot: tree not empty");
  root_.reset(new Node(std::move(e)));
  size_ = 1;
  return root_.get();
}

template <typename T>
typename BinaryTree<T>::Position BinaryTree<T>::insertLeft(Position p, T e) {
  if (!p)
    throw std::invalid_argument("insertLeft: null position");
  if (p->left_)
    throw std::logic_error("insertLeft: left child exists");
  Node *n = const_cast<Node *>(p); // safe: this tree owns every node
  n->left_.reset(new Node(std::move(e)));
  ++size_;
  return n->left_.get();
}

template <typename T>
typename BinaryTree<T>::Position BinaryTree<T>::insertRight(Position p, T e) {
  if (!p)
    throw std::invalid_argument("insertRight: null position");
  if (p->right_)
    throw std::logic_error("insertRight: right child exists");
  Node *n = const_cast<Node *>(p);
  n->right_.reset(new Node(std::move(e)));
  ++size_;
  return n->right_.get();
}

template <typename T> void BinaryTree<T>::clear() noexcept {
  std::stack<std::unique_ptr<Node>> pending;
  if (root_)
    pending.push(std::move(root_));
  while (!pending.empty()) {
    std::unique_ptr<Node> n = std::move(pending.top());
    pending.pop();
    if (n->left_)
      pending.push(std::move(n->left_));
    if (n->right_)
      pending.push(std::move(n->right_));
    // n is destroyed here with null children, so there is no recursion
  }
  size_ = 0;
}

} // namespace adt

#endif // ADT_BINARY_TREE_TPP