/**
 * @file skip_list_map.tpp
 * @brief Template implementation of adt::SkipListMap.
 *
 * @note Include this file through skip_list_map.hpp.
 */
#ifndef ADT_SKIP_LIST_MAP_TPP
#define ADT_SKIP_LIST_MAP_TPP

#ifndef ADT_SKIP_LIST_MAP_HPP
#error "Include skip_list_map.hpp instead of this implementation file."
#endif

#include <algorithm>
#include <stdexcept>

namespace adt {

template <typename K, typename V, typename C>
SkipListMap<K, V, C>::SkipListMap()
    : SkipListMap(32, 0.5, key_compare(), 1337) {}

template <typename K, typename V, typename C>
SkipListMap<K, V, C>::SkipListMap(size_type maxLevel, double probability,
                                 key_compare compare, std::uint32_t seed)
    : max_level_(maxLevel), probability_(probability), head_(maxLevel),
      compare_(std::move(compare)), random_engine_(seed) {
  if (max_level_ == 0) {
    throw std::invalid_argument(
        "adt::SkipListMap: maxLevel must be greater than zero");
  }
  if (!(probability_ > 0.0 && probability_ < 1.0)) {
    throw std::invalid_argument(
        "adt::SkipListMap: probability must be between zero and one");
  }
}

template <typename K, typename V, typename C>
SkipListMap<K, V, C>::SkipListMap(std::initializer_list<value_type> init,
                                 key_compare compare, size_type maxLevel,
                                 double probability, std::uint32_t seed)
    : SkipListMap(maxLevel, probability, std::move(compare), seed) {
  try {
    for (const auto &entry : init) {
      insert_or_assign(entry.first, entry.second);
    }
  } catch (...) {
    clear();
    throw;
  }
}

template <typename K, typename V, typename C>
SkipListMap<K, V, C>::SkipListMap(const SkipListMap &other)
    : SkipListMap(other.max_level_, other.probability_, other.compare_, 1337) {
  random_engine_ = other.random_engine_;
  try {
    copyFrom(other);
  } catch (...) {
    clear();
    throw;
  }
}

template <typename K, typename V, typename C>
SkipListMap<K, V, C>::SkipListMap(SkipListMap &&other)
    : SkipListMap(other.max_level_, other.probability_, other.compare_, 1337) {
  random_engine_ = other.random_engine_;
  swapContents(other);
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::operator=(const SkipListMap &other) -> SkipListMap & {
  if (this != std::addressof(other)) {
    SkipListMap copy(other);
    swapContents(copy);
  }
  return *this;
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::operator=(SkipListMap &&other) -> SkipListMap & {
  if (this != std::addressof(other)) {
    SkipListMap moved(std::move(other));
    swapContents(moved);
  }
  return *this;
}

template <typename K, typename V, typename C>
SkipListMap<K, V, C>::~SkipListMap() {
  clear();
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::size() const noexcept -> size_type {
  return size_;
}

template <typename K, typename V, typename C>
bool SkipListMap<K, V, C>::empty() const noexcept {
  return size_ == 0;
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::begin() noexcept -> iterator {
  return iterator{head_.forward[0]};
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::begin() const noexcept -> const_iterator {
  return const_iterator{head_.forward[0]};
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::cbegin() const noexcept -> const_iterator {
  return begin();
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::end() noexcept -> iterator {
  return iterator{nullptr};
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::end() const noexcept -> const_iterator {
  return const_iterator{nullptr};
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::cend() const noexcept -> const_iterator {
  return end();
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::find(const key_type &key) -> iterator {
  return iterator{findNode(key)};
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::find(const key_type &key) const -> const_iterator {
  return const_iterator{findNode(key)};
}

template <typename K, typename V, typename C>
bool SkipListMap<K, V, C>::contains(const key_type &key) const {
  return findNode(key) != nullptr;
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::at(const key_type &key) -> mapped_type & {
  Node *const node = findNode(key);
  if (node == nullptr) {
    throw std::out_of_range("adt::SkipListMap::at: key not found");
  }
  return node->entry->second;
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::at(const key_type &key) const
    -> const mapped_type & {
  const Node *const node = findNode(key);
  if (node == nullptr) {
    throw std::out_of_range("adt::SkipListMap::at: key not found");
  }
  return node->entry->second;
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::lower_bound(const key_type &key) -> iterator {
  return iterator{lowerBoundNode(key)};
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::lower_bound(const key_type &key) const
    -> const_iterator {
  return const_iterator{lowerBoundNode(key)};
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::upper_bound(const key_type &key) -> iterator {
  Node *node = lowerBoundNode(key);
  if (node != nullptr && !compare_(key, node->entry->first)) {
    node = node->forward[0];
  }
  return iterator{node};
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::upper_bound(const key_type &key) const
    -> const_iterator {
  Node *node = lowerBoundNode(key);
  if (node != nullptr && !compare_(key, node->entry->first)) {
    node = node->forward[0];
  }
  return const_iterator{node};
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::predecessor(const key_type &key) -> iterator {
  const auto update = predecessors(key);
  return iterator{update[0] == std::addressof(head_) ? nullptr : update[0]};
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::predecessor(const key_type &key) const
    -> const_iterator {
  const auto update = predecessors(key);
  return const_iterator{update[0] == std::addressof(head_) ? nullptr
                                                           : update[0]};
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::successor(const key_type &key) -> iterator {
  return upper_bound(key);
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::successor(const key_type &key) const
    -> const_iterator {
  return upper_bound(key);
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::min() const -> std::optional<value_type> {
  if (empty()) {
    return std::nullopt;
  }
  return *head_.forward[0]->entry;
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::max() const -> std::optional<value_type> {
  if (empty()) {
    return std::nullopt;
  }
  const Node *node = std::addressof(head_);
  for (size_type level = max_level_; level-- > 0;) {
    while (node->forward[level] != nullptr) {
      node = node->forward[level];
    }
  }
  return *node->entry;
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::key_comp() const
    noexcept(std::is_nothrow_copy_constructible_v<key_compare>) -> key_compare {
  return compare_;
}

template <typename K, typename V, typename C>
template <typename... Args>
auto SkipListMap<K, V, C>::try_emplace(const key_type &key, Args &&...args)
    -> std::pair<iterator, bool> {
  auto update = predecessors(key);
  Node *const next = update[0]->forward[0];
  if (next != nullptr && !compare_(key, next->entry->first)) {
    return {iterator{next}, false};
  }
  return {iterator{insertNode(update, key, std::forward<Args>(args)...)}, true};
}

template <typename K, typename V, typename C>
template <typename... Args>
auto SkipListMap<K, V, C>::try_emplace(key_type &&key, Args &&...args)
    -> std::pair<iterator, bool> {
  auto update = predecessors(key);
  Node *const next = update[0]->forward[0];
  if (next != nullptr && !compare_(key, next->entry->first)) {
    return {iterator{next}, false};
  }
  return {
      iterator{insertNode(update, std::move(key), std::forward<Args>(args)...)},
      true};
}

template <typename K, typename V, typename C>
template <typename M>
auto SkipListMap<K, V, C>::insert_or_assign(const key_type &key, M &&value)
    -> std::pair<iterator, bool> {
  auto update = predecessors(key);
  Node *const next = update[0]->forward[0];
  if (next != nullptr && !compare_(key, next->entry->first)) {
    next->entry->second = std::forward<M>(value);
    return {iterator{next}, false};
  }
  return {iterator{insertNode(update, key, std::forward<M>(value))}, true};
}

template <typename K, typename V, typename C>
template <typename M>
auto SkipListMap<K, V, C>::insert_or_assign(key_type &&key, M &&value)
    -> std::pair<iterator, bool> {
  auto update = predecessors(key);
  Node *const next = update[0]->forward[0];
  if (next != nullptr && !compare_(key, next->entry->first)) {
    next->entry->second = std::forward<M>(value);
    return {iterator{next}, false};
  }
  return {iterator{insertNode(update, std::move(key), std::forward<M>(value))},
          true};
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::insert(const value_type &entry)
    -> std::pair<iterator, bool> {
  return try_emplace(entry.first, entry.second);
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::insert(value_type &&entry)
    -> std::pair<iterator, bool> {
  return try_emplace(entry.first, std::move(entry.second));
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::operator[](const key_type &key) -> mapped_type & {
  return try_emplace(key).first->second;
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::operator[](key_type &&key) -> mapped_type & {
  return try_emplace(std::move(key)).first->second;
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::erase(const key_type &key) -> size_type {
  auto update = predecessors(key);
  Node *const victim = update[0]->forward[0];
  if (victim == nullptr || compare_(key, victim->entry->first)) {
    return 0;
  }
  for (size_type level = 0; level < victim->forward.size(); ++level) {
    if (update[level]->forward[level] == victim) {
      update[level]->forward[level] = victim->forward[level];
    }
  }
  delete victim;
  --size_;
  return 1;
}

template <typename K, typename V, typename C>
void SkipListMap<K, V, C>::clear() noexcept {
  Node *node = head_.forward[0];
  while (node != nullptr) {
    Node *next = node->forward[0];
    delete node;
    node = next;
  }
  std::fill(head_.forward.begin(), head_.forward.end(), nullptr);
  size_ = 0;
}

template <typename K, typename V, typename C>
void SkipListMap<K, V, C>::swap(SkipListMap &other) noexcept(
    std::is_nothrow_swappable_v<key_compare>) {
  swapContents(other);
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::lowerBoundNode(const key_type &key) const -> Node * {
  Node *node = const_cast<Node *>(std::addressof(head_));
  for (size_type level = max_level_; level-- > 0;) {
    while (node->forward[level] != nullptr &&
           compare_(node->forward[level]->entry->first, key)) {
      node = node->forward[level];
    }
  }
  return node->forward[0];
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::findNode(const key_type &key) const -> Node * {
  Node *node = lowerBoundNode(key);
  if (node != nullptr && !compare_(key, node->entry->first)) {
    return node;
  }
  return nullptr;
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::predecessors(const key_type &key) const
    -> std::vector<Node *> {
  std::vector<Node *> update(max_level_);
  Node *node = const_cast<Node *>(std::addressof(head_));
  for (size_type level = max_level_; level-- > 0;) {
    while (node->forward[level] != nullptr &&
           compare_(node->forward[level]->entry->first, key)) {
      node = node->forward[level];
    }
    update[level] = node;
  }
  return update;
}

template <typename K, typename V, typename C>
auto SkipListMap<K, V, C>::randomHeight() -> size_type {
  size_type height = 1;
  while (height < max_level_ && distribution_(random_engine_) < probability_) {
    ++height;
  }
  return height;
}

template <typename K, typename V, typename C>
template <typename KeyArg, typename... Args>
auto SkipListMap<K, V, C>::insertNode(const std::vector<Node *> &update,
                                      KeyArg &&key, Args &&...args) -> Node * {
  const size_type height = randomHeight();
  Node *const node =
      new Node(height, std::forward<KeyArg>(key), std::forward<Args>(args)...);
  for (size_type level = 0; level < height; ++level) {
    node->forward[level] = update[level]->forward[level];
    update[level]->forward[level] = node;
  }
  ++size_;
  return node;
}

template <typename K, typename V, typename C>
void SkipListMap<K, V, C>::copyFrom(const SkipListMap &other) {
  for (Node *source = other.head_.forward[0]; source != nullptr;
       source = source->forward[0]) {
    auto update = predecessors(source->entry->first);
    const size_type height = source->forward.size();
    Node *const node = new Node(height, source->entry->first,
                                source->entry->second);
    for (size_type level = 0; level < height; ++level) {
      node->forward[level] = update[level]->forward[level];
      update[level]->forward[level] = node;
    }
    ++size_;
  }
}

template <typename K, typename V, typename C>
void SkipListMap<K, V, C>::swapContents(
    SkipListMap &other) noexcept(std::is_nothrow_swappable_v<key_compare>) {
  using std::swap;
  head_.forward.swap(other.head_.forward);
  swap(size_, other.size_);
  swap(max_level_, other.max_level_);
  swap(probability_, other.probability_);
  swap(compare_, other.compare_);
  swap(random_engine_, other.random_engine_);
  swap(distribution_, other.distribution_);
}

template <typename K, typename V, typename C>
bool operator==(const SkipListMap<K, V, C> &lhs,
                const SkipListMap<K, V, C> &rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  const auto compare = lhs.key_comp();
  auto left = lhs.begin();
  auto right = rhs.begin();
  for (; left != lhs.end(); ++left, ++right) {
    if (compare(left->first, right->first) ||
        compare(right->first, left->first) ||
        !(left->second == right->second)) {
      return false;
    }
  }
  return true;
}

template <typename K, typename V, typename C>
bool operator!=(const SkipListMap<K, V, C> &lhs,
                const SkipListMap<K, V, C> &rhs) {
  return !(lhs == rhs);
}

template <typename K, typename V, typename C>
void swap(SkipListMap<K, V, C> &lhs,
          SkipListMap<K, V, C> &rhs) noexcept(noexcept(lhs.swap(rhs))) {
  lhs.swap(rhs);
}

} // namespace adt

#endif // ADT_SKIP_LIST_MAP_TPP
