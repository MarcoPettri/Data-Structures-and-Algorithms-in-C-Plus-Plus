/**
 * @file parentless_binary_tree.tpp
 * @brief Template implementation of adt::ParentlessBinaryTree.
 *
 * Included at the end of parentless_binary_tree.hpp; do not include directly.
 */
#ifndef PTREE_PARENTLESS_BINARY_TREE_TPP
#define PTREE_PARENTLESS_BINARY_TREE_TPP

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <utility>

namespace adt {

// ------------------------------------------------------ special members ---

template <typename E> ParentlessBinaryTree<E>::~ParentlessBinaryTree() {
  destroy(root_);
}

template <typename E>
ParentlessBinaryTree<E>::ParentlessBinaryTree(
    ParentlessBinaryTree &&other) noexcept
    : root_(std::exchange(other.root_, nullptr)),
      size_(std::exchange(other.size_, 0)) {}

template <typename E>
ParentlessBinaryTree<E> &
ParentlessBinaryTree<E>::operator=(ParentlessBinaryTree &&other) noexcept {
  if (this != &other) {
    clear();
    root_ = std::exchange(other.root_, nullptr);
    size_ = std::exchange(other.size_, 0);
  }
  return *this;
}

// -------------------------------------------------- upward queries --------

template <typename E>
auto ParentlessBinaryTree<E>::parent(const Position &p) const -> Position {
  return Position(locate(require_node(p)).parent);
}

template <typename E>
auto ParentlessBinaryTree<E>::sibling(const Position &p) const -> Position {
  const Node *v = require_node(p);
  const Locator loc = locate(v);
  if (loc.parent == nullptr) {
    throw std::logic_error(
        "ParentlessBinaryTree::sibling: the root has no sibling");
  }
  return Position(loc.parent->left == v ? loc.parent->right : loc.parent->left);
}

template <typename E>
std::size_t ParentlessBinaryTree<E>::depth(const Position &p) const {
  return locate(require_node(p)).depth;
}

template <typename E> std::size_t ParentlessBinaryTree<E>::height() const {
  struct Frame {
    const Node *node;
    std::size_t depth;
  };
  std::size_t h = 0;
  if (root_ == nullptr)
    return h;

  std::vector<Frame> stack;
  stack.push_back({root_, 0});
  while (!stack.empty()) {
    const Frame f = stack.back();
    stack.pop_back();
    h = std::max(h, f.depth);
    if (f.node->left)
      stack.push_back({f.node->left, f.depth + 1});
    if (f.node->right)
      stack.push_back({f.node->right, f.depth + 1});
  }
  return h;
}

// ------------------------------------------------------- traversals -------

template <typename E>
auto ParentlessBinaryTree<E>::positions() const -> std::vector<Position> {
  std::vector<Position> out;
  out.reserve(size_);
  if (root_ == nullptr)
    return out;

  std::vector<Node *> stack;
  stack.push_back(root_);
  while (!stack.empty()) {
    Node *v = stack.back();
    stack.pop_back();
    out.push_back(Position(v));
    if (v->right)
      stack.push_back(v->right); // pushed first, visited last
    if (v->left)
      stack.push_back(v->left);
  }
  return out;
}

template <typename E>
auto ParentlessBinaryTree<E>::inorder() const -> std::vector<Position> {
  std::vector<Position> out;
  out.reserve(size_);

  std::vector<Node *> stack;
  Node *cur = root_;
  while (cur != nullptr || !stack.empty()) {
    while (cur != nullptr) { // descend along the left spine
      stack.push_back(cur);
      cur = cur->left;
    }
    cur = stack.back();
    stack.pop_back();
    out.push_back(Position(cur));
    cur = cur->right;
  }
  return out;
}

// ---------------------------------------------------------- updates -------

template <typename E>
auto ParentlessBinaryTree<E>::addRoot(const E &e) -> Position {
  if (root_ != nullptr) {
    throw std::logic_error(
        "ParentlessBinaryTree::addRoot: tree already has a root");
  }
  root_ = new Node(e);
  size_ = 1;
  return Position(root_);
}

template <typename E>
void ParentlessBinaryTree<E>::expandExternal(const Position &p,
                                             const E &left_elt,
                                             const E &right_elt) {
  Node *v = require_node(p);
  if (v->left != nullptr || v->right != nullptr) {
    throw std::logic_error(
        "ParentlessBinaryTree::expandExternal: node is not external");
  }
  auto l = std::make_unique<Node>(
      left_elt); // exception-safe: no leak if the 2nd throws
  auto r = std::make_unique<Node>(right_elt);
  v->left = l.release();
  v->right = r.release();
  size_ += 2;
}

template <typename E>
auto ParentlessBinaryTree<E>::removeAboveExternal(const Position &p)
    -> Position {
  Node *v = require_node(p);
  if (v->left != nullptr || v->right != nullptr) {
    throw std::logic_error(
        "ParentlessBinaryTree::removeAboveExternal: node is not external");
  }

  const Locator loc = locate(v); // Theta(n): finds parent AND grandparent
  Node *w = loc.parent;
  if (w == nullptr) {
    throw std::logic_error(
        "ParentlessBinaryTree::removeAboveExternal: the root has no parent");
  }

  Node *sib = (w->left == v) ? w->right : w->left;
  if (w == root_) {
    root_ = sib; // sibling becomes the new root
  } else {
    Node *g = loc.grandparent;
    if (g->left == w)
      g->left = sib;
    else
      g->right = sib;
  }
  delete v;
  delete w;
  size_ -= 2;
  return Position(sib);
}

template <typename E> void ParentlessBinaryTree<E>::clear() noexcept {
  destroy(root_);
  root_ = nullptr;
  size_ = 0;
}

// ---------------------------------------------------------- helpers -------

template <typename E>
auto ParentlessBinaryTree<E>::locate(const Node *target) const -> Locator {
  struct Frame {
    Node *node;
    Node *parent;
    Node *grandparent;
    std::size_t depth;
  };

  if (target != nullptr && root_ != nullptr) {
    std::vector<Frame> stack; // never holds more than height+1 frames
    stack.push_back({root_, nullptr, nullptr, 0});
    while (!stack.empty()) {
      const Frame f = stack.back();
      stack.pop_back();
      if (f.node == target) {
        return {f.parent, f.grandparent, f.depth};
      }
      if (f.node->right)
        stack.push_back({f.node->right, f.node, f.parent, f.depth + 1});
      if (f.node->left)
        stack.push_back({f.node->left, f.node, f.parent, f.depth + 1});
    }
  }
  throw std::invalid_argument(
      "ParentlessBinaryTree: position is null or does not belong to this tree");
}

template <typename E>
auto ParentlessBinaryTree<E>::require_node(const Position &p) -> Node * {
  if (p.isNull()) {
    throw std::invalid_argument("ParentlessBinaryTree: null position");
  }
  return p.node_;
}

template <typename E>
void ParentlessBinaryTree<E>::destroy(Node *root) noexcept {
  // Rotate the left child above the current node until there is no left
  // child, then delete the node and continue with its right subtree.
  // Each node is rotated up at most once, so the total time is O(n),
  // with O(1) extra space and no recursion.
  while (root != nullptr) {
    if (Node *l = root->left) {
      root->left = l->right;
      l->right = root;
      root = l;
    } else {
      Node *next = root->right;
      delete root;
      root = next;
    }
  }
}

} // namespace adt

#endif // PTREE_PARENTLESS_BINARY_TREE_TPP