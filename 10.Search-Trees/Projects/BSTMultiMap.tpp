/**
 * @file BSTMultiMap.tpp
 * @brief Implementation of ds::BSTMultiMap.  Included by BSTMultiMap.hpp;
 *        do not compile or include directly.
 */

#ifndef DS_BST_MULTI_MAP_TPP
#define DS_BST_MULTI_MAP_TPP

#include <algorithm>
#include <stdexcept>

namespace ds {

// =============================================================================
// Construction / assignment / destruction
// =============================================================================

template <typename Key, typename T, typename Compare>
BSTMultiMap<Key, T, Compare>::BSTMultiMap(
    std::initializer_list<value_type> init, const Compare &comp)
    : comp_(comp) {
  for (const auto &e : init)
    insert(e.first, e.second);
}

template <typename Key, typename T, typename Compare>
BSTMultiMap<Key, T, Compare>::BSTMultiMap(const BSTMultiMap &other)
    : size_(other.size_), comp_(other.comp_) {
  root_ = cloneSubtree(other.root_, nullptr);
}

template <typename Key, typename T, typename Compare>
BSTMultiMap<Key, T, Compare>::BSTMultiMap(BSTMultiMap &&other) noexcept
    : root_(other.root_), size_(other.size_), comp_(std::move(other.comp_)) {
  other.root_ = nullptr;
  other.size_ = 0;
}

template <typename Key, typename T, typename Compare>
BSTMultiMap<Key, T, Compare> &
BSTMultiMap<Key, T, Compare>::operator=(BSTMultiMap other) noexcept {
  swap(other);
  return *this;
}

template <typename Key, typename T, typename Compare>
BSTMultiMap<Key, T, Compare>::~BSTMultiMap() {
  destroySubtree(root_);
}

template <typename Key, typename T, typename Compare>
void BSTMultiMap<Key, T, Compare>::swap(BSTMultiMap &other) noexcept {
  using std::swap;
  swap(root_, other.root_);
  swap(size_, other.size_);
  swap(comp_, other.comp_);
}

// =============================================================================
// Capacity
// =============================================================================

template <typename Key, typename T, typename Compare>
int BSTMultiMap<Key, T, Compare>::height() const noexcept {
  return heightOf(root_);
}

// =============================================================================
// Static navigation helpers
// =============================================================================

template <typename Key, typename T, typename Compare>
typename BSTMultiMap<Key, T, Compare>::Node *
BSTMultiMap<Key, T, Compare>::minNode(Node *n) noexcept {
  if (n)
    while (n->left)
      n = n->left;
  return n;
}

template <typename Key, typename T, typename Compare>
typename BSTMultiMap<Key, T, Compare>::Node *
BSTMultiMap<Key, T, Compare>::maxNode(Node *n) noexcept {
  if (n)
    while (n->right)
      n = n->right;
  return n;
}

template <typename Key, typename T, typename Compare>
typename BSTMultiMap<Key, T, Compare>::Node *
BSTMultiMap<Key, T, Compare>::successor(Node *n) noexcept {
  if (n->right)
    return minNode(n->right);
  Node *p = n->parent;
  while (p && n == p->right) {
    n = p;
    p = p->parent;
  }
  return p;
}

template <typename Key, typename T, typename Compare>
typename BSTMultiMap<Key, T, Compare>::Node *
BSTMultiMap<Key, T, Compare>::predecessor(Node *n) noexcept {
  if (n->left)
    return maxNode(n->left);
  Node *p = n->parent;
  while (p && n == p->left) {
    n = p;
    p = p->parent;
  }
  return p;
}

// =============================================================================
// replaceChild helper
// =============================================================================

template <typename Key, typename T, typename Compare>
void BSTMultiMap<Key, T, Compare>::replaceChild(Node *parent, Node *oldChild,
                                                Node *newChild) noexcept {
  if (!parent) {
    root_ = newChild;
  } else if (parent->left == oldChild) {
    parent->left = newChild;
  } else {
    parent->right = newChild;
  }
}

// =============================================================================
// Dictionary ADT operations
// =============================================================================

template <typename Key, typename T, typename Compare>
template <typename V>
typename BSTMultiMap<Key, T, Compare>::iterator
BSTMultiMap<Key, T, Compare>::insert(const Key &key, V &&value) {
  Node *n = insertNode(key, std::forward<V>(value));
  return iterator(n, this);
}

template <typename Key, typename T, typename Compare>
template <typename... Args>
typename BSTMultiMap<Key, T, Compare>::Node *
BSTMultiMap<Key, T, Compare>::insertNode(const Key &key, Args &&...args) {
  Node *parent = nullptr;
  Node *cur = root_;
  bool goLeft = false;

  while (cur) {
    parent = cur;
    if (comp_(key, cur->kv.first)) {
      goLeft = true;
      cur = cur->left;
    } else {
      // key >= cur->key: go right (duplicates are placed to the right of
      // existing equal keys so that inorder produces non-decreasing keys).
      goLeft = false;
      cur = cur->right;
    }
  }

  Node *node = new Node(parent, key, std::forward<Args>(args)...);
  if (!parent) {
    root_ = node;
  } else if (goLeft) {
    parent->left = node;
  } else {
    parent->right = node;
  }
  ++size_;
  return node;
}

template <typename Key, typename T, typename Compare>
typename BSTMultiMap<Key, T, Compare>::Node *
BSTMultiMap<Key, T, Compare>::findNode(const Key &key) const {
  // Walk down; track the best candidate (the leftmost equal key encountered
  // so far by scanning inorder for the first match is O(h) with this trick).
  Node *cur = root_;
  Node *candidate = nullptr;
  while (cur) {
    if (comp_(key, cur->kv.first)) {
      cur = cur->left;
    } else if (comp_(cur->kv.first, key)) {
      cur = cur->right;
    } else {
      // Equal key found.  Because duplicates are placed to the right, the
      // leftmost occurrence is in the left subtree relative to the current
      // candidate.  Record this node and keep searching left.
      candidate = cur;
      cur = cur->left;
    }
  }
  return candidate;
}

template <typename Key, typename T, typename Compare>
typename BSTMultiMap<Key, T, Compare>::iterator
BSTMultiMap<Key, T, Compare>::find(const Key &key) {
  return iterator(findNode(key), this);
}

template <typename Key, typename T, typename Compare>
typename BSTMultiMap<Key, T, Compare>::const_iterator
BSTMultiMap<Key, T, Compare>::find(const Key &key) const {
  return const_iterator(findNode(key), this);
}

template <typename Key, typename T, typename Compare>
bool BSTMultiMap<Key, T, Compare>::contains(const Key &key) const {
  return findNode(key) != nullptr;
}

template <typename Key, typename T, typename Compare>
typename BSTMultiMap<Key, T, Compare>::entry_set
BSTMultiMap<Key, T, Compare>::findAll(const Key &key) const {
  entry_set result;
  // Start from the leftmost match and collect consecutive equal keys.
  for (auto it = find(key); it != end() && !comp_(key, it->first); ++it)
    result.push_back(&(*it));
  return result;
}

template <typename Key, typename T, typename Compare>
typename BSTMultiMap<Key, T, Compare>::iterator
BSTMultiMap<Key, T, Compare>::erase(const_iterator pos) {
  Node *n = pos.node_;
  Node *next = successor(n);
  eraseNode(n);
  return iterator(next, this);
}

template <typename Key, typename T, typename Compare>
bool BSTMultiMap<Key, T, Compare>::erase(const Key &key) {
  Node *n = findNode(key);
  if (!n)
    return false;
  eraseNode(n);
  return true;
}

template <typename Key, typename T, typename Compare>
typename BSTMultiMap<Key, T, Compare>::size_type
BSTMultiMap<Key, T, Compare>::eraseAll(const Key &key) {
  size_type removed = 0;
  for (auto it = find(key); it != end() && !comp_(key, it->first);) {
    it = erase(it);
    ++removed;
  }
  return removed;
}

template <typename Key, typename T, typename Compare>
void BSTMultiMap<Key, T, Compare>::eraseNode(Node *z) noexcept {
  if (!z->left || !z->right) {
    // Zero or one child: splice out directly.
    Node *child = z->left ? z->left : z->right;
    if (child)
      child->parent = z->parent;
    replaceChild(z->parent, z, child);
  } else {
    // Two children: replace with in-order successor (minimum of right
    // subtree). Relink nodes without copying so that iterators to other
    // nodes remain valid.
    Node *s = minNode(z->right);
    if (s->parent != z) {
      // Detach successor from its current position.
      s->parent->left = s->right;
      if (s->right)
        s->right->parent = s->parent;
      s->right = z->right;
      z->right->parent = s;
    }
    s->left = z->left;
    z->left->parent = s;
    s->parent = z->parent;
    replaceChild(z->parent, z, s);
  }

  delete z;
  --size_;
}

template <typename Key, typename T, typename Compare>
void BSTMultiMap<Key, T, Compare>::clear() noexcept {
  destroySubtree(root_);
  root_ = nullptr;
  size_ = 0;
}

// =============================================================================
// Traversal collectors (P-10.11)
// =============================================================================

template <typename Key, typename T, typename Compare>
void BSTMultiMap<Key, T, Compare>::collectPreorder(const Node *n,
                                                   entry_set &out) {
  if (!n)
    return;
  out.push_back(&n->kv);
  collectPreorder(n->left, out);
  collectPreorder(n->right, out);
}

template <typename Key, typename T, typename Compare>
void BSTMultiMap<Key, T, Compare>::collectInorder(const Node *n,
                                                  entry_set &out) {
  if (!n)
    return;
  collectInorder(n->left, out);
  out.push_back(&n->kv);
  collectInorder(n->right, out);
}

template <typename Key, typename T, typename Compare>
void BSTMultiMap<Key, T, Compare>::collectPostorder(const Node *n,
                                                    entry_set &out) {
  if (!n)
    return;
  collectPostorder(n->left, out);
  collectPostorder(n->right, out);
  out.push_back(&n->kv);
}

template <typename Key, typename T, typename Compare>
typename BSTMultiMap<Key, T, Compare>::entry_set
BSTMultiMap<Key, T, Compare>::entrySetPreorder() const {
  entry_set out;
  out.reserve(size_);
  collectPreorder(root_, out);
  return out;
}

template <typename Key, typename T, typename Compare>
typename BSTMultiMap<Key, T, Compare>::entry_set
BSTMultiMap<Key, T, Compare>::entrySetInorder() const {
  entry_set out;
  out.reserve(size_);
  collectInorder(root_, out);
  return out;
}

template <typename Key, typename T, typename Compare>
typename BSTMultiMap<Key, T, Compare>::entry_set
BSTMultiMap<Key, T, Compare>::entrySetPostorder() const {
  entry_set out;
  out.reserve(size_);
  collectPostorder(root_, out);
  return out;
}

// =============================================================================
// Memory management helpers
// =============================================================================

template <typename Key, typename T, typename Compare>
typename BSTMultiMap<Key, T, Compare>::Node *
BSTMultiMap<Key, T, Compare>::cloneSubtree(const Node *src, Node *parent) {
  if (!src)
    return nullptr;
  Node *n = new Node(parent, src->kv.first, src->kv.second);
  try {
    n->left = cloneSubtree(src->left, n);
    n->right = cloneSubtree(src->right, n);
  } catch (...) {
    destroySubtree(n);
    throw;
  }
  return n;
}

template <typename Key, typename T, typename Compare>
void BSTMultiMap<Key, T, Compare>::destroySubtree(Node *n) noexcept {
  if (!n)
    return;
  destroySubtree(n->left);
  destroySubtree(n->right);
  delete n;
}

template <typename Key, typename T, typename Compare>
int BSTMultiMap<Key, T, Compare>::heightOf(const Node *n) noexcept {
  if (!n)
    return 0;
  return 1 + std::max(heightOf(n->left), heightOf(n->right));
}

// =============================================================================
// Validation (testing / debugging)
// =============================================================================

template <typename Key, typename T, typename Compare>
bool BSTMultiMap<Key, T, Compare>::validateSubtree(const Node *n,
                                                   const Node *parent,
                                                   size_type &count) const {
  if (!n)
    return true;
  if (n->parent != parent)
    return false;
  if (n->left && comp_(n->kv.first, n->left->kv.first))
    return false;
  if (n->right && comp_(n->right->kv.first, n->kv.first))
    return false;
  ++count;
  return validateSubtree(n->left, n, count) &&
         validateSubtree(n->right, n, count);
}

template <typename Key, typename T, typename Compare>
bool BSTMultiMap<Key, T, Compare>::isValid() const {
  size_type count = 0;
  if (!validateSubtree(root_, nullptr, count))
    return false;
  if (count != size_)
    return false;
  // Check non-decreasing inorder sequence (duplicates allowed → non-strict).
  const Node *prev = nullptr;
  for (const Node *n = minNode(root_); n;
       n = successor(const_cast<Node *>(n))) {
    if (prev && comp_(n->kv.first, prev->kv.first))
      return false;
    prev = n;
  }
  return true;
}

} // namespace ds

#endif // DS_BST_MULTI_MAP_TPP
