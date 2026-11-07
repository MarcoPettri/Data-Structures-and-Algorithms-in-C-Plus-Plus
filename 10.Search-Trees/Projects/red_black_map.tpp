/**
 * @file red_black_map.tpp
 * @brief Implementation of ds::RedBlackMap. Included by red_black_map.hpp; do
 * not compile or include directly.
 *
 * Null pointers play the role of the (black) nil leaves, so no sentinel node is
 * allocated. Because the fix-up after erasure may start at a null child, it is
 * given the parent of that child explicitly.
 */

#ifndef DS_RED_BLACK_MAP_TPP
#define DS_RED_BLACK_MAP_TPP

#include <algorithm>

namespace ds {

// ============================================================================
// Construction / assignment / destruction
// ============================================================================

template <typename Key, typename T, typename Compare>
RedBlackMap<Key, T, Compare>::RedBlackMap(
    std::initializer_list<value_type> init, const Compare &comp)
    : comp_(comp) {
  for (const auto &e : init) {
    put(e.first, e.second);
  }
}

template <typename Key, typename T, typename Compare>
RedBlackMap<Key, T, Compare>::RedBlackMap(const RedBlackMap &other)
    : size_(other.size_), comp_(other.comp_) {
  root_ = cloneSubtree(other.root_, nullptr);
}

template <typename Key, typename T, typename Compare>
RedBlackMap<Key, T, Compare>::RedBlackMap(RedBlackMap &&other) noexcept
    : root_(other.root_), size_(other.size_), comp_(std::move(other.comp_)) {
  other.root_ = nullptr;
  other.size_ = 0;
}

template <typename Key, typename T, typename Compare>
RedBlackMap<Key, T, Compare> &
RedBlackMap<Key, T, Compare>::operator=(RedBlackMap other) noexcept {
  swap(other);
  return *this;
}

template <typename Key, typename T, typename Compare>
RedBlackMap<Key, T, Compare>::~RedBlackMap() {
  destroySubtree(root_);
}

template <typename Key, typename T, typename Compare>
void RedBlackMap<Key, T, Compare>::swap(RedBlackMap &other) noexcept {
  using std::swap;
  swap(root_, other.root_);
  swap(size_, other.size_);
  swap(comp_, other.comp_);
}

// ============================================================================
// Capacity
// ============================================================================

template <typename Key, typename T, typename Compare>
int RedBlackMap<Key, T, Compare>::heightOf(const Node *n) noexcept {
  // Recursion depth is bounded by the tree height, at most 2*log2(n+1).
  return n ? 1 + std::max(heightOf(n->left), heightOf(n->right)) : 0;
}

template <typename Key, typename T, typename Compare>
int RedBlackMap<Key, T, Compare>::height() const noexcept {
  return heightOf(root_);
}

// ============================================================================
// Static navigation helpers
// ============================================================================

template <typename Key, typename T, typename Compare>
typename RedBlackMap<Key, T, Compare>::Node *
RedBlackMap<Key, T, Compare>::minNode(Node *n) noexcept {
  if (n) {
    while (n->left)
      n = n->left;
  }
  return n;
}

template <typename Key, typename T, typename Compare>
typename RedBlackMap<Key, T, Compare>::Node *
RedBlackMap<Key, T, Compare>::maxNode(Node *n) noexcept {
  if (n) {
    while (n->right)
      n = n->right;
  }
  return n;
}

template <typename Key, typename T, typename Compare>
typename RedBlackMap<Key, T, Compare>::Node *
RedBlackMap<Key, T, Compare>::successor(Node *n) noexcept {
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
typename RedBlackMap<Key, T, Compare>::Node *
RedBlackMap<Key, T, Compare>::predecessor(Node *n) noexcept {
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
// Red-black balancing
// ============================================================================

template <typename Key, typename T, typename Compare>
void RedBlackMap<Key, T, Compare>::replaceChild(Node *parent, Node *oldChild,
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
void RedBlackMap<Key, T, Compare>::transplant(Node *u, Node *v) noexcept {
  replaceChild(u->parent, u, v);
  if (v)
    v->parent = u->parent;
}

template <typename Key, typename T, typename Compare>
void RedBlackMap<Key, T, Compare>::rotateLeft(Node *x) noexcept {
  Node *y = x->right;
  x->right = y->left;
  if (y->left)
    y->left->parent = x;
  y->parent = x->parent;
  replaceChild(x->parent, x, y);
  y->left = x;
  x->parent = y;
}

template <typename Key, typename T, typename Compare>
void RedBlackMap<Key, T, Compare>::rotateRight(Node *x) noexcept {
  Node *y = x->left;
  x->left = y->right;
  if (y->right)
    y->right->parent = x;
  y->parent = x->parent;
  replaceChild(x->parent, x, y);
  y->right = x;
  x->parent = y;
}

template <typename Key, typename T, typename Compare>
void RedBlackMap<Key, T, Compare>::insertFixup(Node *z) noexcept {
  // z is red. A red parent is never the root, so a grandparent exists.
  while (z->parent && z->parent->red) {
    Node *p = z->parent;
    Node *g = p->parent;
    if (p == g->left) {
      Node *uncle = g->right;
      if (isRed(uncle)) { // case 1: recolor, continue from the grandparent
        p->red = false;
        uncle->red = false;
        g->red = true;
        z = g;
      } else {
        if (z == p->right) { // case 2: straighten the zig-zag
          z = p;
          rotateLeft(z);
          p = z->parent;
        }
        p->red = false; // case 3
        g->red = true;
        rotateRight(g);
        break;
      }
    } else {
      Node *uncle = g->left;
      if (isRed(uncle)) {
        p->red = false;
        uncle->red = false;
        g->red = true;
        z = g;
      } else {
        if (z == p->left) {
          z = p;
          rotateRight(z);
          p = z->parent;
        }
        p->red = false;
        g->red = true;
        rotateLeft(g);
        break;
      }
    }
  }
  root_->red = false;
}

template <typename Key, typename T, typename Compare>
void RedBlackMap<Key, T, Compare>::eraseNode(Node *z) noexcept {
  Node *y = z; // node removed from, or moved within, the tree
  bool yWasRed = y->red;
  Node *x = nullptr; // child that takes y's place (may be null)
  Node *xParent =
      nullptr; // parent of x, tracked explicitly since x may be null

  if (!z->left) {
    x = z->right;
    xParent = z->parent;
    transplant(z, z->right);
  } else if (!z->right) {
    x = z->left;
    xParent = z->parent;
    transplant(z, z->left);
  } else {
    // Two children: relink the in-order successor into z's position
    // (nodes are relinked, never copied, so other iterators stay valid).
    y = minNode(z->right);
    yWasRed = y->red;
    x = y->right;
    if (y->parent == z) {
      xParent = y;
    } else {
      xParent = y->parent;
      transplant(y, y->right);
      y->right = z->right;
      y->right->parent = y;
    }
    transplant(z, y);
    y->left = z->left;
    y->left->parent = y;
    y->red = z->red;
  }

  delete z;
  --size_;
  if (!yWasRed)
    eraseFixup(x, xParent);
}

template <typename Key, typename T, typename Compare>
void RedBlackMap<Key, T, Compare>::eraseFixup(Node *x, Node *xParent) noexcept {
  // x carries an extra "double black"; push it up or resolve it with rotations.
  // The sibling always exists: the deficient side has black height >= 1 on the
  // other side.
  while (x != root_ && !isRed(x)) {
    if (x == xParent->left) {
      Node *w = xParent->right;
      if (w->red) { // case 1: red sibling -> black sibling
        w->red = false;
        xParent->red = true;
        rotateLeft(xParent);
        w = xParent->right;
      }
      if (!isRed(w->left) && !isRed(w->right)) { // case 2: recolor, move up
        w->red = true;
        x = xParent;
        xParent = x->parent;
      } else {
        if (!isRed(w->right)) { // case 3: near nephew red -> far nephew red
          w->left->red = false;
          w->red = true;
          rotateRight(w);
          w = xParent->right;
        }
        w->red = xParent->red; // case 4: terminal
        xParent->red = false;
        if (w->right)
          w->right->red = false;
        rotateLeft(xParent);
        x = root_;
        break;
      }
    } else {
      Node *w = xParent->left;
      if (w->red) {
        w->red = false;
        xParent->red = true;
        rotateRight(xParent);
        w = xParent->left;
      }
      if (!isRed(w->left) && !isRed(w->right)) {
        w->red = true;
        x = xParent;
        xParent = x->parent;
      } else {
        if (!isRed(w->left)) {
          w->right->red = false;
          w->red = true;
          rotateLeft(w);
          w = xParent->left;
        }
        w->red = xParent->red;
        xParent->red = false;
        if (w->left)
          w->left->red = false;
        rotateRight(xParent);
        x = root_;
        break;
      }
    }
  }
  if (x)
    x->red = false;
}

// ============================================================================
// Lookup helpers
// ============================================================================

template <typename Key, typename T, typename Compare>
typename RedBlackMap<Key, T, Compare>::Node *
RedBlackMap<Key, T, Compare>::findNode(const Key &key) const {
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
typename RedBlackMap<Key, T, Compare>::Node *
RedBlackMap<Key, T, Compare>::ceilingNode(const Key &key) const {
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
typename RedBlackMap<Key, T, Compare>::Node *
RedBlackMap<Key, T, Compare>::floorNode(const Key &key) const {
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
typename RedBlackMap<Key, T, Compare>::Node *
RedBlackMap<Key, T, Compare>::higherNode(const Key &key) const {
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
typename RedBlackMap<Key, T, Compare>::Node *
RedBlackMap<Key, T, Compare>::lowerNode(const Key &key) const {
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
typename RedBlackMap<Key, T, Compare>::iterator
RedBlackMap<Key, T, Compare>::find(const Key &key) {
  return iterator(findNode(key), this);
}

template <typename Key, typename T, typename Compare>
typename RedBlackMap<Key, T, Compare>::const_iterator
RedBlackMap<Key, T, Compare>::find(const Key &key) const {
  return const_iterator(findNode(key), this);
}

template <typename Key, typename T, typename Compare>
bool RedBlackMap<Key, T, Compare>::contains(const Key &key) const {
  return findNode(key) != nullptr;
}

template <typename Key, typename T, typename Compare>
T &RedBlackMap<Key, T, Compare>::at(const Key &key) {
  Node *n = findNode(key);
  if (!n)
    throw std::out_of_range("RedBlackMap::at: key not found");
  return n->kv.second;
}

template <typename Key, typename T, typename Compare>
const T &RedBlackMap<Key, T, Compare>::at(const Key &key) const {
  const Node *n = findNode(key);
  if (!n)
    throw std::out_of_range("RedBlackMap::at: key not found");
  return n->kv.second;
}

template <typename Key, typename T, typename Compare>
T &RedBlackMap<Key, T, Compare>::operator[](const Key &key) {
  return insertImpl(key).first->kv.second;
}

template <typename Key, typename T, typename Compare>
template <typename V>
std::pair<typename RedBlackMap<Key, T, Compare>::iterator, bool>
RedBlackMap<Key, T, Compare>::put(const Key &key, V &&value) {
  auto [node, inserted] = insertImpl(key, std::forward<V>(value));
  if (!inserted)
    node->kv.second = std::forward<V>(value); // args were not consumed
  return {iterator(node, this), inserted};
}

template <typename Key, typename T, typename Compare>
template <typename V>
std::pair<typename RedBlackMap<Key, T, Compare>::iterator, bool>
RedBlackMap<Key, T, Compare>::insert(const Key &key, V &&value) {
  auto [node, inserted] = insertImpl(key, std::forward<V>(value));
  return {iterator(node, this), inserted};
}

template <typename Key, typename T, typename Compare>
template <typename... Args>
std::pair<typename RedBlackMap<Key, T, Compare>::Node *, bool>
RedBlackMap<Key, T, Compare>::insertImpl(const Key &key, Args &&...args) {
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
  insertFixup(node);
  return {node, true};
}

template <typename Key, typename T, typename Compare>
bool RedBlackMap<Key, T, Compare>::erase(const Key &key) {
  Node *n = findNode(key);
  if (!n)
    return false;
  eraseNode(n);
  return true;
}

template <typename Key, typename T, typename Compare>
typename RedBlackMap<Key, T, Compare>::iterator
RedBlackMap<Key, T, Compare>::erase(const_iterator pos) {
  Node *n = pos.node_;
  Node *next = successor(n); // computed before the node disappears
  eraseNode(n);
  return iterator(next, this);
}

template <typename Key, typename T, typename Compare>
void RedBlackMap<Key, T, Compare>::clear() noexcept {
  destroySubtree(root_);
  root_ = nullptr;
  size_ = 0;
}

// ============================================================================
// Ordered map ADT operations
// ============================================================================

template <typename Key, typename T, typename Compare>
typename RedBlackMap<Key, T, Compare>::iterator
RedBlackMap<Key, T, Compare>::lastEntry() noexcept {
  return iterator(maxNode(root_), this);
}

template <typename Key, typename T, typename Compare>
typename RedBlackMap<Key, T, Compare>::const_iterator
RedBlackMap<Key, T, Compare>::lastEntry() const noexcept {
  return const_iterator(maxNode(root_), this);
}

template <typename Key, typename T, typename Compare>
typename RedBlackMap<Key, T, Compare>::iterator
RedBlackMap<Key, T, Compare>::ceilingEntry(const Key &key) {
  return iterator(ceilingNode(key), this);
}

template <typename Key, typename T, typename Compare>
typename RedBlackMap<Key, T, Compare>::const_iterator
RedBlackMap<Key, T, Compare>::ceilingEntry(const Key &key) const {
  return const_iterator(ceilingNode(key), this);
}

template <typename Key, typename T, typename Compare>
typename RedBlackMap<Key, T, Compare>::iterator
RedBlackMap<Key, T, Compare>::floorEntry(const Key &key) {
  return iterator(floorNode(key), this);
}

template <typename Key, typename T, typename Compare>
typename RedBlackMap<Key, T, Compare>::const_iterator
RedBlackMap<Key, T, Compare>::floorEntry(const Key &key) const {
  return const_iterator(floorNode(key), this);
}

template <typename Key, typename T, typename Compare>
typename RedBlackMap<Key, T, Compare>::iterator
RedBlackMap<Key, T, Compare>::higherEntry(const Key &key) {
  return iterator(higherNode(key), this);
}

template <typename Key, typename T, typename Compare>
typename RedBlackMap<Key, T, Compare>::const_iterator
RedBlackMap<Key, T, Compare>::higherEntry(const Key &key) const {
  return const_iterator(higherNode(key), this);
}

template <typename Key, typename T, typename Compare>
typename RedBlackMap<Key, T, Compare>::iterator
RedBlackMap<Key, T, Compare>::lowerEntry(const Key &key) {
  return iterator(lowerNode(key), this);
}

template <typename Key, typename T, typename Compare>
typename RedBlackMap<Key, T, Compare>::const_iterator
RedBlackMap<Key, T, Compare>::lowerEntry(const Key &key) const {
  return const_iterator(lowerNode(key), this);
}

template <typename Key, typename T, typename Compare>
std::pair<typename RedBlackMap<Key, T, Compare>::iterator,
          typename RedBlackMap<Key, T, Compare>::iterator>
RedBlackMap<Key, T, Compare>::subMap(const Key &k1, const Key &k2) {
  if (!comp_(k1, k2))
    return {end(), end()};
  return {ceilingEntry(k1), ceilingEntry(k2)};
}

template <typename Key, typename T, typename Compare>
std::pair<typename RedBlackMap<Key, T, Compare>::const_iterator,
          typename RedBlackMap<Key, T, Compare>::const_iterator>
RedBlackMap<Key, T, Compare>::subMap(const Key &k1, const Key &k2) const {
  if (!comp_(k1, k2))
    return {end(), end()};
  return {ceilingEntry(k1), ceilingEntry(k2)};
}

// ============================================================================
// Memory management helpers
// ============================================================================

template <typename Key, typename T, typename Compare>
typename RedBlackMap<Key, T, Compare>::Node *
RedBlackMap<Key, T, Compare>::cloneSubtree(const Node *src, Node *parent) {
  if (!src)
    return nullptr;
  Node *n = new Node(parent, src->kv.first, src->kv.second);
  n->red = src->red;
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
void RedBlackMap<Key, T, Compare>::destroySubtree(Node *n) noexcept {
  // Recursion depth is bounded by the tree height, O(log n).
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
int RedBlackMap<Key, T, Compare>::validateSubtree(const Node *n,
                                                  const Node *parent,
                                                  size_type &count) const {
  // Returns the black height of the subtree, or -1 if an invariant is broken.
  if (!n)
    return 1; // null leaves count as one black node
  if (n->parent != parent)
    return -1;
  if (n->red && (isRed(n->left) || isRed(n->right)))
    return -1; // red-red edge
  const int lb = validateSubtree(n->left, n, count);
  if (lb < 0)
    return -1;
  const int rb = validateSubtree(n->right, n, count);
  if (rb < 0 || lb != rb)
    return -1;
  ++count;
  return lb + (n->red ? 0 : 1);
}

template <typename Key, typename T, typename Compare>
bool RedBlackMap<Key, T, Compare>::isValid() const {
  if (isRed(root_))
    return false; // root must be black
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

#endif // DS_RED_BLACK_MAP_TPP