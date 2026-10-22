/**
 * @file linked_binary_tree.tpp
 * @brief Template implementation of adt::LinkedBinaryTree.
 *
 * Included at the end of linked_binary_tree.hpp; do not include directly.
 */
#ifndef C7_LINKED_BINARY_TREE_TPP
#define C7_LINKED_BINARY_TREE_TPP

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

namespace adt {

// ---------------------------------------------------------------- lifecycle

template <typename E>
LinkedBinaryTree<E>::LinkedBinaryTree(const LinkedBinaryTree &other)
    : LinkedBinaryTree() {
  // The delegating constructor has completed, so if copyFrom() throws the
  // destructor runs and frees whatever was built so far.
  copyFrom(other);
}

template <typename E>
LinkedBinaryTree<E>::LinkedBinaryTree(LinkedBinaryTree &&other) noexcept
    : root_(std::exchange(other.root_, nullptr)),
      size_(std::exchange(other.size_, 0)), token_(std::move(other.token_)) {}

template <typename E>
LinkedBinaryTree<E> &
LinkedBinaryTree<E>::operator=(const LinkedBinaryTree &other) {
  if (this != &other) {
    LinkedBinaryTree copy(other); // may throw: *this is still untouched
    swap(copy);
  }
  return *this;
}

template <typename E>
LinkedBinaryTree<E> &
LinkedBinaryTree<E>::operator=(LinkedBinaryTree &&other) noexcept {
  if (this != &other) {
    clear();
    root_ = std::exchange(other.root_, nullptr);
    size_ = std::exchange(other.size_, 0);
    token_ = std::move(other.token_);
  }
  return *this;
}

template <typename E> LinkedBinaryTree<E>::~LinkedBinaryTree() {
  destroyAll(root_);
}

template <typename E>
void LinkedBinaryTree<E>::swap(LinkedBinaryTree &other) noexcept {
  std::swap(root_, other.root_);
  std::swap(size_, other.size_);
  token_.swap(other.token_);
}

// ------------------------------------------------------------------ queries

template <typename E>
bool LinkedBinaryTree<E>::owns(const ConstPosition &p) const noexcept {
  return p.node_ != nullptr && p.node_->owner == token_.get();
}

template <typename E>
std::size_t LinkedBinaryTree<E>::depth(const ConstPosition &p) const {
  const Node *v = requireNode(p.node_, "depth");
  std::size_t d = 0;
  while (v->parent != nullptr) {
    v = v->parent;
    ++d;
  }
  return d;
}

template <typename E> std::size_t LinkedBinaryTree<E>::height() const {
  if (root_ == nullptr) {
    throw InvalidOperation("LinkedBinaryTree::height: the tree is empty");
  }
  return height(root());
}

template <typename E>
std::size_t LinkedBinaryTree<E>::height(const ConstPosition &p) const {
  const Node *top = requireNode(p.node_, "height");

  struct Frame {
    const Node *node;
    std::size_t level; ///< distance from `top`
  };

  std::size_t best = 0;
  std::vector<Frame> stack;
  stack.push_back(Frame{top, 0});
  while (!stack.empty()) {
    const Frame f = stack.back();
    stack.pop_back();
    best = std::max(best, f.level);
    if (f.node->left != nullptr) {
      stack.push_back(Frame{f.node->left, f.level + 1});
    }
    if (f.node->right != nullptr) {
      stack.push_back(Frame{f.node->right, f.level + 1});
    }
  }
  return best;
}

template <typename E>
auto LinkedBinaryTree<E>::positions(Traversal order) -> PositionList {
  return wrap<Position>(collect(order));
}

template <typename E>
auto LinkedBinaryTree<E>::positions(Traversal order) const
    -> ConstPositionList {
  return wrap<ConstPosition>(collect(order));
}

// ------------------------------------------------------------------ updates

template <typename E> auto LinkedBinaryTree<E>::addRoot(E e) -> Position {
  if (root_ != nullptr) {
    throw InvalidOperation(
        "LinkedBinaryTree::addRoot: tree already has a root");
  }
  ensureToken();
  root_ = makeNode(nullptr, std::move(e)); // throws before any state changes
  size_ = 1;
  return Position(root_);
}

template <typename E>
void LinkedBinaryTree<E>::expandExternal(const Position &p, E leftElt,
                                         E rightElt) {
  Node *v = requireNode(p.node_, "expandExternal");
  if (v->left != nullptr || v->right != nullptr) {
    throw InvalidOperation(
        "LinkedBinaryTree::expandExternal: node is already internal");
  }
  // Build both children first: if the second allocation or construction
  // throws, the first is released and the tree is unchanged.
  std::unique_ptr<Node> l(makeNode(v, std::move(leftElt)));
  std::unique_ptr<Node> r(makeNode(v, std::move(rightElt)));
  v->left = l.release();
  v->right = r.release();
  size_ += 2;
}

template <typename E>
auto LinkedBinaryTree<E>::removeAboveExternal(const Position &p) -> Position {
  Node *v = requireNode(p.node_, "removeAboveExternal");
  if (v->left != nullptr || v->right != nullptr) {
    throw InvalidOperation(
        "LinkedBinaryTree::removeAboveExternal: node is not external");
  }
  Node *w = v->parent;
  if (w == nullptr) {
    throw BoundaryViolation(
        "LinkedBinaryTree::removeAboveExternal: the root has no parent");
  }

  // Everything below is non-throwing.
  Node *sibling = (w->left == v) ? w->right : w->left;
  Node *grand = w->parent;
  sibling->parent = grand;
  if (grand == nullptr) {
    root_ = sibling; // the sibling becomes the new root
  } else {
    (grand->left == w ? grand->left : grand->right) = sibling;
  }
  delete v;
  delete w;
  size_ -= 2;
  return Position(sibling);
}

template <typename E> void LinkedBinaryTree<E>::clear() noexcept {
  destroyAll(root_);
  root_ = nullptr;
  size_ = 0;
}

// ------------------------------------------------------------------ helpers

template <typename E>
auto LinkedBinaryTree<E>::requireNode(Node *v, const char *caller) const
    -> Node * {
  if (v == nullptr) {
    throw InvalidPosition(std::string("LinkedBinaryTree::") + caller +
                          ": null position");
  }
  if (v->owner != token_.get()) {
    throw InvalidPosition(std::string("LinkedBinaryTree::") + caller +
                          ": position does not belong to this tree");
  }
  return v;
}

template <typename E> void LinkedBinaryTree<E>::ensureToken() {
  if (!token_) {
    token_ = std::make_unique<char>();
  }
}

template <typename E>
void LinkedBinaryTree<E>::copyFrom(const LinkedBinaryTree &other) {
  if (other.root_ == nullptr) {
    return;
  }
  ensureToken();
  root_ = makeNode(nullptr, other.root_->elt);
  size_ = 1;

  // (node in `other`, its copy in *this) whose children still have to be
  // copied.
  std::vector<std::pair<const Node *, Node *>> pending;
  pending.emplace_back(other.root_, root_);
  while (!pending.empty()) {
    const auto [src, dst] = pending.back();
    pending.pop_back();
    if (src->left != nullptr) {
      dst->left = makeNode(dst, src->left->elt);
      ++size_;
      pending.emplace_back(src->left, dst->left);
    }
    if (src->right != nullptr) {
      dst->right = makeNode(dst, src->right->elt);
      ++size_;
      pending.emplace_back(src->right, dst->right);
    }
  }
}

template <typename E>
void LinkedBinaryTree<E>::destroyAll(Node *root) noexcept {
  // Walk down to a leaf, unlink and delete it, then continue from its parent.
  // Every node is deleted once: O(n) time, O(1) space, no recursion.
  Node *cur = root;
  while (cur != nullptr) {
    if (cur->left != nullptr) {
      cur = cur->left;
    } else if (cur->right != nullptr) {
      cur = cur->right;
    } else {
      Node *up = cur->parent;
      if (up != nullptr) {
        (up->left == cur ? up->left : up->right) = nullptr;
      }
      delete cur;
      cur = up;
    }
  }
}

// ---------------------------------------------------------------- traversals

template <typename E>
auto LinkedBinaryTree<E>::collect(Traversal order) const
    -> std::vector<Node *> {
  switch (order) {
  case Traversal::Preorder:
    return preorderNodes();
  case Traversal::Inorder:
    return inorderNodes();
  case Traversal::Postorder:
    return postorderNodes();
  case Traversal::LevelOrder:
    return levelOrderNodes();
  }
  throw std::invalid_argument(
      "LinkedBinaryTree::positions: unknown traversal order");
}

template <typename E>
auto LinkedBinaryTree<E>::preorderNodes() const -> std::vector<Node *> {
  std::vector<Node *> out;
  out.reserve(size_);
  if (root_ == nullptr) {
    return out;
  }
  std::vector<Node *> stack;
  stack.push_back(root_);
  while (!stack.empty()) {
    Node *v = stack.back();
    stack.pop_back();
    out.push_back(v);
    if (v->right != nullptr) {
      stack.push_back(v->right); // pushed first, visited last
    }
    if (v->left != nullptr) {
      stack.push_back(v->left);
    }
  }
  return out;
}

template <typename E>
auto LinkedBinaryTree<E>::inorderNodes() const -> std::vector<Node *> {
  std::vector<Node *> out;
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
    out.push_back(cur);
    cur = cur->right;
  }
  return out;
}

template <typename E>
auto LinkedBinaryTree<E>::postorderNodes() const -> std::vector<Node *> {
  std::vector<Node *> out;
  out.reserve(size_);
  std::vector<Node *> stack;
  Node *cur = root_;
  Node *last = nullptr; // most recently visited node
  while (cur != nullptr || !stack.empty()) {
    if (cur != nullptr) {
      stack.push_back(cur);
      cur = cur->left;
    } else {
      Node *top = stack.back();
      if (top->right != nullptr && top->right != last) {
        cur = top->right; // right subtree not done yet
      } else {
        out.push_back(top);
        last = top;
        stack.pop_back();
      }
    }
  }
  return out;
}

template <typename E>
auto LinkedBinaryTree<E>::levelOrderNodes() const -> std::vector<Node *> {
  std::vector<Node *> out; // doubles as the queue
  out.reserve(size_);
  if (root_ == nullptr) {
    return out;
  }
  out.push_back(root_);
  for (std::size_t i = 0; i < out.size(); ++i) {
    Node *v = out[i];
    if (v->left != nullptr) {
      out.push_back(v->left);
    }
    if (v->right != nullptr) {
      out.push_back(v->right);
    }
  }
  return out;
}

} // namespace adt

#endif // C7_LINKED_BINARY_TREE_TPP