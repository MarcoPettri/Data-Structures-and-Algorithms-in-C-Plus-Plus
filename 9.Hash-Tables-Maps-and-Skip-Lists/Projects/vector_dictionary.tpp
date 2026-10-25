/**
 * @file vector_dictionary.tpp
 * @brief Template implementation of adt::VectorDictionary.
 */
#ifndef ADT_VECTOR_DICTIONARY_TPP
#define ADT_VECTOR_DICTIONARY_TPP

#ifndef ADT_VECTOR_DICTIONARY_HPP
#error "Include vector_dictionary.hpp instead of this implementation file."
#endif

#include <stdexcept>

namespace adt {

template <typename K, typename V, typename E>
VectorDictionary<K, V, E>::VectorDictionary(key_equal equal) noexcept(
    std::is_nothrow_move_constructible_v<key_equal>)
    : equal_(std::move(equal)) {}

template <typename K, typename V, typename E>
VectorDictionary<K, V, E>::VectorDictionary(
    std::initializer_list<value_type> init, key_equal equal)
    : equal_(std::move(equal)) {
  for (const value_type &entry : init) {
    try_emplace(entry.first, entry.second);
  }
}

template <typename K, typename V, typename E>
VectorDictionary<K, V, E>::VectorDictionary(const VectorDictionary &other)
    : entries_(other.entries_), equal_(other.equal_) {}

template <typename K, typename V, typename E>
VectorDictionary<K, V, E>::VectorDictionary(VectorDictionary &&other) noexcept(
    std::is_nothrow_move_constructible_v<key_equal>)
    : entries_(std::move(other.entries_)), equal_(std::move(other.equal_)) {
  other.entries_.clear();
}

template <typename K, typename V, typename E>
auto VectorDictionary<K, V, E>::operator=(const VectorDictionary &other)
    -> VectorDictionary & {
  if (this != std::addressof(other)) {
    VectorDictionary copy(other);
    swap(copy);
  }
  return *this;
}

template <typename K, typename V, typename E>
auto VectorDictionary<K, V, E>::operator=(VectorDictionary &&other) noexcept(
    std::is_nothrow_move_assignable_v<key_equal>) -> VectorDictionary & {
  if (this != std::addressof(other)) {
    entries_ = std::move(other.entries_);
    equal_ = std::move(other.equal_);
    other.entries_.clear();
  }
  return *this;
}

template <typename K, typename V, typename E>
auto VectorDictionary<K, V, E>::size() const noexcept -> size_type {
  return entries_.size();
}

template <typename K, typename V, typename E>
bool VectorDictionary<K, V, E>::empty() const noexcept {
  return entries_.empty();
}

template <typename K, typename V, typename E>
auto VectorDictionary<K, V, E>::begin() noexcept -> iterator {
  return entries_.begin();
}

template <typename K, typename V, typename E>
auto VectorDictionary<K, V, E>::begin() const noexcept -> const_iterator {
  return entries_.begin();
}

template <typename K, typename V, typename E>
auto VectorDictionary<K, V, E>::cbegin() const noexcept -> const_iterator {
  return entries_.cbegin();
}

template <typename K, typename V, typename E>
auto VectorDictionary<K, V, E>::end() noexcept -> iterator {
  return entries_.end();
}

template <typename K, typename V, typename E>
auto VectorDictionary<K, V, E>::end() const noexcept -> const_iterator {
  return entries_.end();
}

template <typename K, typename V, typename E>
auto VectorDictionary<K, V, E>::cend() const noexcept -> const_iterator {
  return entries_.cend();
}

template <typename K, typename V, typename E>
auto VectorDictionary<K, V, E>::find(const key_type &key) -> iterator {
  for (auto it = entries_.begin(); it != entries_.end(); ++it) {
    if (equal_(it->first, key)) {
      return it;
    }
  }
  return end();
}

template <typename K, typename V, typename E>
auto VectorDictionary<K, V, E>::find(const key_type &key) const
    -> const_iterator {
  for (auto it = entries_.cbegin(); it != entries_.cend(); ++it) {
    if (equal_(it->first, key)) {
      return it;
    }
  }
  return cend();
}

template <typename K, typename V, typename E>
bool VectorDictionary<K, V, E>::contains(const key_type &key) const {
  return find(key) != cend();
}

template <typename K, typename V, typename E>
auto VectorDictionary<K, V, E>::at(const key_type &key) -> mapped_type & {
  const auto it = find(key);
  if (it == end()) {
    throw std::out_of_range("adt::VectorDictionary::at: key not found");
  }
  return it->second;
}

template <typename K, typename V, typename E>
auto VectorDictionary<K, V, E>::at(const key_type &key) const
    -> const mapped_type & {
  const auto it = find(key);
  if (it == cend()) {
    throw std::out_of_range("adt::VectorDictionary::at: key not found");
  }
  return it->second;
}

template <typename K, typename V, typename E>
template <typename... Args>
auto VectorDictionary<K, V, E>::try_emplace(const key_type &key, Args &&...args)
    -> std::pair<iterator, bool> {
  const auto existing = find(key);
  if (existing != end()) {
    return {existing, false};
  }
  return {append(key, std::forward<Args>(args)...), true};
}

template <typename K, typename V, typename E>
template <typename... Args>
auto VectorDictionary<K, V, E>::try_emplace(key_type &&key, Args &&...args)
    -> std::pair<iterator, bool> {
  const auto existing = find(key);
  if (existing != end()) {
    return {existing, false};
  }
  return {append(std::move(key), std::forward<Args>(args)...), true};
}

template <typename K, typename V, typename E>
auto VectorDictionary<K, V, E>::insert(const value_type &value)
    -> std::pair<iterator, bool> {
  return try_emplace(value.first, value.second);
}

template <typename K, typename V, typename E>
auto VectorDictionary<K, V, E>::insert(value_type &&value)
    -> std::pair<iterator, bool> {
  return try_emplace(value.first, std::move(value.second));
}

template <typename K, typename V, typename E>
template <typename M>
auto VectorDictionary<K, V, E>::insert_or_assign(const key_type &key, M &&obj)
    -> std::pair<iterator, bool> {
  const auto existing = find(key);
  if (existing != end()) {
    existing->second = std::forward<M>(obj);
    return {existing, false};
  }
  return {append(key, std::forward<M>(obj)), true};
}

template <typename K, typename V, typename E>
template <typename M>
auto VectorDictionary<K, V, E>::insert_or_assign(key_type &&key, M &&obj)
    -> std::pair<iterator, bool> {
  const auto existing = find(key);
  if (existing != end()) {
    existing->second = std::forward<M>(obj);
    return {existing, false};
  }
  return {append(std::move(key), std::forward<M>(obj)), true};
}

template <typename K, typename V, typename E>
auto VectorDictionary<K, V, E>::operator[](const key_type &key)
    -> mapped_type & {
  return try_emplace(key).first->second;
}

template <typename K, typename V, typename E>
auto VectorDictionary<K, V, E>::operator[](key_type &&key) -> mapped_type & {
  return try_emplace(std::move(key)).first->second;
}

template <typename K, typename V, typename E>
auto VectorDictionary<K, V, E>::erase(const key_type &key) -> size_type {
  const auto it = find(key);
  if (it == end()) {
    return 0;
  }
  entries_.erase(it);
  return 1;
}

template <typename K, typename V, typename E>
void VectorDictionary<K, V, E>::clear() noexcept {
  entries_.clear();
}

template <typename K, typename V, typename E>
void VectorDictionary<K, V, E>::swap(VectorDictionary &other) noexcept(
    std::is_nothrow_swappable_v<key_equal>) {
  using std::swap;
  entries_.swap(other.entries_);
  swap(equal_, other.equal_);
}

template <typename K, typename V, typename E>
auto VectorDictionary<K, V, E>::key_eq() const
    noexcept(std::is_nothrow_copy_constructible_v<key_equal>) -> key_equal {
  return equal_;
}

template <typename K, typename V, typename E>
template <typename KeyArg, typename... Args>
auto VectorDictionary<K, V, E>::append(KeyArg &&key, Args &&...args)
    -> iterator {
  entries_.emplace_back(std::piecewise_construct, std::forward<KeyArg>(key),
                        std::forward<Args>(args)...);
  return std::prev(entries_.end());
}

template <typename K, typename V, typename E>
bool operator==(const VectorDictionary<K, V, E> &lhs,
                const VectorDictionary<K, V, E> &rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (const auto &entry : lhs) {
    const auto match = rhs.find(entry.first);
    if (match == rhs.end() || !(match->second == entry.second)) {
      return false;
    }
  }
  return true;
}

template <typename K, typename V, typename E>
bool operator!=(const VectorDictionary<K, V, E> &lhs,
                const VectorDictionary<K, V, E> &rhs) {
  return !(lhs == rhs);
}

template <typename K, typename V, typename E>
void swap(VectorDictionary<K, V, E> &lhs,
          VectorDictionary<K, V, E> &rhs) noexcept(noexcept(lhs.swap(rhs))) {
  lhs.swap(rhs);
}

} // namespace adt

#endif // ADT_VECTOR_DICTIONARY_TPP
