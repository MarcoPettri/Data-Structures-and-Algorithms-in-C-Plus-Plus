/**
 * @file ab_tree.tpp
 * @brief Implementation of ds::ABTree. Included by ab_tree.hpp; do not
 *        compile or include directly.
 *
 * ### Insert (bottom-up split)
 * Descend to the target leaf recording the path, insert the key, then walk
 * back up splitting any node that now has B keys.  If the root itself splits
 * a new root is created and the height increases by 1.
 *
 * ### Erase (bottom-up merge / transfer)
 * Locate the key (swapping with the in-order successor when found at an
 * internal node), delete from the leaf, then walk back up repairing any node
 * that underflows by rotating from a sibling (transfer) or merging with it.
 * If the root is left with 0 keys the sole remaining child becomes the new
 * root and the height decreases by 1.
 */

#ifndef DS_AB_TREE_TPP
#define DS_AB_TREE_TPP

#include <algorithm>
#include <cassert>
#include <utility>

namespace ds {

// ============================================================================
// Construction / assignment / destruction
// ============================================================================

template <typename Key, std::size_t A, std::size_t B>
ABTree<Key, A, B>::ABTree()
    : root_(std::make_unique<node_type>()), size_(0), height_(0) {}

template <typename Key, std::size_t A, std::size_t B>
ABTree<Key, A, B>::ABTree(const ABTree &other)
    : root_(clone(other.root_)), size_(other.size_), height_(other.height_) {}

template <typename Key, std::size_t A, std::size_t B>
ABTree<Key, A, B> &ABTree<Key, A, B>::operator=(ABTree other) noexcept {
  swap(other);
  return *this;
}

template <typename Key, std::size_t A, std::size_t B>
void ABTree<Key, A, B>::swap(ABTree &other) noexcept {
  using std::swap;
  swap(root_, other.root_);
  swap(size_, other.size_);
  swap(height_, other.height_);
}

// ============================================================================
// contains
// ============================================================================

template <typename Key, std::size_t A, std::size_t B>
bool ABTree<Key, A, B>::contains(const Key &key) const noexcept {
  return search(root_.get(), key);
}

// ============================================================================
// insert
// ============================================================================

template <typename Key, std::size_t A, std::size_t B>
bool ABTree<Key, A, B>::insert(const Key &key) {
  // Path entry: node pointer + child-index used to descend.
  struct Frame {
    node_type *node;
    std::size_t child_idx;
  };

  std::vector<Frame> path;
  path.reserve(height_ + 1);

  // ---- Phase 1: descend to the correct leaf --------------------------------
  node_type *cur = root_.get();
  while (!cur->is_leaf()) {
    std::size_t idx = lower_bound_idx(cur->keys, key);
    // Exact match at an internal node → duplicate.
    if (idx < cur->keys.size() && !(key < cur->keys[idx]))
      return false;
    path.push_back({cur, idx});
    cur = cur->children[idx].get();
  }

  // ---- Check duplicate in leaf --------------------------------------------
  std::size_t leaf_pos = lower_bound_idx(cur->keys, key);
  if (leaf_pos < cur->keys.size() && !(key < cur->keys[leaf_pos]))
    return false;

  // ---- Phase 2: insert key into leaf --------------------------------------
  cur->keys.insert(cur->keys.begin() + static_cast<std::ptrdiff_t>(leaf_pos),
                   key);
  ++size_;

  // ---- Phase 3: propagate splits upward -----------------------------------
  // carry (promoted_key, overflow_right) upward after each split.
  node_ptr overflow_right{nullptr};
  Key promoted_key{};
  bool overflowed =
      do_split(cur, /*is_leaf=*/true, promoted_key, overflow_right);

  for (auto it = path.rbegin(); overflowed && it != path.rend(); ++it) {
    node_type *parent = it->node;
    std::size_t ci = it->child_idx;

    parent->keys.insert(parent->keys.begin() + static_cast<std::ptrdiff_t>(ci),
                        std::move(promoted_key));
    parent->children.insert(parent->children.begin() +
                                static_cast<std::ptrdiff_t>(ci + 1),
                            std::move(overflow_right));

    overflowed =
        do_split(parent, /*is_leaf=*/false, promoted_key, overflow_right);
  }

  // ---- Phase 4: root split (height grows) ---------------------------------
  if (overflowed) {
    auto new_root = std::make_unique<node_type>();
    new_root->keys.push_back(std::move(promoted_key));
    new_root->children.push_back(std::move(root_));
    new_root->children.push_back(std::move(overflow_right));
    root_ = std::move(new_root);
    ++height_;
  }

  return true;
}

// ============================================================================
// erase
// ============================================================================

template <typename Key, std::size_t A, std::size_t B>
bool ABTree<Key, A, B>::erase(const Key &key) {
  struct Frame {
    node_type *node;
    std::size_t child_idx; ///< index in node->children we descended into
  };

  std::vector<Frame> path;
  path.reserve(height_ + 1);

  // ---- Phase 1: descend; track where the key lives ------------------------
  node_type *cur = root_.get();
  bool key_in_internal = false;
  std::size_t internal_key_idx = 0;
  node_type *internal_node = nullptr;

  while (!cur->is_leaf()) {
    std::size_t idx = lower_bound_idx(cur->keys, key);
    if (idx < cur->keys.size() && !(key < cur->keys[idx])) {
      // Found at this internal node: record it and descend to the in-order
      // successor (leftmost key of subtree children[idx+1]).
      key_in_internal = true;
      internal_node = cur;
      internal_key_idx = idx;
      path.push_back({cur, idx + 1});
      cur = cur->children[idx + 1].get();
    } else {
      path.push_back({cur, idx});
      cur = cur->children[idx].get();
    }
  }

  // ---- Confirm presence ---------------------------------------------------
  std::size_t leaf_pos = lower_bound_idx(cur->keys, key);
  bool found_in_leaf =
      (leaf_pos < cur->keys.size()) && !(key < cur->keys[leaf_pos]);

  if (!found_in_leaf && !key_in_internal)
    return false; // key not in tree

  // ---- Phase 2: delete from leaf (possibly via successor swap) ------------
  if (key_in_internal) {
    // Replace the internal key with the leaf's leftmost key (in-order
    // successor), then remove that leftmost key from the leaf.
    assert(!cur->keys.empty());
    internal_node->keys[internal_key_idx] = cur->keys[0];
    cur->keys.erase(cur->keys.begin());
  } else {
    cur->keys.erase(cur->keys.begin() + static_cast<std::ptrdiff_t>(leaf_pos));
  }
  --size_;

  // ---- Phase 3: propagate underflows upward -------------------------------
  node_type *child = cur; // start at the modified leaf

  for (auto it = path.rbegin(); it != path.rend(); ++it) {
    // Underflow condition:
    //   leaf:     keys.size() < A-1 (non-root leaf must have >= A-1 keys)
    //   internal: children.size() < A
    bool underflow = child->is_leaf()
                         ? (!path.empty() && child->keys.size() < A - 1)
                         : child->children.size() < A;
    if (!underflow)
      break;

    node_type *parent = it->node;
    std::size_t ci = it->child_idx; // parent->children[ci] == child

    // ---- Try transfer from left sibling -----------------------------------
    if (ci > 0) {
      node_type *left_sib = parent->children[ci - 1].get();
      bool can_donate = child->is_leaf() ? left_sib->keys.size() >= A
                                         : left_sib->children.size() > A;
      if (can_donate) {
        transfer_from_left(parent, ci, child, left_sib);
        break;
      }
    }

    // ---- Try transfer from right sibling ----------------------------------
    if (ci + 1 < parent->children.size()) {
      node_type *right_sib = parent->children[ci + 1].get();
      bool can_donate = child->is_leaf() ? right_sib->keys.size() >= A
                                         : right_sib->children.size() > A;
      if (can_donate) {
        transfer_from_right(parent, ci, child, right_sib);
        break;
      }
    }

    // ---- Merge (prefer left sibling when available) ----------------------
    if (ci > 0) {
      // Merge: left_sib + separator + child → left_sib.
      node_type *left_sib = parent->children[ci - 1].get();
      merge_into_left(parent, ci - 1, left_sib, child);
    } else {
      // Merge: child + separator + right_sib → child.
      node_type *right_sib = parent->children[ci + 1].get();
      merge_into_left(parent, ci, child, right_sib);
    }
    child = parent; // parent may now underflow
  }

  // ---- Phase 4: shrink root if it became empty ----------------------------
  while (root_->keys.empty() && !root_->children.empty()) {
    root_ = std::move(root_->children[0]);
    if (height_ > 0)
      --height_;
  }

  return true;
}

// ============================================================================
// Utilities
// ============================================================================

template <typename Key, std::size_t A, std::size_t B>
std::vector<Key> ABTree<Key, A, B>::to_sorted_vector() const {
  std::vector<Key> result;
  result.reserve(size_);
  collect(root_.get(), result);
  return result;
}

template <typename Key, std::size_t A, std::size_t B>
bool ABTree<Key, A, B>::validate() const {
  if (!root_)
    return false;
  return validate_node(root_.get(), /*is_root=*/true, std::optional<Key>{},
                       std::optional<Key>{}, 0, height_);
}

// ============================================================================
// Private helpers
// ============================================================================

// ---- Binary search: leftmost i where keys[i] >= key ----------------------
template <typename Key, std::size_t A, std::size_t B>
std::size_t ABTree<Key, A, B>::lower_bound_idx(const std::vector<Key> &keys,
                                               const Key &key) {
  std::size_t lo = 0, hi = keys.size();
  while (lo < hi) {
    std::size_t mid = lo + (hi - lo) / 2;
    if (keys[mid] < key)
      lo = mid + 1;
    else
      hi = mid;
  }
  return lo;
}

// ---- Recursive search ------------------------------------------------------
template <typename Key, std::size_t A, std::size_t B>
bool ABTree<Key, A, B>::search(const node_type *node, const Key &key) noexcept {
  std::size_t idx = lower_bound_idx(node->keys, key);
  if (idx < node->keys.size() && !(key < node->keys[idx]))
    return true; // found
  if (node->is_leaf())
    return false;
  return search(node->children[idx].get(), key);
}

// ---- Deep copy -------------------------------------------------------------
template <typename Key, std::size_t A, std::size_t B>
typename ABTree<Key, A, B>::node_ptr
ABTree<Key, A, B>::clone(const node_ptr &src) {
  if (!src)
    return nullptr;
  auto copy = std::make_unique<node_type>();
  copy->keys = src->keys;
  copy->children.reserve(src->children.size());
  for (const auto &ch : src->children)
    copy->children.push_back(clone(ch));
  return copy;
}

// ---- Split -----------------------------------------------------------------
// Returns true if @p node overflowed and was split.
// On true: @p promoted receives the median key; @p right receives the new
// right sibling (caller must insert them into the parent).
template <typename Key, std::size_t A, std::size_t B>
bool ABTree<Key, A, B>::do_split(node_type *node, bool is_leaf, Key &promoted,
                                 node_ptr &right) {
  // Overflow condition:
  //   leaf:     keys.size() == B  (max is B-1)
  //   internal: children.size() == B+1  (max is B)
  if (is_leaf && node->keys.size() <= B - 1)
    return false;
  if (!is_leaf && node->children.size() <= B)
    return false;

  std::size_t total_keys = node->keys.size(); // == B in both overflow cases
  std::size_t m = total_keys / 2; // median index

  right = std::make_unique<node_type>();
  promoted = node->keys[m];

  // Keys: left keeps [0..m-1], right gets [m+1..B-1].
  right->keys.assign(
      std::make_move_iterator(node->keys.begin() +
                              static_cast<std::ptrdiff_t>(m + 1)),
      std::make_move_iterator(node->keys.end()));
  node->keys.erase(node->keys.begin() + static_cast<std::ptrdiff_t>(m),
                   node->keys.end());

  if (!is_leaf) {
    // Children: left keeps [0..m], right gets [m+1..B].
    right->children.assign(
        std::make_move_iterator(node->children.begin() +
                                static_cast<std::ptrdiff_t>(m + 1)),
        std::make_move_iterator(node->children.end()));
    node->children.erase(node->children.begin() +
                             static_cast<std::ptrdiff_t>(m + 1),
                         node->children.end());
  }

  return true;
}

// ---- Transfer from left sibling --------------------------------------------
// Rotate: left_sib's rightmost key → parent separator → child's front.
// parent->children[ci-1] == left_sib,  parent->children[ci] == child.
template <typename Key, std::size_t A, std::size_t B>
void ABTree<Key, A, B>::transfer_from_left(node_type *parent, std::size_t ci,
                                           node_type *child,
                                           node_type *left_sib) {
  child->keys.insert(child->keys.begin(), parent->keys[ci - 1]);
  parent->keys[ci - 1] = std::move(left_sib->keys.back());
  left_sib->keys.pop_back();

  if (!child->is_leaf()) {
    child->children.insert(child->children.begin(),
                           std::move(left_sib->children.back()));
    left_sib->children.pop_back();
  }
}

// ---- Transfer from right sibling -------------------------------------------
// parent->children[ci] == child,  parent->children[ci+1] == right_sib.
template <typename Key, std::size_t A, std::size_t B>
void ABTree<Key, A, B>::transfer_from_right(node_type *parent, std::size_t ci,
                                            node_type *child,
                                            node_type *right_sib) {
  child->keys.push_back(parent->keys[ci]);
  parent->keys[ci] = std::move(right_sib->keys.front());
  right_sib->keys.erase(right_sib->keys.begin());

  if (!child->is_leaf()) {
    child->children.push_back(std::move(right_sib->children.front()));
    right_sib->children.erase(right_sib->children.begin());
  }
}

// ---- Merge -----------------------------------------------------------------
// Merge: left + parent->keys[sep_idx] + right → left.
// After: parent loses keys[sep_idx] and children[sep_idx+1]; right destroyed.
template <typename Key, std::size_t A, std::size_t B>
void ABTree<Key, A, B>::merge_into_left(node_type *parent, std::size_t sep_idx,
                                        node_type *left, node_type *right) {
  left->keys.push_back(std::move(parent->keys[sep_idx]));
  for (auto &k : right->keys)
    left->keys.push_back(std::move(k));
  for (auto &ch : right->children)
    left->children.push_back(std::move(ch));

  parent->keys.erase(parent->keys.begin() +
                     static_cast<std::ptrdiff_t>(sep_idx));
  // Erase children[sep_idx+1]: the unique_ptr destructor frees right's memory.
  parent->children.erase(parent->children.begin() +
                         static_cast<std::ptrdiff_t>(sep_idx + 1));
}

// ---- In-order key collection -----------------------------------------------
template <typename Key, std::size_t A, std::size_t B>
void ABTree<Key, A, B>::collect(const node_type *node, std::vector<Key> &out) {
  if (node->is_leaf()) {
    out.insert(out.end(), node->keys.begin(), node->keys.end());
    return;
  }
  for (std::size_t i = 0; i < node->children.size(); ++i) {
    collect(node->children[i].get(), out);
    if (i < node->keys.size())
      out.push_back(node->keys[i]);
  }
}

// ---- Structural validation -------------------------------------------------
template <typename Key, std::size_t A, std::size_t B>
bool ABTree<Key, A, B>::validate_node(const node_type *node, bool is_root,
                                      std::optional<Key> lo,
                                      std::optional<Key> hi, std::size_t depth,
                                      std::size_t expected_height) const {
  // Degree bounds.
  if (!is_root) {
    if (node->is_leaf()) {
      if (size_ > 0 && node->keys.size() < A - 1)
        return false;
      if (node->keys.size() > B - 1)
        return false;
    } else {
      if (node->degree() < A || node->degree() > B)
        return false;
    }
  } else {
    if (!node->is_leaf() && (node->degree() < 2 || node->degree() > B))
      return false;
  }

  // Key count == children - 1 for internal nodes.
  if (!node->is_leaf() && node->keys.size() + 1 != node->degree())
    return false;

  // Keys are sorted and within (lo, hi).
  for (std::size_t i = 0; i < node->keys.size(); ++i) {
    if (lo.has_value() && !(lo.value() < node->keys[i]))
      return false;
    if (hi.has_value() && !(node->keys[i] < hi.value()))
      return false;
    if (i > 0 && !(node->keys[i - 1] < node->keys[i]))
      return false;
  }

  // Uniform leaf depth.
  if (node->is_leaf())
    return depth == expected_height;

  // Recurse.
  for (std::size_t i = 0; i < node->children.size(); ++i) {
    std::optional<Key> child_lo =
        (i == 0) ? lo : std::optional<Key>{node->keys[i - 1]};
    std::optional<Key> child_hi =
        (i == node->keys.size()) ? hi : std::optional<Key>{node->keys[i]};
    if (!validate_node(node->children[i].get(), false, child_lo, child_hi,
                       depth + 1, expected_height))
      return false;
  }
  return true;
}

} // namespace ds

#endif // DS_AB_TREE_TPP
