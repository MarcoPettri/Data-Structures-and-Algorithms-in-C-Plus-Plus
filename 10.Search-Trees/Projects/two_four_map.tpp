/**
 * @file two_four_map.tpp
 * @brief Implementation of ds::TwoFourMap. Included by two_four_map.hpp; do not
 *        compile or include directly.
 *
 * Algorithms:
 *  - Insert: one read-only descent to detect duplicates, then a top-down pass
 *    that splits every full node (3 entries) met on the way, so the target leaf
 *    always has room and no upward propagation is needed.
 *  - Erase: locate the entry, replace an internal entry by its predecessor
 * taken from a leaf, then repair an emptied node bottom-up by borrowing from a
 *    sibling (transfer) or merging with it (fusion).
 */

#ifndef DS_TWO_FOUR_MAP_TPP
#define DS_TWO_FOUR_MAP_TPP

namespace ds {

// ============================================================================
// Construction / assignment / destruction
// ============================================================================

template <typename Key, typename T, typename Compare>
TwoFourMap<Key, T, Compare>::TwoFourMap(std::initializer_list<value_type> init,
                                        const Compare &comp)
    : comp_(comp) {
  for (const auto &e : init) {
    put(e.first, e.second);
  }
}

template <typename Key, typename T, typename Compare>
TwoFourMap<Key, T, Compare>::TwoFourMap(const TwoFourMap &other)
    : size_(other.size_), comp_(other.comp_) {
  root_ = cloneSubtree(other.root_, nullptr);
}

template <typename Key, typename T, typename Compare>
TwoFourMap<Key, T, Compare>::TwoFourMap(TwoFourMap &&other) noexcept
    : root_(other.root_), size_(other.size_), comp_(std::move(other.comp_)) {
  other.root_ = nullptr;
  other.size_ = 0;
}

template <typename Key, typename T, typename Compare>
TwoFourMap<Key, T, Compare> &
TwoFourMap<Key, T, Compare>::operator=(TwoFourMap other) noexcept {
  swap(other);
  return *this;
}

template <typename Key, typename T, typename Compare>
TwoFourMap<Key, T, Compare>::~TwoFourMap() {
  destroySubtree(root_);
}

template <typename Key, typename T, typename Compare>
void TwoFourMap<Key, T, Compare>::swap(TwoFourMap &other) noexcept {
  using std::swap;
  swap(root_, other.root_);
  swap(size_, other.size_);
  swap(comp_, other.comp_);
}

// ============================================================================
// Capacity
// ============================================================================

template <typename Key, typename T, typename Compare>
int TwoFourMap<Key, T, Compare>::height() const noexcept {
  int h = 0;
  for (const Node *x = root_; x; x = x->child[0])
    ++h;
  return h;
}

// ============================================================================
// Static navigation helpers
// ============================================================================

template <typename Key, typename T, typename Compare>
typename TwoFourMap<Key, T, Compare>::Pos
TwoFourMap<Key, T, Compare>::minPos(Node *n) noexcept {
  if (!n)
    return {};
  while (!n->isLeaf())
    n = n->child[0];
  return {n, 0};
}

template <typename Key, typename T, typename Compare>
typename TwoFourMap<Key, T, Compare>::Pos
TwoFourMap<Key, T, Compare>::maxPos(Node *n) noexcept {
  if (!n)
    return {};
  while (!n->isLeaf())
    n = n->child[n->n];
  return {n, n->n - 1};
}

template <typename Key, typename T, typename Compare>
int TwoFourMap<Key, T, Compare>::indexInParent(const Node *parent,
                                               const Node *child) noexcept {
  int i = 0;
  while (parent->child[i] != child)
    ++i;
  return i;
}

template <typename Key, typename T, typename Compare>
typename TwoFourMap<Key, T, Compare>::Pos
TwoFourMap<Key, T, Compare>::successorPos(Pos p) noexcept {
  Node *x = p.node;
  if (!x->isLeaf()) { // leftmost entry of the subtree right of this entry
    x = x->child[p.idx + 1];
    while (!x->isLeaf())
      x = x->child[0];
    return {x, 0};
  }
  if (p.idx + 1 < x->n)
    return {x, p.idx + 1};
  Node *c = x;
  Node *par = c->parent;
  while (par) {
    const int ci = indexInParent(par, c);
    if (ci < par->n)
      return {par, ci};
    c = par;
    par = par->parent;
  }
  return {}; // end()
}

template <typename Key, typename T, typename Compare>
typename TwoFourMap<Key, T, Compare>::Pos
TwoFourMap<Key, T, Compare>::predecessorPos(Pos p) noexcept {
  Node *x = p.node;
  if (!x->isLeaf()) { // rightmost entry of the subtree left of this entry
    x = x->child[p.idx];
    while (!x->isLeaf())
      x = x->child[x->n];
    return {x, x->n - 1};
  }
  if (p.idx > 0)
    return {x, p.idx - 1};
  Node *c = x;
  Node *par = c->parent;
  while (par) {
    const int ci = indexInParent(par, c);
    if (ci > 0)
      return {par, ci - 1};
    c = par;
    par = par->parent;
  }
  return {}; // before begin(): not dereferenceable
}

// ============================================================================
// Lookup helpers
// ============================================================================

template <typename Key, typename T, typename Compare>
int TwoFourMap<Key, T, Compare>::lowerIndex(const Node *x,
                                            const Key &key) const {
  int i = 0;
  while (i < x->n && comp_(x->key(i), key))
    ++i;
  return i;
}

template <typename Key, typename T, typename Compare>
int TwoFourMap<Key, T, Compare>::upperIndex(const Node *x,
                                            const Key &key) const {
  int i = 0;
  while (i < x->n && !comp_(key, x->key(i)))
    ++i;
  return i;
}

template <typename Key, typename T, typename Compare>
typename TwoFourMap<Key, T, Compare>::Pos
TwoFourMap<Key, T, Compare>::findPos(const Key &key) const {
  Node *x = root_;
  while (x) {
    const int i = lowerIndex(x, key);
    if (i < x->n && !comp_(key, x->key(i)))
      return {x, i};
    x = x->child[i]; // nullptr for leaves
  }
  return {};
}

template <typename Key, typename T, typename Compare>
typename TwoFourMap<Key, T, Compare>::Pos
TwoFourMap<Key, T, Compare>::ceilingPos(const Key &key) const {
  Pos best;
  Node *x = root_;
  while (x) {
    const int i = lowerIndex(x, key);
    if (i < x->n) {
      best = {x, i};
      if (!comp_(key, x->key(i)))
        return best; // exact match
    }
    x = x->child[i];
  }
  return best;
}

template <typename Key, typename T, typename Compare>
typename TwoFourMap<Key, T, Compare>::Pos
TwoFourMap<Key, T, Compare>::higherPos(const Key &key) const {
  Pos best;
  Node *x = root_;
  while (x) {
    const int i = upperIndex(x, key);
    if (i < x->n)
      best = {x, i};
    x = x->child[i];
  }
  return best;
}

template <typename Key, typename T, typename Compare>
typename TwoFourMap<Key, T, Compare>::Pos
TwoFourMap<Key, T, Compare>::floorPos(const Key &key) const {
  Pos best;
  Node *x = root_;
  while (x) {
    const int i = upperIndex(x, key);
    if (i > 0)
      best = {x, i - 1};
    x = x->child[i];
  }
  return best;
}

template <typename Key, typename T, typename Compare>
typename TwoFourMap<Key, T, Compare>::Pos
TwoFourMap<Key, T, Compare>::lowerPos(const Key &key) const {
  Pos best;
  Node *x = root_;
  while (x) {
    const int i = lowerIndex(x, key);
    if (i > 0)
      best = {x, i - 1};
    x = x->child[i];
  }
  return best;
}

// ============================================================================
// Map ADT operations
// ============================================================================

template <typename Key, typename T, typename Compare>
typename TwoFourMap<Key, T, Compare>::iterator
TwoFourMap<Key, T, Compare>::find(const Key &key) {
  return iterator(findPos(key), this);
}

template <typename Key, typename T, typename Compare>
typename TwoFourMap<Key, T, Compare>::const_iterator
TwoFourMap<Key, T, Compare>::find(const Key &key) const {
  return const_iterator(findPos(key), this);
}

template <typename Key, typename T, typename Compare>
bool TwoFourMap<Key, T, Compare>::contains(const Key &key) const {
  return findPos(key).node != nullptr;
}

template <typename Key, typename T, typename Compare>
T &TwoFourMap<Key, T, Compare>::at(const Key &key) {
  const Pos p = findPos(key);
  if (!p.node)
    throw std::out_of_range("TwoFourMap::at: key not found");
  return p.node->entry(p.idx).second;
}

template <typename Key, typename T, typename Compare>
const T &TwoFourMap<Key, T, Compare>::at(const Key &key) const {
  const Pos p = findPos(key);
  if (!p.node)
    throw std::out_of_range("TwoFourMap::at: key not found");
  return p.node->entry(p.idx).second;
}

template <typename Key, typename T, typename Compare>
T &TwoFourMap<Key, T, Compare>::operator[](const Key &key) {
  const Pos p = insertImpl(key).first;
  return p.node->entry(p.idx).second;
}

template <typename Key, typename T, typename Compare>
template <typename V>
std::pair<typename TwoFourMap<Key, T, Compare>::iterator, bool>
TwoFourMap<Key, T, Compare>::put(const Key &key, V &&value) {
  auto [pos, inserted] = insertImpl(key, std::forward<V>(value));
  if (!inserted)
    pos.node->entry(pos.idx).second = std::forward<V>(value); // args untouched
  return {iterator(pos, this), inserted};
}

template <typename Key, typename T, typename Compare>
template <typename V>
std::pair<typename TwoFourMap<Key, T, Compare>::iterator, bool>
TwoFourMap<Key, T, Compare>::insert(const Key &key, V &&value) {
  auto [pos, inserted] = insertImpl(key, std::forward<V>(value));
  return {iterator(pos, this), inserted};
}

template <typename Key, typename T, typename Compare>
void TwoFourMap<Key, T, Compare>::splitChild(Node *parent, int i) {
  // child (3 entries e0 e1 e2) -> child(e0) | e1 moves up | right(e2)
  Node *c = parent->child[i];
  Node *r = new Node(parent); // may throw: nothing has been modified yet

  r->pushBack(c->popBack()); // e2
  if (c->child[2]) {
    r->child[0] = c->child[2];
    r->child[1] = c->child[3];
    r->child[0]->parent = r;
    r->child[1]->parent = r;
    c->child[2] = c->child[3] = nullptr;
  }
  value_type mid = c->popBack(); // e1; c keeps e0

  for (int j = parent->n; j > i; --j)
    parent->child[j + 1] = parent->child[j];
  parent->child[i + 1] = r;
  parent->insertAt(i, std::move(mid));
}

template <typename Key, typename T, typename Compare>
template <typename... Args>
std::pair<typename TwoFourMap<Key, T, Compare>::Pos, bool>
TwoFourMap<Key, T, Compare>::insertImpl(const Key &key, Args &&...args) {
  if (const Pos found = findPos(key); found.node)
    return {found, false};

  // Build the entry up front: if this throws, the tree is untouched.
  value_type e(std::piecewise_construct, std::forward_as_tuple(key),
               std::forward_as_tuple(std::forward<Args>(args)...));

  if (!root_) {
    root_ = new Node();
    root_->pushBack(std::move(e));
    size_ = 1;
    return {Pos{root_, 0}, true};
  }

  if (root_->n == 3) { // grow the tree upward by splitting a full root
    Node *newRoot = new Node();
    newRoot->child[0] = root_;
    try {
      splitChild(newRoot, 0);
    } catch (...) {
      delete newRoot;
      throw;
    }
    root_->parent = newRoot;
    root_ = newRoot;
  }

  Node *x = root_;
  for (;;) {
    int i = 0;
    while (i < x->n && comp_(x->key(i), e.first))
      ++i; // key is absent: strict

    if (x->isLeaf()) {
      x->insertAt(i, std::move(e));
      ++size_;
      return {Pos{x, i}, true};
    }

    Node *c = x->child[i];
    if (c->n == 3) {
      splitChild(x, i);
      if (comp_(x->key(i), e.first))
        ++i; // median moved up: pick the side
      c = x->child[i];
    }
    x = c;
  }
}

template <typename Key, typename T, typename Compare>
bool TwoFourMap<Key, T, Compare>::erase(const Key &key) {
  const Pos p = findPos(key);
  if (!p.node)
    return false;
  eraseAt(p); // never touches `key` again, so aliasing a stored key is safe
  return true;
}

template <typename Key, typename T, typename Compare>
typename TwoFourMap<Key, T, Compare>::iterator
TwoFourMap<Key, T, Compare>::erase(const_iterator pos) {
  const Pos cur = pos.pos_;
  const Pos nxt = successorPos(cur);
  if (!nxt.node) {
    eraseAt(cur);
    return end();
  }
  Key nextKey = nxt.node->key(nxt.idx); // entries move: remember by key
  eraseAt(cur);
  return iterator(findPos(nextKey), this);
}

template <typename Key, typename T, typename Compare>
void TwoFourMap<Key, T, Compare>::eraseAt(Pos p) noexcept {
  Node *x = p.node;
  if (x->isLeaf()) {
    (void)x->removeAt(p.idx);
    fixUnderflow(x);
  } else {
    // Replace by the predecessor, which always sits in a leaf.
    Node *leaf = x->child[p.idx];
    while (!leaf->isLeaf())
      leaf = leaf->child[leaf->n];
    x->replaceAt(p.idx, leaf->popBack());
    fixUnderflow(leaf);
  }
  --size_;
}

template <typename Key, typename T, typename Compare>
void TwoFourMap<Key, T, Compare>::fixUnderflow(Node *x) noexcept {
  // Invariant: only `x` may be empty. An empty internal node has exactly one
  // child, in child[0]; an empty leaf has none.
  while (x->n == 0) {
    Node *const p = x->parent;

    if (!p) { // empty root: its only child (if any) becomes the root
      root_ = x->child[0];
      if (root_)
        root_->parent = nullptr;
      delete x;
      return;
    }

    const int ci = indexInParent(p, x);
    Node *const l = ci > 0 ? p->child[ci - 1] : nullptr;
    Node *const r = ci < p->n ? p->child[ci + 1] : nullptr;

    if (l && l->n >= 2) { // transfer from the left sibling
      Node *moved = l->child[l->n];
      l->child[l->n] = nullptr;
      value_type down = std::move(p->entry(ci - 1));
      p->replaceAt(ci - 1, l->popBack());
      x->child[1] = x->child[0];
      x->child[0] = moved;
      if (moved)
        moved->parent = x;
      x->insertAt(0, std::move(down));
      return;
    }

    if (r && r->n >= 2) { // transfer from the right sibling
      Node *moved = r->child[0];
      value_type down = std::move(p->entry(ci));
      p->replaceAt(ci, r->removeAt(0));
      x->insertAt(0, std::move(down));
      x->child[1] = moved;
      if (moved)
        moved->parent = x;
      for (int j = 0; j <= r->n; ++j)
        r->child[j] = r->child[j + 1];
      r->child[r->n + 1] = nullptr;
      return;
    }

    // Fusion: both neighbours are 2-nodes. The parent loses one entry and
    // may become empty, so the loop continues one level up.
    const int oldN = p->n;
    if (l) { // fuse x into the left sibling
      l->child[2] = x->child[0];
      if (l->child[2])
        l->child[2]->parent = l;
      l->pushBack(p->removeAt(ci - 1));
      for (int j = ci; j < oldN; ++j)
        p->child[j] = p->child[j + 1];
      p->child[oldN] = nullptr;
      x->child[0] = nullptr;
      delete x;
    } else { // fuse the right sibling into x
      x->pushBack(p->removeAt(ci));
      x->pushBack(r->removeAt(0));
      x->child[1] = r->child[0];
      x->child[2] = r->child[1];
      if (x->child[1]) {
        x->child[1]->parent = x;
        x->child[2]->parent = x;
      }
      for (int j = ci + 1; j < oldN; ++j)
        p->child[j] = p->child[j + 1];
      p->child[oldN] = nullptr;
      r->child[0] = r->child[1] = nullptr;
      delete r;
    }
    x = p;
  }
}

template <typename Key, typename T, typename Compare>
void TwoFourMap<Key, T, Compare>::clear() noexcept {
  destroySubtree(root_);
  root_ = nullptr;
  size_ = 0;
}

// ============================================================================
// Ordered map ADT operations
// ============================================================================

template <typename Key, typename T, typename Compare>
typename TwoFourMap<Key, T, Compare>::iterator
TwoFourMap<Key, T, Compare>::lastEntry() noexcept {
  return iterator(maxPos(root_), this);
}

template <typename Key, typename T, typename Compare>
typename TwoFourMap<Key, T, Compare>::const_iterator
TwoFourMap<Key, T, Compare>::lastEntry() const noexcept {
  return const_iterator(maxPos(root_), this);
}

template <typename Key, typename T, typename Compare>
typename TwoFourMap<Key, T, Compare>::iterator
TwoFourMap<Key, T, Compare>::ceilingEntry(const Key &key) {
  return iterator(ceilingPos(key), this);
}

template <typename Key, typename T, typename Compare>
typename TwoFourMap<Key, T, Compare>::const_iterator
TwoFourMap<Key, T, Compare>::ceilingEntry(const Key &key) const {
  return const_iterator(ceilingPos(key), this);
}

template <typename Key, typename T, typename Compare>
typename TwoFourMap<Key, T, Compare>::iterator
TwoFourMap<Key, T, Compare>::floorEntry(const Key &key) {
  return iterator(floorPos(key), this);
}

template <typename Key, typename T, typename Compare>
typename TwoFourMap<Key, T, Compare>::const_iterator
TwoFourMap<Key, T, Compare>::floorEntry(const Key &key) const {
  return const_iterator(floorPos(key), this);
}

template <typename Key, typename T, typename Compare>
typename TwoFourMap<Key, T, Compare>::iterator
TwoFourMap<Key, T, Compare>::higherEntry(const Key &key) {
  return iterator(higherPos(key), this);
}

template <typename Key, typename T, typename Compare>
typename TwoFourMap<Key, T, Compare>::const_iterator
TwoFourMap<Key, T, Compare>::higherEntry(const Key &key) const {
  return const_iterator(higherPos(key), this);
}

template <typename Key, typename T, typename Compare>
typename TwoFourMap<Key, T, Compare>::iterator
TwoFourMap<Key, T, Compare>::lowerEntry(const Key &key) {
  return iterator(lowerPos(key), this);
}

template <typename Key, typename T, typename Compare>
typename TwoFourMap<Key, T, Compare>::const_iterator
TwoFourMap<Key, T, Compare>::lowerEntry(const Key &key) const {
  return const_iterator(lowerPos(key), this);
}

template <typename Key, typename T, typename Compare>
std::pair<typename TwoFourMap<Key, T, Compare>::iterator,
          typename TwoFourMap<Key, T, Compare>::iterator>
TwoFourMap<Key, T, Compare>::subMap(const Key &k1, const Key &k2) {
  if (!comp_(k1, k2))
    return {end(), end()};
  return {ceilingEntry(k1), ceilingEntry(k2)};
}

template <typename Key, typename T, typename Compare>
std::pair<typename TwoFourMap<Key, T, Compare>::const_iterator,
          typename TwoFourMap<Key, T, Compare>::const_iterator>
TwoFourMap<Key, T, Compare>::subMap(const Key &k1, const Key &k2) const {
  if (!comp_(k1, k2))
    return {end(), end()};
  return {ceilingEntry(k1), ceilingEntry(k2)};
}

// ============================================================================
// Memory management helpers
// ============================================================================

template <typename Key, typename T, typename Compare>
typename TwoFourMap<Key, T, Compare>::Node *
TwoFourMap<Key, T, Compare>::cloneSubtree(const Node *src, Node *parent) {
  if (!src)
    return nullptr;
  Node *n = new Node(parent);
  try {
    for (int i = 0; i < src->n; ++i)
      n->pushBack(value_type(src->entry(i)));
    for (int i = 0; i <= src->n && src->child[i]; ++i) {
      n->child[i] = cloneSubtree(src->child[i], n);
    }
  } catch (...) {
    destroySubtree(n); // frees already attached children too
    throw;
  }
  return n;
}

template <typename Key, typename T, typename Compare>
void TwoFourMap<Key, T, Compare>::destroySubtree(Node *n) noexcept {
  // Recursion depth is bounded by the tree height, O(log n).
  if (!n)
    return;
  for (int i = 0; i < 4; ++i)
    destroySubtree(n->child[i]);
  delete n;
}

// ============================================================================
// Validation (testing / debugging)
// ============================================================================

template <typename Key, typename T, typename Compare>
bool TwoFourMap<Key, T, Compare>::validateNode(const Node *x,
                                               const Node *parent, int depth,
                                               int &leafDepth, size_type &count,
                                               const Key *lo,
                                               const Key *hi) const {
  if (x->parent != parent)
    return false;
  if (x->n < 1 || x->n > 3)
    return false;

  for (int i = 0; i < x->n; ++i) {
    if (lo && !comp_(*lo, x->key(i)))
      return false;
    if (hi && !comp_(x->key(i), *hi))
      return false;
    if (i > 0 && !comp_(x->key(i - 1), x->key(i)))
      return false;
  }
  count += static_cast<size_type>(x->n);

  if (x->isLeaf()) {
    for (int j = 0; j < 4; ++j) {
      if (x->child[j])
        return false;
    }
    if (leafDepth < 0)
      leafDepth = depth;
    return leafDepth == depth;
  }

  for (int j = 0; j <= x->n; ++j) {
    if (!x->child[j])
      return false;
    const Key *clo = j > 0 ? &x->key(j - 1) : lo;
    const Key *chi = j < x->n ? &x->key(j) : hi;
    if (!validateNode(x->child[j], x, depth + 1, leafDepth, count, clo, chi))
      return false;
  }
  for (int j = x->n + 1; j < 4; ++j) {
    if (x->child[j])
      return false;
  }
  return true;
}

template <typename Key, typename T, typename Compare>
bool TwoFourMap<Key, T, Compare>::isValid() const {
  if (!root_)
    return size_ == 0;
  int leafDepth = -1;
  size_type count = 0;
  if (!validateNode(root_, nullptr, 0, leafDepth, count, nullptr, nullptr))
    return false;
  return count == size_;
}

} // namespace ds

#endif // DS_TWO_FOUR_MAP_TPP