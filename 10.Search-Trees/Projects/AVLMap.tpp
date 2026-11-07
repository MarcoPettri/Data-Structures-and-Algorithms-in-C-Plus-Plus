/**
 * @file AVLMap.tpp
 * @brief Implementation of ds::AVLMap. Included by AVLMap.hpp; do not compile
 *        or include directly.
 */

#ifndef DS_AVL_MAP_TPP
#define DS_AVL_MAP_TPP

#include <algorithm>
#include <cstdlib>
#include <stdexcept>
namespace ds {

// ============================================================================
// Construction / assignment / destruction
// ============================================================================

template <typename Key, typename T, typename Compare>
AVLMap<Key, T, Compare>::AVLMap(std::initializer_list<value_type> init,
                                const Compare &comp)
    : comp_(comp) {
  for (const auto &e : init) {
    put(e.first, e.second);
  }
}

template <typename Key, typename T, typename Compare>
AVLMap<Key, T, Compare>::AVLMap(const AVLMap &other)
    : size_(other.size_), comp_(other.comp_) {
  root_ = cloneSubtree(other.root_, nullptr);
}

template <typename Key, typename T, typename Compare>
AVLMap<Key, T, Compare>::AVLMap(AVLMap &&other) noexcept
    : root_(other.root_), size_(other.size_), comp_(std::move(other.comp_)) {
  other.root_ = nullptr;
  other.size_ = 0;
}

template <typename Key, typename T, typename Compare>
AVLMap<Key, T, Compare> &
AVLMap<Key, T, Compare>::operator=(AVLMap other) noexcept {
  swap(other);
  return *this;
}

template <typename Key, typename T, typename Compare>
AVLMap<Key, T, Compare>::~AVLMap() {
  destroySubtree(root_);
}

template <typename Key, typename T, typename Compare>
void AVLMap<Key, T, Compare>::swap(AVLMap &other) noexcept {
  using std::swap;
  swap(root_, other.root_);
  swap(size_, other.size_);
  swap(comp_, other.comp_);
}

// ============================================================================
// Capacity
// ============================================================================

template <typename Key, typename T, typename Compare>
int AVLMap<Key, T, Compare>::height() const noexcept {
  return heightOf(root_);
}

// ============================================================================
// Static navigation helpers
// ============================================================================

template <typename Key, typename T, typename Compare>
typename AVLMap<Key, T, Compare>::Node *
AVLMap<Key, T, Compare>::minNode(Node *n) noexcept {
  if (n) {
    while (n->left)
      n = n->left;
  }
  return n;
}

template <typename Key, typename T, typename Compare>
typename AVLMap<Key, T, Compare>::Node *
AVLMap<Key, T, Compare>::maxNode(Node *n) noexcept {
  if (n) {
    while (n->right)
      n = n->right;
  }
  return n;
}

template <typename Key, typename T, typename Compare>
typename AVLMap<Key, T, Compare>::Node *
AVLMap<Key, T, Compare>::successor(Node *n) noexcept {
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
typename AVLMap<Key, T, Compare>::Node *
AVLMap<Key, T, Compare>::predecessor(Node *n) noexcept {
  if (n->left)
    return maxNode(n->left);
  Node *p = n->parent;
  while (p && n == p->left) {
    n = p;
    p = p->parent;
  }
  return p;
}

// ============================================================================
// AVL balancing
// ============================================================================

template <typename Key, typename T, typename Compare>
void AVLMap<Key, T, Compare>::updateHeight(Node *n) noexcept {
  n->height = 1 + std::max(heightOf(n->left), heightOf(n->right));
}

template <typename Key, typename T, typename Compare>
void AVLMap<Key, T, Compare>::replaceChild(Node *parent, Node *oldChild,
                                           Node *newChild) noexcept {
  if (!parent) {
    root_ = newChild;
  } else if (parent->left == oldChild) {
    parent->left = newChild;
  } else {
    parent->right = newChild;
  }
}

template <typename Key, typename T, typename Compare>
typename AVLMap<Key, T, Compare>::Node *
AVLMap<Key, T, Compare>::rotateLeft(Node *x) noexcept {
  Node *y = x->right;
  x->right = y->left;
  if (y->left)
    y->left->parent = x;
  y->parent = x->parent;
  replaceChild(x->parent, x, y);
  y->left = x;
  x->parent = y;
  updateHeight(x);
  updateHeight(y);
  return y;
}

template <typename Key, typename T, typename Compare>
typename AVLMap<Key, T, Compare>::Node *
AVLMap<Key, T, Compare>::rotateRight(Node *x) noexcept {
  Node *y = x->left;
  x->left = y->right;
  if (y->right)
    y->right->parent = x;
  y->parent = x->parent;
  replaceChild(x->parent, x, y);
  y->right = x;
  x->parent = y;
  updateHeight(x);
  updateHeight(y);
  return y;
}

template <typename Key, typename T, typename Compare>
void AVLMap<Key, T, Compare>::rebalanceUp(Node *n) noexcept {
  while (n) {
    Node *const parent = n->parent;
    const int oldHeight = n->height;
    updateHeight(n);
    const int bf = balanceOf(n);

    if (bf > 1) { // left heavy
      if (balanceOf(n->left) < 0)
        rotateLeft(n->left); // left-right case
      n = rotateRight(n);
    } else if (bf < -1) { // right heavy
      if (balanceOf(n->right) > 0)
        rotateRight(n->right); // right-left case
      n = rotateLeft(n);
    } else if (n->height == oldHeight) {
      break; // height unchanged and balanced: ancestors are unaffected
    }
    n = parent;
  }
}

// ============================================================================
// Lookup helpers
// ============================================================================

template <typename Key, typename T, typename Compare>
typename AVLMap<Key, T, Compare>::Node *
AVLMap<Key, T, Compare>::findNode(const Key &key) const {
  Node *cur = root_;
  while (cur) {
    if (comp_(key, cur->kv.first)) {
      cur = cur->left;
    } else if (comp_(cur->kv.first, key)) {
      cur = cur->right;
    } else {
      return cur;
    }
  }
  return nullptr;
}

template <typename Key, typename T, typename Compare>
typename AVLMap<Key, T, Compare>::Node *
AVLMap<Key, T, Compare>::ceilingNode(const Key &key) const {
  Node *cur = root_;
  Node *best = nullptr;
  while (cur) {
    if (comp_(cur->kv.first, key)) {
      cur = cur->right;
    } else {
      best = cur;
      cur = cur->left;
    }
  }
  return best;
}

template <typename Key, typename T, typename Compare>
typename AVLMap<Key, T, Compare>::Node *
AVLMap<Key, T, Compare>::floorNode(const Key &key) const {
  Node *cur = root_;
  Node *best = nullptr;
  while (cur) {
    if (comp_(key, cur->kv.first)) {
      cur = cur->left;
    } else {
      best = cur;
      cur = cur->right;
    }
  }
  return best;
}

template <typename Key, typename T, typename Compare>
typename AVLMap<Key, T, Compare>::Node *
AVLMap<Key, T, Compare>::higherNode(const Key &key) const {
  Node *cur = root_;
  Node *best = nullptr;
  while (cur) {
    if (comp_(key, cur->kv.first)) {
      best = cur;
      cur = cur->left;
    } else {
      cur = cur->right;
    }
  }
  return best;
}

template <typename Key, typename T, typename Compare>
typename AVLMap<Key, T, Compare>::Node *
AVLMap<Key, T, Compare>::lowerNode(const Key &key) const {
  Node *cur = root_;
  Node *best = nullptr;
  while (cur) {
    if (comp_(cur->kv.first, key)) {
      best = cur;
      cur = cur->right;
    } else {
      cur = cur->left;
    }
  }
  return best;
}

// ============================================================================
// Map ADT operations
// ============================================================================

template <typename Key, typename T, typename Compare>
typename AVLMap<Key, T, Compare>::iterator
AVLMap<Key, T, Compare>::find(const Key &key) {
  return iterator(findNode(key), this);
}

template <typename Key, typename T, typename Compare>
typename AVLMap<Key, T, Compare>::const_iterator
AVLMap<Key, T, Compare>::find(const Key &key) const {
  return const_iterator(findNode(key), this);
}

template <typename Key, typename T, typename Compare>
bool AVLMap<Key, T, Compare>::contains(const Key &key) const {
  return findNode(key) != nullptr;
}

template <typename Key, typename T, typename Compare>
T &AVLMap<Key, T, Compare>::at(const Key &key) {
  Node *n = findNode(key);
  if (!n)
    throw std::out_of_range("AVLMap::at: key not found");
  return n->kv.second;
}

template <typename Key, typename T, typename Compare>
const T &AVLMap<Key, T, Compare>::at(const Key &key) const {
  const Node *n = findNode(key);
  if (!n)
    throw std::out_of_range("AVLMap::at: key not found");
  return n->kv.second;
}

template <typename Key, typename T, typename Compare>
T &AVLMap<Key, T, Compare>::operator[](const Key &key) {
  return insertImpl(key).first->kv.second;
}

template <typename Key, typename T, typename Compare>
template <typename V>
std::pair<typename AVLMap<Key, T, Compare>::iterator, bool>
AVLMap<Key, T, Compare>::put(const Key &key, V &&value) {
  auto [node, inserted] = insertImpl(key, std::forward<V>(value));
  if (!inserted)
    node->kv.second = std::forward<V>(value); // args were not consumed
  return {iterator(node, this), inserted};
}

template <typename Key, typename T, typename Compare>
template <typename V>
std::pair<typename AVLMap<Key, T, Compare>::iterator, bool>
AVLMap<Key, T, Compare>::insert(const Key &key, V &&value) {
  auto [node, inserted] = insertImpl(key, std::forward<V>(value));
  return {iterator(node, this), inserted};
}

template <typename Key, typename T, typename Compare>
template <typename... Args>
std::pair<typename AVLMap<Key, T, Compare>::Node *, bool>
AVLMap<Key, T, Compare>::insertImpl(const Key &key, Args &&...args) {
  Node *parent = nullptr;
  Node *cur = root_;
  bool goLeft = false;

  while (cur) {
    parent = cur;
    if (comp_(key, cur->kv.first)) {
      goLeft = true;
      cur = cur->left;
    } else if (comp_(cur->kv.first, key)) {
      goLeft = false;
      cur = cur->right;
    } else {
      return {cur, false};
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
  rebalanceUp(parent);
  return {node, true};
}

template <typename Key, typename T, typename Compare>
bool AVLMap<Key, T, Compare>::erase(const Key &key) {
  Node *n = findNode(key);
  if (!n)
    return false;
  eraseNode(n);
  return true;
}

template <typename Key, typename T, typename Compare>
typename AVLMap<Key, T, Compare>::iterator
AVLMap<Key, T, Compare>::erase(const_iterator pos) {
  Node *n = pos.node_;
  Node *next = successor(n); // computed before the node disappears
  eraseNode(n);
  return iterator(next, this);
}

template <typename Key, typename T, typename Compare>
void AVLMap<Key, T, Compare>::eraseNode(Node *z) noexcept {
  Node *rebalanceStart = nullptr;

  if (!z->left || !z->right) {
    // Zero or one child: splice the node out.
    Node *child = z->left ? z->left : z->right;
    rebalanceStart = z->parent;
    if (child)
      child->parent = z->parent;
    replaceChild(z->parent, z, child);
  } else {
    // Two children: relink the in-order successor into z's position
    // (nodes are relinked, never copied, so other iterators stay valid).
    Node *s = minNode(z->right);
    if (s->parent == z) {
      rebalanceStart = s;
    } else {
      rebalanceStart = s->parent;
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
    s->height = z->height;
  }

  delete z;
  --size_;
  rebalanceUp(rebalanceStart);
}

template <typename Key, typename T, typename Compare>
void AVLMap<Key, T, Compare>::clear() noexcept {
  destroySubtree(root_);
  root_ = nullptr;
  size_ = 0;
}

// ============================================================================
// Ordered map ADT operations
// ============================================================================

template <typename Key, typename T, typename Compare>
typename AVLMap<Key, T, Compare>::iterator
AVLMap<Key, T, Compare>::lastEntry() noexcept {
  return iterator(maxNode(root_), this);
}

template <typename Key, typename T, typename Compare>
typename AVLMap<Key, T, Compare>::const_iterator
AVLMap<Key, T, Compare>::lastEntry() const noexcept {
  return const_iterator(maxNode(root_), this);
}

template <typename Key, typename T, typename Compare>
typename AVLMap<Key, T, Compare>::iterator
AVLMap<Key, T, Compare>::ceilingEntry(const Key &key) {
  return iterator(ceilingNode(key), this);
}

template <typename Key, typename T, typename Compare>
typename AVLMap<Key, T, Compare>::const_iterator
AVLMap<Key, T, Compare>::ceilingEntry(const Key &key) const {
  return const_iterator(ceilingNode(key), this);
}

template <typename Key, typename T, typename Compare>
typename AVLMap<Key, T, Compare>::iterator
AVLMap<Key, T, Compare>::floorEntry(const Key &key) {
  return iterator(floorNode(key), this);
}

template <typename Key, typename T, typename Compare>
typename AVLMap<Key, T, Compare>::const_iterator
AVLMap<Key, T, Compare>::floorEntry(const Key &key) const {
  return const_iterator(floorNode(key), this);
}

template <typename Key, typename T, typename Compare>
typename AVLMap<Key, T, Compare>::iterator
AVLMap<Key, T, Compare>::higherEntry(const Key &key) {
  return iterator(higherNode(key), this);
}

template <typename Key, typename T, typename Compare>
typename AVLMap<Key, T, Compare>::const_iterator
AVLMap<Key, T, Compare>::higherEntry(const Key &key) const {
  return const_iterator(higherNode(key), this);
}

template <typename Key, typename T, typename Compare>
typename AVLMap<Key, T, Compare>::iterator
AVLMap<Key, T, Compare>::lowerEntry(const Key &key) {
  return iterator(lowerNode(key), this);
}

template <typename Key, typename T, typename Compare>
typename AVLMap<Key, T, Compare>::const_iterator
AVLMap<Key, T, Compare>::lowerEntry(const Key &key) const {
  return const_iterator(lowerNode(key), this);
}

template <typename Key, typename T, typename Compare>
std::pair<typename AVLMap<Key, T, Compare>::iterator,
          typename AVLMap<Key, T, Compare>::iterator>
AVLMap<Key, T, Compare>::subMap(const Key &k1, const Key &k2) {
  if (!comp_(k1, k2))
    return {end(), end()};
  return {ceilingEntry(k1), ceilingEntry(k2)};
}

template <typename Key, typename T, typename Compare>
std::pair<typename AVLMap<Key, T, Compare>::const_iterator,
          typename AVLMap<Key, T, Compare>::const_iterator>
AVLMap<Key, T, Compare>::subMap(const Key &k1, const Key &k2) const {
  if (!comp_(k1, k2))
    return {end(), end()};
  return {ceilingEntry(k1), ceilingEntry(k2)};
}

// ============================================================================
// Memory management helpers
// ============================================================================

template <typename Key, typename T, typename Compare>
typename AVLMap<Key, T, Compare>::Node *
AVLMap<Key, T, Compare>::cloneSubtree(const Node *src, Node *parent) {
  if (!src)
    return nullptr;
  Node *n = new Node(parent, src->kv.first, src->kv.second);
  n->height = src->height;
  try {
    n->left = cloneSubtree(src->left, n);
    n->right = cloneSubtree(src->right, n);
  } catch (...) {
    destroySubtree(n); // also frees any child already attached
    throw;
  }
  return n;
}

template <typename Key, typename T, typename Compare>
void AVLMap<Key, T, Compare>::destroySubtree(Node *n) noexcept {
  // Recursion depth is bounded by the AVL height, O(log n).
  if (!n)
    return;
  destroySubtree(n->left);
  destroySubtree(n->right);
  delete n;
}

// ============================================================================
// Validation (testing / debugging)
// ============================================================================

template <typename Key, typename T, typename Compare>
int AVLMap<Key, T, Compare>::validateSubtree(const Node *n, const Node *parent,
                                             size_type &count) const {
  if (!n)
    return 0;
  if (n->parent != parent)
    return -1;
  const int lh = validateSubtree(n->left, n, count);
  if (lh < 0)
    return -1;
  const int rh = validateSubtree(n->right, n, count);
  if (rh < 0)
    return -1;
  if (std::abs(lh - rh) > 1)
    return -1;
  const int h = 1 + std::max(lh, rh);
  if (n->height != h)
    return -1;
  ++count;
  return h;
}

template <typename Key, typename T, typename Compare>
bool AVLMap<Key, T, Compare>::isValid() const {
  size_type count = 0;
  if (validateSubtree(root_, nullptr, count) < 0)
    return false;
  if (count != size_)
    return false;

  // Strict ascending order via in-order traversal.
  const Node *prev = nullptr;
  for (const Node *n = minNode(root_); n;
       n = successor(const_cast<Node *>(n))) {
    if (prev && !comp_(prev->kv.first, n->kv.first))
      return false;
    prev = n;
  }
  return true;
}

} // namespace ds

#endif // DS_AVL_MAP_TPP