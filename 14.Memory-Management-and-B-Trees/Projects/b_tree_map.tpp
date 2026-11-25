/**
 * @file b_tree_map.tpp
 * @brief Implementation of @ref ds::BTreeMap.
 */

#ifndef DS_B_TREE_MAP_TPP
#define DS_B_TREE_MAP_TPP

namespace ds {

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
BTreeMap<Key, T, BlockSize, Compare>::BTreeMap(const Compare &compare)
    : comp_(compare) {}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
void BTreeMap<Key, T, BlockSize, Compare>::clear() noexcept {
  disk_.clear();
  root_ = kInvalidBlock;
  size_ = 0;
  height_ = 0;
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
typename BTreeMap<Key, T, BlockSize, Compare>::Node
BTreeMap<Key, T, BlockSize, Compare>::deserialize(
    const typename BlockDevice<BlockSize>::block_type &buf) {
  Node node;
  std::size_t offset = 0;

  std::uint32_t count = 0;
  std::uint32_t leaf_flag = 1;
  std::memcpy(&count, buf.data() + offset, sizeof(count));
  offset += sizeof(count);
  std::memcpy(&leaf_flag, buf.data() + offset, sizeof(leaf_flag));
  offset += sizeof(leaf_flag);

  node.key_count = count;
  node.is_leaf = (leaf_flag != 0);

  constexpr std::size_t keys_bytes = sizeof(Key) * kMaxKeys;
  constexpr std::size_t vals_bytes = sizeof(T) * kMaxKeys;
  constexpr std::size_t children_bytes = sizeof(block_id_type) * kOrder;

  std::memcpy(node.keys.data(), buf.data() + offset, keys_bytes);
  offset += keys_bytes;
  std::memcpy(node.values.data(), buf.data() + offset, vals_bytes);
  offset += vals_bytes;
  std::memcpy(node.children.data(), buf.data() + offset, children_bytes);

  return node;
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
typename BlockDevice<BlockSize>::block_type
BTreeMap<Key, T, BlockSize, Compare>::serialize(const Node &node) {
  typename BlockDevice<BlockSize>::block_type buf{};
  std::size_t offset = 0;

  const std::uint32_t count = node.key_count;
  const std::uint32_t leaf_flag = node.is_leaf ? 1u : 0u;
  std::memcpy(buf.data() + offset, &count, sizeof(count));
  offset += sizeof(count);
  std::memcpy(buf.data() + offset, &leaf_flag, sizeof(leaf_flag));
  offset += sizeof(leaf_flag);

  constexpr std::size_t keys_bytes = sizeof(Key) * kMaxKeys;
  constexpr std::size_t vals_bytes = sizeof(T) * kMaxKeys;
  constexpr std::size_t children_bytes = sizeof(block_id_type) * kOrder;

  std::memcpy(buf.data() + offset, node.keys.data(), keys_bytes);
  offset += keys_bytes;
  std::memcpy(buf.data() + offset, node.values.data(), vals_bytes);
  offset += vals_bytes;
  std::memcpy(buf.data() + offset, node.children.data(), children_bytes);

  return buf;
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
typename BTreeMap<Key, T, BlockSize, Compare>::Node
BTreeMap<Key, T, BlockSize, Compare>::read_node(block_id_type id) const {
  return deserialize(disk_.read_block(id));
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
typename BTreeMap<Key, T, BlockSize, Compare>::Node
BTreeMap<Key, T, BlockSize, Compare>::peek_node(block_id_type id) const {
  return deserialize(disk_.peek_block(id));
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
void BTreeMap<Key, T, BlockSize, Compare>::write_node(block_id_type id,
                                                      const Node &node) {
  disk_.write_block(id, serialize(node));
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
typename BTreeMap<Key, T, BlockSize, Compare>::size_type
BTreeMap<Key, T, BlockSize, Compare>::lower_bound_index(const Node &node,
                                                        const Key &key) const {
  size_type lo = 0;
  size_type hi = node.key_count;
  while (lo < hi) {
    const size_type mid = lo + (hi - lo) / 2;
    if (comp_(node.keys[mid], key)) {
      lo = mid + 1;
    } else {
      hi = mid;
    }
  }
  return lo;
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
std::optional<typename BTreeMap<Key, T, BlockSize, Compare>::value_type>
BTreeMap<Key, T, BlockSize, Compare>::find(const Key &key) const {
  block_id_type curr = root_;
  while (curr != kInvalidBlock) {
    const Node node = read_node(curr);
    const size_type idx = lower_bound_index(node, key);
    if (idx < node.key_count && !comp_(key, node.keys[idx])) {
      return value_type{node.keys[idx], node.values[idx]};
    }
    if (node.is_leaf) {
      return std::nullopt;
    }
    curr = node.children[idx];
  }
  return std::nullopt;
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
bool BTreeMap<Key, T, BlockSize, Compare>::contains(const Key &key) const {
  return find(key).has_value();
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
T BTreeMap<Key, T, BlockSize, Compare>::at(const Key &key) const {
  const auto res = find(key);
  if (!res) {
    throw std::out_of_range("BTreeMap::at: key not found");
  }
  return res->second;
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
void BTreeMap<Key, T, BlockSize, Compare>::split_child(block_id_type parent_id,
                                                       Node &parent,
                                                       size_type child_idx) {
  const block_id_type full_id = parent.children[child_idx];
  Node left = read_node(full_id);

  // Midpoint index promotes left.keys[mid] into parent.
  constexpr size_type mid = kMaxKeys / 2;
  const block_id_type right_id = disk_.allocate_block();
  Node right;
  right.is_leaf = left.is_leaf;
  right.key_count = static_cast<std::uint32_t>(kMaxKeys - mid - 1);

  for (size_type j = 0; j < right.key_count; ++j) {
    right.keys[j] = left.keys[mid + 1 + j];
    right.values[j] = left.values[mid + 1 + j];
  }
  if (!left.is_leaf) {
    for (size_type j = 0; j <= right.key_count; ++j) {
      right.children[j] = left.children[mid + 1 + j];
    }
  }
  left.key_count = static_cast<std::uint32_t>(mid);

  // Shift parent's children and keys right to make room for promoted entry.
  for (size_type j = parent.key_count; j > child_idx; --j) {
    parent.children[j + 1] = parent.children[j];
  }
  parent.children[child_idx + 1] = right_id;

  for (size_type j = parent.key_count; j > child_idx; --j) {
    parent.keys[j] = parent.keys[j - 1];
    parent.values[j] = parent.values[j - 1];
  }
  parent.keys[child_idx] = left.keys[mid];
  parent.values[child_idx] = left.values[mid];
  ++parent.key_count;

  write_node(full_id, left);
  write_node(right_id, right);
  write_node(parent_id, parent);
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
std::pair<typename BTreeMap<Key, T, BlockSize, Compare>::value_type, bool>
BTreeMap<Key, T, BlockSize, Compare>::insert_nonfull(block_id_type node_id,
                                                     const Key &key,
                                                     const T &value,
                                                     bool overwrite) {
  block_id_type curr_id = node_id;
  Node curr = read_node(curr_id);

  while (true) {
    size_type idx = lower_bound_index(curr, key);
    if (idx < curr.key_count && !comp_(key, curr.keys[idx])) {
      if (overwrite) {
        curr.values[idx] = value;
        write_node(curr_id, curr);
      }
      return {value_type{curr.keys[idx], curr.values[idx]}, false};
    }

    if (curr.is_leaf) {
      for (size_type j = curr.key_count; j > idx; --j) {
        curr.keys[j] = curr.keys[j - 1];
        curr.values[j] = curr.values[j - 1];
      }
      curr.keys[idx] = key;
      curr.values[idx] = value;
      ++curr.key_count;
      ++size_;
      write_node(curr_id, curr);
      return {value_type{key, value}, true};
    }

    // Proactively split full child before descending.
    const block_id_type child_id = curr.children[idx];
    const Node child = read_node(child_id);
    if (child.key_count == kMaxKeys) {
      split_child(curr_id, curr, idx);
      // Compare against the newly promoted median key at curr.keys[idx].
      if (!comp_(key, curr.keys[idx]) && !comp_(curr.keys[idx], key)) {
        if (overwrite) {
          curr.values[idx] = value;
          write_node(curr_id, curr);
        }
        return {value_type{curr.keys[idx], curr.values[idx]}, false};
      }
      if (comp_(curr.keys[idx], key)) {
        ++idx;
      }
    }

    curr_id = curr.children[idx];
    curr = read_node(curr_id);
  }
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
std::pair<typename BTreeMap<Key, T, BlockSize, Compare>::value_type, bool>
BTreeMap<Key, T, BlockSize, Compare>::insert_impl(const Key &key,
                                                  const T &value,
                                                  bool overwrite) {
  if (root_ == kInvalidBlock) {
    root_ = disk_.allocate_block();
    Node root_node;
    root_node.is_leaf = true;
    root_node.key_count = 1;
    root_node.keys[0] = key;
    root_node.values[0] = value;
    write_node(root_, root_node);
    size_ = 1;
    height_ = 1;
    return {value_type{key, value}, true};
  }

  const Node root_node = read_node(root_);
  if (root_node.key_count == kMaxKeys) {
    const block_id_type new_root_id = disk_.allocate_block();
    Node new_root;
    new_root.is_leaf = false;
    new_root.key_count = 0;
    new_root.children[0] = root_;
    root_ = new_root_id;
    ++height_;
    split_child(new_root_id, new_root, 0);
  }

  return insert_nonfull(root_, key, value, overwrite);
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
std::pair<typename BTreeMap<Key, T, BlockSize, Compare>::value_type, bool>
BTreeMap<Key, T, BlockSize, Compare>::put(const Key &key, const T &value) {
  return insert_impl(key, value, /*overwrite=*/true);
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
std::pair<typename BTreeMap<Key, T, BlockSize, Compare>::value_type, bool>
BTreeMap<Key, T, BlockSize, Compare>::insert(const Key &key, const T &value) {
  return insert_impl(key, value, /*overwrite=*/false);
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
typename BTreeMap<Key, T, BlockSize, Compare>::value_type
BTreeMap<Key, T, BlockSize, Compare>::rightmost_entry(
    block_id_type node_id) const {
  block_id_type curr = node_id;
  while (true) {
    const Node node = read_node(curr);
    if (node.is_leaf) {
      return value_type{node.keys[node.key_count - 1],
                        node.values[node.key_count - 1]};
    }
    curr = node.children[node.key_count];
  }
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
typename BTreeMap<Key, T, BlockSize, Compare>::value_type
BTreeMap<Key, T, BlockSize, Compare>::leftmost_entry(
    block_id_type node_id) const {
  block_id_type curr = node_id;
  while (true) {
    const Node node = read_node(curr);
    if (node.is_leaf) {
      return value_type{node.keys[0], node.values[0]};
    }
    curr = node.children[0];
  }
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
void BTreeMap<Key, T, BlockSize, Compare>::borrow_from_prev(
    block_id_type parent_id, Node &parent, size_type child_idx) {
  const block_id_type child_id = parent.children[child_idx];
  const block_id_type sibling_id = parent.children[child_idx - 1];
  Node child = read_node(child_id);
  Node sibling = read_node(sibling_id);

  for (size_type i = child.key_count; i > 0; --i) {
    child.keys[i] = child.keys[i - 1];
    child.values[i] = child.values[i - 1];
  }
  if (!child.is_leaf) {
    for (size_type i = child.key_count + 1; i > 0; --i) {
      child.children[i] = child.children[i - 1];
    }
    child.children[0] = sibling.children[sibling.key_count];
  }

  child.keys[0] = parent.keys[child_idx - 1];
  child.values[0] = parent.values[child_idx - 1];
  ++child.key_count;

  parent.keys[child_idx - 1] = sibling.keys[sibling.key_count - 1];
  parent.values[child_idx - 1] = sibling.values[sibling.key_count - 1];
  --sibling.key_count;

  write_node(child_id, child);
  write_node(sibling_id, sibling);
  write_node(parent_id, parent);
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
void BTreeMap<Key, T, BlockSize, Compare>::borrow_from_next(
    block_id_type parent_id, Node &parent, size_type child_idx) {
  const block_id_type child_id = parent.children[child_idx];
  const block_id_type sibling_id = parent.children[child_idx + 1];
  Node child = read_node(child_id);
  Node sibling = read_node(sibling_id);

  child.keys[child.key_count] = parent.keys[child_idx];
  child.values[child.key_count] = parent.values[child_idx];
  if (!child.is_leaf) {
    child.children[child.key_count + 1] = sibling.children[0];
  }
  ++child.key_count;

  parent.keys[child_idx] = sibling.keys[0];
  parent.values[child_idx] = sibling.values[0];

  for (size_type i = 1; i < sibling.key_count; ++i) {
    sibling.keys[i - 1] = sibling.keys[i];
    sibling.values[i - 1] = sibling.values[i];
  }
  if (!sibling.is_leaf) {
    for (size_type i = 1; i <= sibling.key_count; ++i) {
      sibling.children[i - 1] = sibling.children[i];
    }
  }
  --sibling.key_count;

  write_node(child_id, child);
  write_node(sibling_id, sibling);
  write_node(parent_id, parent);
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
void BTreeMap<Key, T, BlockSize, Compare>::merge_children(
    block_id_type parent_id, Node &parent, size_type left_idx) {
  const block_id_type left_id = parent.children[left_idx];
  const block_id_type right_id = parent.children[left_idx + 1];
  Node left = read_node(left_id);
  const Node right = read_node(right_id);

  // Pull separator key from parent into left, then append right.
  const size_type base = left.key_count;
  left.keys[base] = parent.keys[left_idx];
  left.values[base] = parent.values[left_idx];

  for (size_type i = 0; i < right.key_count; ++i) {
    left.keys[base + 1 + i] = right.keys[i];
    left.values[base + 1 + i] = right.values[i];
  }
  if (!left.is_leaf) {
    for (size_type i = 0; i <= right.key_count; ++i) {
      left.children[base + 1 + i] = right.children[i];
    }
  }
  left.key_count = static_cast<std::uint32_t>(base + 1 + right.key_count);

  for (size_type i = left_idx + 1; i < parent.key_count; ++i) {
    parent.keys[i - 1] = parent.keys[i];
    parent.values[i - 1] = parent.values[i];
  }
  for (size_type i = left_idx + 2; i <= parent.key_count; ++i) {
    parent.children[i - 1] = parent.children[i];
  }
  --parent.key_count;

  write_node(left_id, left);
  write_node(parent_id, parent);
  disk_.free_block(right_id);
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
void BTreeMap<Key, T, BlockSize, Compare>::ensure_min_plus_one(
    block_id_type parent_id, Node &parent, size_type child_idx) {
  if (child_idx > 0) {
    const Node left_sib = read_node(parent.children[child_idx - 1]);
    if (left_sib.key_count > kMinKeys) {
      borrow_from_prev(parent_id, parent, child_idx);
      return;
    }
  }
  if (child_idx < parent.key_count) {
    const Node right_sib = read_node(parent.children[child_idx + 1]);
    if (right_sib.key_count > kMinKeys) {
      borrow_from_next(parent_id, parent, child_idx);
      return;
    }
  }
  if (child_idx < parent.key_count) {
    merge_children(parent_id, parent, child_idx);
  } else {
    merge_children(parent_id, parent, child_idx - 1);
  }
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
bool BTreeMap<Key, T, BlockSize, Compare>::erase_from(block_id_type node_id,
                                                      const Key &key) {
  Node node = read_node(node_id);
  const size_type idx = lower_bound_index(node, key);

  if (idx < node.key_count && !comp_(key, node.keys[idx])) {
    if (node.is_leaf) {
      for (size_type i = idx + 1; i < node.key_count; ++i) {
        node.keys[i - 1] = node.keys[i];
        node.values[i - 1] = node.values[i];
      }
      --node.key_count;
      --size_;
      write_node(node_id, node);
      return true;
    }

    // Internal node match: replace with predecessor/successor or merge.
    const block_id_type left_id = node.children[idx];
    const Node left = read_node(left_id);
    if (left.key_count > kMinKeys) {
      const value_type pred = rightmost_entry(left_id);
      node.keys[idx] = pred.first;
      node.values[idx] = pred.second;
      write_node(node_id, node);
      return erase_from(left_id, pred.first);
    }

    const block_id_type right_id = node.children[idx + 1];
    const Node right = read_node(right_id);
    if (right.key_count > kMinKeys) {
      const value_type succ = leftmost_entry(right_id);
      node.keys[idx] = succ.first;
      node.values[idx] = succ.second;
      write_node(node_id, node);
      return erase_from(right_id, succ.first);
    }

    merge_children(node_id, node, idx);
    return erase_from(left_id, key);
  }

  if (node.is_leaf) {
    return false;
  }

  const bool last_child = (idx == node.key_count);
  const Node child = read_node(node.children[idx]);
  if (child.key_count == kMinKeys) {
    ensure_min_plus_one(node_id, node, idx);
  }

  // After a merge on the last child, the merged subtree is at idx - 1.
  const size_type next_idx =
      (last_child && idx > node.key_count) ? (idx - 1) : idx;
  return erase_from(node.children[next_idx], key);
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
bool BTreeMap<Key, T, BlockSize, Compare>::erase(const Key &key) {
  if (root_ == kInvalidBlock) {
    return false;
  }

  const bool removed = erase_from(root_, key);

  const Node root_node = read_node(root_);
  if (root_node.key_count == 0) {
    const block_id_type old_root = root_;
    if (root_node.is_leaf) {
      root_ = kInvalidBlock;
      height_ = 0;
    } else {
      root_ = root_node.children[0];
      --height_;
    }
    disk_.free_block(old_root);
  }

  return removed;
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
std::optional<typename BTreeMap<Key, T, BlockSize, Compare>::value_type>
BTreeMap<Key, T, BlockSize, Compare>::first_entry() const {
  if (root_ == kInvalidBlock) {
    return std::nullopt;
  }
  return leftmost_entry(root_);
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
std::optional<typename BTreeMap<Key, T, BlockSize, Compare>::value_type>
BTreeMap<Key, T, BlockSize, Compare>::last_entry() const {
  if (root_ == kInvalidBlock) {
    return std::nullopt;
  }
  return rightmost_entry(root_);
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
std::optional<typename BTreeMap<Key, T, BlockSize, Compare>::value_type>
BTreeMap<Key, T, BlockSize, Compare>::ceiling_entry(const Key &key) const {
  block_id_type curr = root_;
  std::optional<value_type> candidate;
  while (curr != kInvalidBlock) {
    const Node node = read_node(curr);
    const size_type idx = lower_bound_index(node, key);
    if (idx < node.key_count) {
      if (!comp_(key, node.keys[idx])) {
        return value_type{node.keys[idx], node.values[idx]};
      }
      candidate = value_type{node.keys[idx], node.values[idx]};
    }
    if (node.is_leaf) {
      break;
    }
    curr = node.children[idx];
  }
  return candidate;
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
std::optional<typename BTreeMap<Key, T, BlockSize, Compare>::value_type>
BTreeMap<Key, T, BlockSize, Compare>::floor_entry(const Key &key) const {
  block_id_type curr = root_;
  std::optional<value_type> candidate;
  while (curr != kInvalidBlock) {
    const Node node = read_node(curr);
    const size_type idx = lower_bound_index(node, key);
    if (idx < node.key_count && !comp_(key, node.keys[idx])) {
      return value_type{node.keys[idx], node.values[idx]};
    }
    if (idx > 0) {
      candidate = value_type{node.keys[idx - 1], node.values[idx - 1]};
    }
    if (node.is_leaf) {
      break;
    }
    curr = node.children[idx];
  }
  return candidate;
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
std::optional<typename BTreeMap<Key, T, BlockSize, Compare>::value_type>
BTreeMap<Key, T, BlockSize, Compare>::higher_entry(const Key &key) const {
  block_id_type curr = root_;
  std::optional<value_type> candidate;
  while (curr != kInvalidBlock) {
    const Node node = read_node(curr);
    // First index strictly greater than key.
    size_type lo = 0;
    size_type hi = node.key_count;
    while (lo < hi) {
      const size_type mid = lo + (hi - lo) / 2;
      if (!comp_(key, node.keys[mid])) {
        lo = mid + 1;
      } else {
        hi = mid;
      }
    }
    if (lo < node.key_count) {
      candidate = value_type{node.keys[lo], node.values[lo]};
    }
    if (node.is_leaf) {
      break;
    }
    curr = node.children[lo];
  }
  return candidate;
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
std::optional<typename BTreeMap<Key, T, BlockSize, Compare>::value_type>
BTreeMap<Key, T, BlockSize, Compare>::lower_entry(const Key &key) const {
  block_id_type curr = root_;
  std::optional<value_type> candidate;
  while (curr != kInvalidBlock) {
    const Node node = read_node(curr);
    const size_type idx = lower_bound_index(node, key);
    if (idx > 0) {
      candidate = value_type{node.keys[idx - 1], node.values[idx - 1]};
    }
    if (node.is_leaf) {
      break;
    }
    curr = node.children[idx];
  }
  return candidate;
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
void BTreeMap<Key, T, BlockSize, Compare>::collect_range(
    block_id_type node_id, const Key &from, const Key &to,
    std::vector<value_type> &out) const {
  const Node node = read_node(node_id);
  size_type i = lower_bound_index(node, from);

  for (; i < node.key_count; ++i) {
    if (!node.is_leaf) {
      collect_range(node.children[i], from, to, out);
    }
    if (!comp_(node.keys[i], to)) {
      return;
    }
    if (!comp_(node.keys[i], from)) {
      out.emplace_back(node.keys[i], node.values[i]);
    }
  }
  if (!node.is_leaf) {
    collect_range(node.children[node.key_count], from, to, out);
  }
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
std::vector<typename BTreeMap<Key, T, BlockSize, Compare>::value_type>
BTreeMap<Key, T, BlockSize, Compare>::sub_map(const Key &from,
                                              const Key &to) const {
  std::vector<value_type> out;
  if (root_ == kInvalidBlock || !comp_(from, to)) {
    return out;
  }
  collect_range(root_, from, to, out);
  return out;
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
void BTreeMap<Key, T, BlockSize, Compare>::collect_all(
    block_id_type node_id, std::vector<value_type> &out) const {
  const Node node = read_node(node_id);
  for (size_type i = 0; i < node.key_count; ++i) {
    if (!node.is_leaf) {
      collect_all(node.children[i], out);
    }
    out.emplace_back(node.keys[i], node.values[i]);
  }
  if (!node.is_leaf) {
    collect_all(node.children[node.key_count], out);
  }
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
std::vector<typename BTreeMap<Key, T, BlockSize, Compare>::value_type>
BTreeMap<Key, T, BlockSize, Compare>::entries() const {
  std::vector<value_type> out;
  if (root_ == kInvalidBlock) {
    return out;
  }
  out.reserve(size_);
  collect_all(root_, out);
  return out;
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
bool BTreeMap<Key, T, BlockSize, Compare>::validate_node(
    block_id_type node_id, const Key *min_bound, const Key *max_bound,
    size_type depth, std::optional<size_type> &leaf_depth,
    size_type &counted_keys, size_type &counted_blocks) const {
  if (node_id >= disk_.allocated_blocks()) {
    return false;
  }
  ++counted_blocks;
  const Node node = peek_node(node_id);

  if (node.key_count > kMaxKeys) {
    return false;
  }
  if (depth == 0) {
    if (node.key_count == 0) {
      return false;
    }
  } else {
    if (node.key_count < kMinKeys) {
      return false;
    }
  }

  for (size_type i = 0; i < node.key_count; ++i) {
    if (min_bound && !comp_(*min_bound, node.keys[i])) {
      return false;
    }
    if (max_bound && !comp_(node.keys[i], *max_bound)) {
      return false;
    }
    if (i > 0 && !comp_(node.keys[i - 1], node.keys[i])) {
      return false;
    }
  }

  counted_keys += node.key_count;

  if (node.is_leaf) {
    if (!leaf_depth) {
      leaf_depth = depth;
    } else if (*leaf_depth != depth) {
      return false;
    }
    return true;
  }

  for (size_type i = 0; i <= node.key_count; ++i) {
    const Key *child_min = (i == 0) ? min_bound : &node.keys[i - 1];
    const Key *child_max = (i == node.key_count) ? max_bound : &node.keys[i];
    if (!validate_node(node.children[i], child_min, child_max, depth + 1,
                       leaf_depth, counted_keys, counted_blocks)) {
      return false;
    }
  }
  return true;
}

template <typename Key, typename T, std::size_t BlockSize, typename Compare>
bool BTreeMap<Key, T, BlockSize, Compare>::validate() const {
  if (root_ == kInvalidBlock) {
    return size_ == 0 && height_ == 0 && disk_.live_blocks() == 0;
  }
  std::optional<size_type> leaf_depth;
  size_type counted_keys = 0;
  size_type counted_blocks = 0;
  if (!validate_node(root_, nullptr, nullptr, 0, leaf_depth, counted_keys,
                     counted_blocks)) {
    return false;
  }
  if (!leaf_depth || (*leaf_depth + 1) != height_) {
    return false;
  }
  return counted_keys == size_ && counted_blocks == disk_.live_blocks();
}

} // namespace ds

#endif // DS_B_TREE_MAP_TPP
