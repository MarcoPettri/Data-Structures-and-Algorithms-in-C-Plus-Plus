/**
 * @file ab_tree_map.tpp
 * @brief Template implementation for ds::ABTreeMap.
 */

#ifndef DS_AB_TREE_MAP_TPP
#define DS_AB_TREE_MAP_TPP

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace ds {

template <typename Key, typename T, typename Compare>
ABTreeMap<Key, T, Compare>::ABTreeMap(size_type a, size_type b,
                                      const Compare &compare)
    : a_(a), b_(b), compare_(compare) {
  if (a_ < 2 || a_ > std::numeric_limits<size_type>::max() / 2 + 1 ||
      b_ < 2 * a_ - 1)
    throw std::invalid_argument("(a,b)-tree requires a >= 2 and b >= 2a-1");
}

template <typename Key, typename T, typename Compare>
typename ABTreeMap<Key, T, Compare>::size_type
ABTreeMap<Key, T, Compare>::size() const noexcept {
  return size_;
}

template <typename Key, typename T, typename Compare>
bool ABTreeMap<Key, T, Compare>::empty() const noexcept {
  return size_ == 0;
}

template <typename Key, typename T, typename Compare>
typename ABTreeMap<Key, T, Compare>::size_type
ABTreeMap<Key, T, Compare>::min_children() const noexcept {
  return a_;
}

template <typename Key, typename T, typename Compare>
typename ABTreeMap<Key, T, Compare>::size_type
ABTreeMap<Key, T, Compare>::max_children() const noexcept {
  return b_;
}

template <typename Key, typename T, typename Compare>
bool ABTreeMap<Key, T, Compare>::Node::leaf() const noexcept {
  return children.empty();
}

template <typename Key, typename T, typename Compare>
bool ABTreeMap<Key, T, Compare>::equal(const Key &x, const Key &y) const {
  return !compare_(x, y) && !compare_(y, x);
}

template <typename Key, typename T, typename Compare>
typename ABTreeMap<Key, T, Compare>::size_type
ABTreeMap<Key, T, Compare>::lower_bound(const Node &node,
                                        const Key &key) const {
  return static_cast<size_type>(
      std::lower_bound(node.entries.begin(), node.entries.end(), key,
                       [this](const value_type &entry, const Key &k) {
                         return compare_(entry.first, k);
                       }) -
      node.entries.begin());
}

template <typename Key, typename T, typename Compare>
typename ABTreeMap<Key, T, Compare>::Node *
ABTreeMap<Key, T, Compare>::locate(const Key &key) {
  Node *node = root_.get();
  while (node) {
    const size_type i = lower_bound(*node, key);
    if (i < node->entries.size() && equal(node->entries[i].first, key))
      return node;
    node = node->leaf() ? nullptr : node->children[i].get();
  }
  return nullptr;
}

template <typename Key, typename T, typename Compare>
const typename ABTreeMap<Key, T, Compare>::Node *
ABTreeMap<Key, T, Compare>::locate(const Key &key) const {
  const Node *node = root_.get();
  while (node) {
    const size_type i = lower_bound(*node, key);
    if (i < node->entries.size() && equal(node->entries[i].first, key))
      return node;
    node = node->leaf() ? nullptr : node->children[i].get();
  }
  return nullptr;
}

template <typename Key, typename T, typename Compare>
std::optional<typename ABTreeMap<Key, T, Compare>::value_type>
ABTreeMap<Key, T, Compare>::find(const Key &key) const {
  const Node *node = root_.get();
  while (node) {
    const size_type i = lower_bound(*node, key);
    if (i < node->entries.size() && equal(node->entries[i].first, key))
      return node->entries[i];
    if (node->leaf())
      return std::nullopt;
    node = node->children[i].get();
  }
  return std::nullopt;
}

template <typename Key, typename T, typename Compare>
bool ABTreeMap<Key, T, Compare>::contains(const Key &key) const {
  return locate(key) != nullptr;
}

template <typename Key, typename T, typename Compare>
template <typename V>
std::pair<typename ABTreeMap<Key, T, Compare>::value_type, bool>
ABTreeMap<Key, T, Compare>::put(const Key &key, V &&value) {
  if (Node *existing = locate(key)) {
    const size_type i = lower_bound(*existing, key);
    existing->entries[i].second = std::forward<V>(value);
    return {existing->entries[i], false};
  }
  insert_new(key, T(std::forward<V>(value)));
  Node *inserted = locate(key);
  return {inserted->entries[lower_bound(*inserted, key)], true};
}

template <typename Key, typename T, typename Compare>
template <typename V>
std::pair<typename ABTreeMap<Key, T, Compare>::value_type, bool>
ABTreeMap<Key, T, Compare>::insert(const Key &key, V &&value) {
  if (const Node *existing = locate(key))
    return {existing->entries[lower_bound(*existing, key)], false};
  insert_new(key, T(std::forward<V>(value)));
  const Node *inserted = locate(key);
  return {inserted->entries[lower_bound(*inserted, key)], true};
}

template <typename Key, typename T, typename Compare>
T &ABTreeMap<Key, T, Compare>::operator[](const Key &key) {
  if (Node *node = locate(key))
    return node->entries[lower_bound(*node, key)].second;
  insert_new(key, T{});
  Node *node = locate(key);
  return node->entries[lower_bound(*node, key)].second;
}

template <typename Key, typename T, typename Compare>
T &ABTreeMap<Key, T, Compare>::at(const Key &key) {
  Node *node = locate(key);
  if (!node)
    throw std::out_of_range("ABTreeMap::at: key not found");
  return node->entries[lower_bound(*node, key)].second;
}

template <typename Key, typename T, typename Compare>
const T &ABTreeMap<Key, T, Compare>::at(const Key &key) const {
  const Node *node = locate(key);
  if (!node)
    throw std::out_of_range("ABTreeMap::at: key not found");
  return node->entries[lower_bound(*node, key)].second;
}

template <typename Key, typename T, typename Compare>
bool ABTreeMap<Key, T, Compare>::erase(const Key &key) {
  if (!locate(key))
    return false;
  erase_from(root_.get(), key);
  --size_;
  if (root_ && root_->entries.empty()) {
    if (root_->leaf())
      root_.reset();
    else
      root_ = std::move(root_->children.front());
  }
  return true;
}

template <typename Key, typename T, typename Compare>
void ABTreeMap<Key, T, Compare>::clear() noexcept {
  root_.reset();
  size_ = 0;
}

template <typename Key, typename T, typename Compare>
std::optional<typename ABTreeMap<Key, T, Compare>::value_type>
ABTreeMap<Key, T, Compare>::extreme(bool first) const {
  const Node *node = root_.get();
  if (!node)
    return std::nullopt;
  while (!node->leaf())
    node = node->children[first ? 0 : node->children.size() - 1].get();
  return node->entries[first ? 0 : node->entries.size() - 1];
}

template <typename Key, typename T, typename Compare>
std::optional<typename ABTreeMap<Key, T, Compare>::value_type>
ABTreeMap<Key, T, Compare>::firstEntry() const {
  return extreme(true);
}

template <typename Key, typename T, typename Compare>
std::optional<typename ABTreeMap<Key, T, Compare>::value_type>
ABTreeMap<Key, T, Compare>::lastEntry() const {
  return extreme(false);
}

template <typename Key, typename T, typename Compare>
std::optional<typename ABTreeMap<Key, T, Compare>::value_type>
ABTreeMap<Key, T, Compare>::bound_entry(const Key &key, bool predecessor,
                                        bool strict) const {
  const Node *node = root_.get();
  std::optional<value_type> best;
  while (node) {
    const size_type i = lower_bound(*node, key);
    const bool exact =
        i < node->entries.size() && equal(node->entries[i].first, key);
    for (const auto &entry : node->entries) {
      const bool qualifies = predecessor
                                 ? (compare_(entry.first, key) ||
                                    (!strict && equal(entry.first, key)))
                                 : (compare_(key, entry.first) ||
                                    (!strict && equal(entry.first, key)));
      if (qualifies &&
          (!best || (predecessor ? compare_(best->first, entry.first)
                                 : compare_(entry.first, best->first))))
        best = entry;
    }
    if (node->leaf() || (exact && !strict))
      break;
    const size_type child_index = exact && !predecessor ? i + 1 : i;
    node = node->children[child_index].get();
  }
  return best;
}

template <typename Key, typename T, typename Compare>
std::optional<typename ABTreeMap<Key, T, Compare>::value_type>
ABTreeMap<Key, T, Compare>::ceilingEntry(const Key &key) const {
  return bound_entry(key, false, false);
}

template <typename Key, typename T, typename Compare>
std::optional<typename ABTreeMap<Key, T, Compare>::value_type>
ABTreeMap<Key, T, Compare>::floorEntry(const Key &key) const {
  return bound_entry(key, true, false);
}

template <typename Key, typename T, typename Compare>
std::optional<typename ABTreeMap<Key, T, Compare>::value_type>
ABTreeMap<Key, T, Compare>::higherEntry(const Key &key) const {
  return bound_entry(key, false, true);
}

template <typename Key, typename T, typename Compare>
std::optional<typename ABTreeMap<Key, T, Compare>::value_type>
ABTreeMap<Key, T, Compare>::lowerEntry(const Key &key) const {
  return bound_entry(key, true, true);
}

template <typename Key, typename T, typename Compare>
std::vector<typename ABTreeMap<Key, T, Compare>::value_type>
ABTreeMap<Key, T, Compare>::subMap(const Key &low, const Key &high) const {
  std::vector<value_type> result;
  if (!compare_(low, high))
    return result;
  collect_range(root_.get(), low, high, result);
  return result;
}

template <typename Key, typename T, typename Compare>
std::vector<typename ABTreeMap<Key, T, Compare>::value_type>
ABTreeMap<Key, T, Compare>::to_vector() const {
  std::vector<value_type> result;
  result.reserve(size_);
  collect(root_.get(), result);
  return result;
}

template <typename Key, typename T, typename Compare>
bool ABTreeMap<Key, T, Compare>::validate() const {
  if (!root_)
    return size_ == 0;
  size_type count = 0;
  std::optional<size_type> leaf_depth;
  if (!validate_node(root_.get(), true, nullptr, nullptr, 0, leaf_depth, count))
    return false;
  return count == size_;
}

template <typename Key, typename T, typename Compare>
std::optional<typename ABTreeMap<Key, T, Compare>::Split>
ABTreeMap<Key, T, Compare>::insert_recursive(Node *node,
                                             const value_type &entry) {
  const size_type i = lower_bound(*node, entry.first);
  if (node->leaf()) {
    node->entries.insert(node->entries.begin() + static_cast<std::ptrdiff_t>(i),
                         entry);
  } else {
    auto split = insert_recursive(node->children[i].get(), entry);
    if (split) {
      node->entries.insert(node->entries.begin() +
                               static_cast<std::ptrdiff_t>(i),
                           std::move(split->promoted));
      node->children.insert(node->children.begin() +
                                static_cast<std::ptrdiff_t>(i + 1),
                            std::move(split->right));
    }
  }
  if (node->entries.size() < b_)
    return std::nullopt;

  // Promote the median so each half retains enough keys after overflow.
  const size_type middle = node->entries.size() / 2;
  auto right = std::make_unique<Node>();
  Split result{node->entries[middle], std::move(right)};
  result.right->entries.assign(
      std::make_move_iterator(node->entries.begin() +
                              static_cast<std::ptrdiff_t>(middle + 1)),
      std::make_move_iterator(node->entries.end()));
  node->entries.erase(node->entries.begin() +
                          static_cast<std::ptrdiff_t>(middle),
                      node->entries.end());
  if (!node->leaf()) {
    result.right->children.assign(
        std::make_move_iterator(node->children.begin() +
                                static_cast<std::ptrdiff_t>(middle + 1)),
        std::make_move_iterator(node->children.end()));
    node->children.erase(node->children.begin() +
                             static_cast<std::ptrdiff_t>(middle + 1),
                         node->children.end());
  }
  return result;
}

template <typename Key, typename T, typename Compare>
void ABTreeMap<Key, T, Compare>::insert_new(const Key &key, T value) {
  value_type entry(key, std::move(value));
  if (!root_) {
    root_ = std::make_unique<Node>();
    root_->entries.push_back(std::move(entry));
    ++size_;
    return;
  }
  auto split = insert_recursive(root_.get(), entry);
  if (split) {
    auto new_root = std::make_unique<Node>();
    new_root->entries.push_back(std::move(split->promoted));
    new_root->children.push_back(std::move(root_));
    new_root->children.push_back(std::move(split->right));
    root_ = std::move(new_root);
  }
  ++size_;
}

template <typename Key, typename T, typename Compare>
void ABTreeMap<Key, T, Compare>::erase_from(Node *node, const Key &key) {
  const size_type i = lower_bound(*node, key);
  if (i < node->entries.size() && equal(node->entries[i].first, key)) {
    if (node->leaf()) {
      node->entries.erase(node->entries.begin() +
                          static_cast<std::ptrdiff_t>(i));
      return;
    }
    Node *predecessor = node->children[i].get();
    while (!predecessor->leaf())
      predecessor = predecessor->children.back().get();
    node->entries[i] = predecessor->entries.back();
    const Key predecessor_key = predecessor->entries.back().first;
    erase_from(node->children[i].get(), predecessor_key);
    fix_underflow(node, i);
    return;
  }
  erase_from(node->children[i].get(), key);
  fix_underflow(node, i);
}

template <typename Key, typename T, typename Compare>
void ABTreeMap<Key, T, Compare>::fix_underflow(Node *parent, size_type i) {
  Node *child = parent->children[i].get();
  if (child->entries.size() >= a_ - 1)
    return;
  // Borrow through the parent separator when a sibling has surplus occupancy.
  if (i > 0 && parent->children[i - 1]->entries.size() >= a_) {
    Node *left = parent->children[i - 1].get();
    child->entries.insert(child->entries.begin(),
                          std::move(parent->entries[i - 1]));
    parent->entries[i - 1] = std::move(left->entries.back());
    left->entries.pop_back();
    if (!left->leaf()) {
      child->children.insert(child->children.begin(),
                             std::move(left->children.back()));
      left->children.pop_back();
    }
    return;
  }
  if (i + 1 < parent->children.size() &&
      parent->children[i + 1]->entries.size() >= a_) {
    Node *right = parent->children[i + 1].get();
    child->entries.push_back(std::move(parent->entries[i]));
    parent->entries[i] = std::move(right->entries.front());
    right->entries.erase(right->entries.begin());
    if (!right->leaf()) {
      child->children.push_back(std::move(right->children.front()));
      right->children.erase(right->children.begin());
    }
    return;
  }
  // If neither sibling can lend, merge with one and propagate parent underflow.
  if (i > 0) {
    Node *left = parent->children[i - 1].get();
    left->entries.push_back(std::move(parent->entries[i - 1]));
    for (auto &entry : child->entries)
      left->entries.push_back(std::move(entry));
    for (auto &descendant : child->children)
      left->children.push_back(std::move(descendant));
    parent->entries.erase(parent->entries.begin() +
                          static_cast<std::ptrdiff_t>(i - 1));
    parent->children.erase(parent->children.begin() +
                           static_cast<std::ptrdiff_t>(i));
  } else {
    Node *right = parent->children[i + 1].get();
    child->entries.push_back(std::move(parent->entries[i]));
    for (auto &entry : right->entries)
      child->entries.push_back(std::move(entry));
    for (auto &descendant : right->children)
      child->children.push_back(std::move(descendant));
    parent->entries.erase(parent->entries.begin() +
                          static_cast<std::ptrdiff_t>(i));
    parent->children.erase(parent->children.begin() +
                           static_cast<std::ptrdiff_t>(i + 1));
  }
}

template <typename Key, typename T, typename Compare>
void ABTreeMap<Key, T, Compare>::collect(const Node *node,
                                         std::vector<value_type> &out) const {
  if (!node)
    return;
  for (size_type i = 0; i < node->entries.size(); ++i) {
    if (!node->leaf())
      collect(node->children[i].get(), out);
    out.push_back(node->entries[i]);
  }
  if (!node->leaf())
    collect(node->children.back().get(), out);
}

template <typename Key, typename T, typename Compare>
void ABTreeMap<Key, T, Compare>::collect_range(
    const Node *node, const Key &low, const Key &high,
    std::vector<value_type> &out) const {
  if (!node)
    return;
  for (size_type i = 0; i < node->entries.size(); ++i) {
    if (!node->leaf())
      collect_range(node->children[i].get(), low, high, out);
    if (!compare_(node->entries[i].first, low) &&
        compare_(node->entries[i].first, high))
      out.push_back(node->entries[i]);
  }
  if (!node->leaf())
    collect_range(node->children.back().get(), low, high, out);
}

template <typename Key, typename T, typename Compare>
bool ABTreeMap<Key, T, Compare>::validate_node(
    const Node *node, bool is_root, const Key *low, const Key *high,
    size_type depth, std::optional<size_type> &leaf_depth,
    size_type &count) const {
  const size_type key_count = node->entries.size();
  if (key_count == 0 || key_count >= b_ || (!is_root && key_count < a_ - 1))
    return false;
  for (size_type i = 0; i < key_count; ++i) {
    const Key &key = node->entries[i].first;
    if ((i && !compare_(node->entries[i - 1].first, key)) ||
        (low && !compare_(*low, key)) || (high && !compare_(key, *high)))
      return false;
  }
  count += key_count;
  if (node->leaf()) {
    if (!leaf_depth)
      leaf_depth = depth;
    return *leaf_depth == depth;
  }
  if (node->children.size() != key_count + 1 || node->children.size() > b_ ||
      (is_root ? node->children.size() < 2 : node->children.size() < a_))
    return false;
  for (size_type i = 0; i < node->children.size(); ++i) {
    const Key *child_low = i == 0 ? low : &node->entries[i - 1].first;
    const Key *child_high = i == key_count ? high : &node->entries[i].first;
    if (!validate_node(node->children[i].get(), false, child_low, child_high,
                       depth + 1, leaf_depth, count))
      return false;
  }
  return true;
}

} // namespace ds

#endif // DS_AB_TREE_MAP_TPP
