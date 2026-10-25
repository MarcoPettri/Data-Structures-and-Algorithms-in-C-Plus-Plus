/**
 * @file linked_list_dictionary.tpp
 * @brief Template implementation of adt::LinkedListDictionary.
 *
 * @note This file is an implementation detail of `linked_list_dictionary.hpp`
 *       and must not be included directly. Member documentation lives in the
 * header.
 */
#ifndef ADT_LINKED_LIST_DICTIONARY_TPP
#define ADT_LINKED_LIST_DICTIONARY_TPP

#ifndef ADT_LINKED_LIST_DICTIONARY_HPP
#error                                                                         \
    "Do not include linked_list_dictionary.tpp directly; include linked_list_dictionary.hpp."
#endif

#include <stdexcept>

namespace adt {

// ---------------------------------------------------------------------------
// Construction, copy, move and destruction
// ---------------------------------------------------------------------------

template <typename K, typename V, typename E>
LinkedListDictionary<K, V, E>::LinkedListDictionary(key_equal equal) noexcept(
    std::is_nothrow_move_constructible_v<key_equal>)
    : equal_(std::move(equal)) {}

template <typename K, typename V, typename E>
LinkedListDictionary<K, V, E>::LinkedListDictionary(
    std::initializer_list<value_type> init, key_equal equal)
    : equal_(std::move(equal)) {
  try {
    for (const value_type &entry : init) {
      try_emplace(entry.first, entry.second); // first occurrence of a key wins
    }
  } catch (...) {
    clear(); // the destructor is not run for a partially constructed object
    throw;
  }
}

template <typename K, typename V, typename E>
LinkedListDictionary<K, V, E>::LinkedListDictionary(
    const LinkedListDictionary &other)
    : equal_(other.equal_) {
  try {
    // Keys in `other` are already unique, so skip the O(n) duplicate lookup.
    for (const value_type &entry : other) {
      appendNode(entry.first, entry.second);
    }
  } catch (...) {
    clear();
    throw;
  }
}

template <typename K, typename V, typename E>
LinkedListDictionary<K, V, E>::LinkedListDictionary(
    LinkedListDictionary
        &&other) noexcept(std::is_nothrow_move_constructible_v<key_equal>)
    : head_(std::move(other.head_)), tail_(std::exchange(other.tail_, nullptr)),
      size_(std::exchange(other.size_, 0)), equal_(std::move(other.equal_)) {}

template <typename K, typename V, typename E>
auto LinkedListDictionary<K, V, E>::operator=(const LinkedListDictionary &other)
    -> LinkedListDictionary & {
  if (this != std::addressof(other)) {
    LinkedListDictionary copy(
        other); // may throw; *this is untouched until the swap
    swap(copy);
  }
  return *this;
}

template <typename K, typename V, typename E>
auto LinkedListDictionary<K, V, E>::operator=(
    LinkedListDictionary
        &&other) noexcept(std::is_nothrow_move_assignable_v<key_equal>)
    -> LinkedListDictionary & {
  if (this != std::addressof(other)) {
    clear();
    head_ = std::move(other.head_);
    tail_ = std::exchange(other.tail_, nullptr);
    size_ = std::exchange(other.size_, 0);
    equal_ = std::move(other.equal_);
  }
  return *this;
}

template <typename K, typename V, typename E>
LinkedListDictionary<K, V, E>::~LinkedListDictionary() {
  clear();
}

// ---------------------------------------------------------------------------
// Capacity
// ---------------------------------------------------------------------------

template <typename K, typename V, typename E>
auto LinkedListDictionary<K, V, E>::size() const noexcept -> size_type {
  return size_;
}

template <typename K, typename V, typename E>
bool LinkedListDictionary<K, V, E>::empty() const noexcept {
  return size_ == 0;
}

// ---------------------------------------------------------------------------
// Iterators
// ---------------------------------------------------------------------------

template <typename K, typename V, typename E>
auto LinkedListDictionary<K, V, E>::begin() noexcept -> iterator {
  return iterator{head_.get()};
}

template <typename K, typename V, typename E>
auto LinkedListDictionary<K, V, E>::begin() const noexcept -> const_iterator {
  return const_iterator{head_.get()};
}

template <typename K, typename V, typename E>
auto LinkedListDictionary<K, V, E>::cbegin() const noexcept -> const_iterator {
  return begin();
}

template <typename K, typename V, typename E>
auto LinkedListDictionary<K, V, E>::end() noexcept -> iterator {
  return iterator{nullptr};
}

template <typename K, typename V, typename E>
auto LinkedListDictionary<K, V, E>::end() const noexcept -> const_iterator {
  return const_iterator{nullptr};
}

template <typename K, typename V, typename E>
auto LinkedListDictionary<K, V, E>::cend() const noexcept -> const_iterator {
  return end();
}

// ---------------------------------------------------------------------------
// Lookup
// ---------------------------------------------------------------------------

template <typename K, typename V, typename E>
auto LinkedListDictionary<K, V, E>::find(const key_type &key) noexcept(
    kNothrowKeyEqual) -> iterator {
  return iterator{findNode(key)};
}

template <typename K, typename V, typename E>
auto LinkedListDictionary<K, V, E>::find(const key_type &key) const
    noexcept(kNothrowKeyEqual) -> const_iterator {
  return const_iterator{findNode(key)};
}

template <typename K, typename V, typename E>
bool LinkedListDictionary<K, V, E>::contains(const key_type &key) const
    noexcept(kNothrowKeyEqual) {
  return findNode(key) != nullptr;
}

template <typename K, typename V, typename E>
auto LinkedListDictionary<K, V, E>::at(const key_type &key) -> mapped_type & {
  Node *const node = findNode(key);
  if (node == nullptr) {
    throw std::out_of_range("adt::LinkedListDictionary::at: key not found");
  }
  return node->entry.second;
}

template <typename K, typename V, typename E>
auto LinkedListDictionary<K, V, E>::at(const key_type &key) const
    -> const mapped_type & {
  const Node *const node = findNode(key);
  if (node == nullptr) {
    throw std::out_of_range("adt::LinkedListDictionary::at: key not found");
  }
  return node->entry.second;
}

// ---------------------------------------------------------------------------
// Modifiers
// ---------------------------------------------------------------------------

template <typename K, typename V, typename E>
template <typename... Args>
auto LinkedListDictionary<K, V, E>::try_emplace(const key_type &key,
                                                Args &&...args)
    -> std::pair<iterator, bool> {
  if (Node *const existing = findNode(key)) {
    return {iterator{existing}, false};
  }
  return {iterator{appendNode(key, std::forward<Args>(args)...)}, true};
}

template <typename K, typename V, typename E>
template <typename... Args>
auto LinkedListDictionary<K, V, E>::try_emplace(key_type &&key, Args &&...args)
    -> std::pair<iterator, bool> {
  if (Node *const existing = findNode(key)) {
    return {iterator{existing}, false}; // `key` has not been moved from
  }
  return {iterator{appendNode(std::move(key), std::forward<Args>(args)...)},
          true};
}

template <typename K, typename V, typename E>
auto LinkedListDictionary<K, V, E>::insert(const value_type &value)
    -> std::pair<iterator, bool> {
  return try_emplace(value.first, value.second);
}

template <typename K, typename V, typename E>
auto LinkedListDictionary<K, V, E>::insert(value_type &&value)
    -> std::pair<iterator, bool> {
  return try_emplace(value.first, std::move(value.second));
}

template <typename K, typename V, typename E>
template <typename M>
auto LinkedListDictionary<K, V, E>::insert_or_assign(const key_type &key,
                                                     M &&obj)
    -> std::pair<iterator, bool> {
  if (Node *const existing = findNode(key)) {
    existing->entry.second = std::forward<M>(obj);
    return {iterator{existing}, false};
  }
  return {iterator{appendNode(key, std::forward<M>(obj))}, true};
}

template <typename K, typename V, typename E>
template <typename M>
auto LinkedListDictionary<K, V, E>::insert_or_assign(key_type &&key, M &&obj)
    -> std::pair<iterator, bool> {
  if (Node *const existing = findNode(key)) {
    existing->entry.second = std::forward<M>(obj);
    return {iterator{existing}, false};
  }
  return {iterator{appendNode(std::move(key), std::forward<M>(obj))}, true};
}

template <typename K, typename V, typename E>
auto LinkedListDictionary<K, V, E>::operator[](const key_type &key)
    -> mapped_type & {
  return try_emplace(key).first->second;
}

template <typename K, typename V, typename E>
auto LinkedListDictionary<K, V, E>::operator[](key_type &&key)
    -> mapped_type & {
  return try_emplace(std::move(key)).first->second;
}

template <typename K, typename V, typename E>
auto LinkedListDictionary<K, V, E>::erase(const key_type &key) noexcept(
    kNothrowKeyEqual) -> size_type {
  // `link` points at the owning pointer that references the node under
  // inspection, so unlinking needs no special case for the head.
  std::unique_ptr<Node> *link = &head_;
  Node *previous = nullptr;
  while (*link != nullptr) {
    if (equal_((*link)->entry.first, key)) {
      std::unique_ptr<Node> victim = std::move(*link);
      *link = std::move(
          victim->next); // victim->next is now null: no recursive destruction
      if (victim.get() == tail_) {
        tail_ = previous;
      }
      --size_;
      return 1;
    }
    previous = link->get();
    link = &previous->next;
  }
  return 0;
}

template <typename K, typename V, typename E>
void LinkedListDictionary<K, V, E>::clear() noexcept {
  // Detach nodes one at a time. Resetting `head_` directly would destroy the
  // chain recursively (one stack frame per node) and could overflow on long
  // lists.
  while (head_ != nullptr) {
    std::unique_ptr<Node> next = std::move(head_->next);
    head_ = std::move(next);
  }
  tail_ = nullptr;
  size_ = 0;
}

template <typename K, typename V, typename E>
void LinkedListDictionary<K, V, E>::swap(LinkedListDictionary &other) noexcept(
    std::is_nothrow_swappable_v<key_equal>) {
  using std::swap;
  swap(head_, other.head_);
  swap(tail_, other.tail_);
  swap(size_, other.size_);
  swap(equal_, other.equal_);
}

// ---------------------------------------------------------------------------
// Observers
// ---------------------------------------------------------------------------

template <typename K, typename V, typename E>
auto LinkedListDictionary<K, V, E>::key_eq() const
    noexcept(std::is_nothrow_copy_constructible_v<key_equal>) -> key_equal {
  return equal_;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

template <typename K, typename V, typename E>
auto LinkedListDictionary<K, V, E>::findNode(const key_type &key) const
    noexcept(kNothrowKeyEqual) -> Node * {
  for (Node *node = head_.get(); node != nullptr; node = node->next.get()) {
    if (equal_(node->entry.first, key)) {
      return node;
    }
  }
  return nullptr;
}

template <typename K, typename V, typename E>
template <typename KeyArg, typename... Args>
auto LinkedListDictionary<K, V, E>::appendNode(KeyArg &&key, Args &&...args)
    -> Node * {
  // Allocation and construction come first: if either throws, no member has
  // changed.
  auto node = std::make_unique<Node>(std::forward<KeyArg>(key),
                                     std::forward<Args>(args)...);
  Node *const raw = node.get();
  if (tail_ != nullptr) {
    tail_->next = std::move(node);
  } else {
    head_ = std::move(node);
  }
  tail_ = raw;
  ++size_;
  return raw;
}

// ---------------------------------------------------------------------------
// Non-member functions
// ---------------------------------------------------------------------------

template <typename K, typename V, typename E>
bool operator==(const LinkedListDictionary<K, V, E> &lhs,
                const LinkedListDictionary<K, V, E> &rhs) {
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
bool operator!=(const LinkedListDictionary<K, V, E> &lhs,
                const LinkedListDictionary<K, V, E> &rhs) {
  return !(lhs == rhs);
}

template <typename K, typename V, typename E>
void swap(
    LinkedListDictionary<K, V, E> &lhs,
    LinkedListDictionary<K, V, E> &rhs) noexcept(noexcept(lhs.swap(rhs))) {
  lhs.swap(rhs);
}

} // namespace adt

#endif // ADT_LINKED_LIST_DICTIONARY_TPP