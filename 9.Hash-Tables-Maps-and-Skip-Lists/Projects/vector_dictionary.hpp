/**
 * @file vector_dictionary.hpp
 * @brief Dictionary (associative array) ADT backed by a contiguous vector.
 *
 * Entries are kept in insertion order. Lookup is linear; insertion appends
 * and may reallocate the vector, while erasure shifts later entries.
 */
#ifndef ADT_VECTOR_DICTIONARY_HPP
#define ADT_VECTOR_DICTIONARY_HPP

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace adt {

/**
 * @brief Dictionary mapping unique keys to values using a vector.
 *
 * @tparam Key Key type. Key assignment is required by vector erasure.
 * @tparam Value Mapped type.
 * @tparam KeyEqual Predicate testing key equivalence.
 *
 * Insertion order is preserved. Insertion and erasure invalidate iterators,
 * pointers and references at or after the modified position; vector
 * reallocation invalidates all of them.
 */
template <typename Key, typename Value, typename KeyEqual = std::equal_to<Key>>
class VectorDictionary {
  static_assert(
      std::is_invocable_r_v<bool, const KeyEqual &, const Key &, const Key &>,
      "KeyEqual must be callable as bool(const Key&, const Key&) on a const "
      "object");

public:
  /// Entry interface compatible with the usual map `first` and `second`.
  class Entry {
  private:
    Key key_;

  public:
    const Key &first;
    Value second;

    template <typename KeyArg, typename ValueArg>
    Entry(KeyArg &&key, ValueArg &&value)
        : key_(std::forward<KeyArg>(key)), first(key_),
          second(std::forward<ValueArg>(value)) {}

    template <typename KeyArg, typename... Args>
    Entry(std::piecewise_construct_t, KeyArg &&key, Args &&...args)
        : key_(std::forward<KeyArg>(key)), first(key_),
          second(std::forward<Args>(args)...) {}

    Entry(const Entry &other)
        : key_(other.key_), first(key_), second(other.second) {}

    Entry(Entry &&other) noexcept(std::is_nothrow_move_constructible_v<Key> &&
                                  std::is_nothrow_move_constructible_v<Value>)
        : key_(std::move(other.key_)), first(key_),
          second(std::move(other.second)) {}

    Entry &operator=(const Entry &other) {
      if (this != std::addressof(other)) {
        key_ = other.key_;
        second = other.second;
      }
      return *this;
    }

    Entry &operator=(Entry &&other) noexcept(
        std::is_nothrow_move_assignable_v<Key> &&
        std::is_nothrow_move_assignable_v<Value>) {
      if (this != std::addressof(other)) {
        key_ = std::move(other.key_);
        second = std::move(other.second);
      }
      return *this;
    }
  };

  using key_type = Key;
  using mapped_type = Value;
  using value_type = Entry;
  using key_equal = KeyEqual;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = value_type &;
  using const_reference = const value_type &;
  using pointer = value_type *;
  using const_pointer = const value_type *;
  using iterator = typename std::vector<value_type>::iterator;
  using const_iterator = typename std::vector<value_type>::const_iterator;

  /// @name Construction, copy, move and destruction
  /// @{
  VectorDictionary() = default;
  explicit VectorDictionary(key_equal equal) noexcept(
      std::is_nothrow_move_constructible_v<key_equal>);
  VectorDictionary(std::initializer_list<value_type> init,
                   key_equal equal = key_equal());
  VectorDictionary(const VectorDictionary &other);
  VectorDictionary(VectorDictionary &&other) noexcept(
      std::is_nothrow_move_constructible_v<key_equal>);
  VectorDictionary &operator=(const VectorDictionary &other);
  VectorDictionary &operator=(VectorDictionary &&other) noexcept(
      std::is_nothrow_move_assignable_v<key_equal>);
  ~VectorDictionary() = default;
  /// @}

  /// @name Capacity and iteration
  /// @{
  [[nodiscard]] size_type size() const noexcept;
  [[nodiscard]] bool empty() const noexcept;
  [[nodiscard]] iterator begin() noexcept;
  [[nodiscard]] const_iterator begin() const noexcept;
  [[nodiscard]] const_iterator cbegin() const noexcept;
  [[nodiscard]] iterator end() noexcept;
  [[nodiscard]] const_iterator end() const noexcept;
  [[nodiscard]] const_iterator cend() const noexcept;
  /// @}

  /// @name Lookup
  /// @{
  [[nodiscard]] iterator find(const key_type &key);
  [[nodiscard]] const_iterator find(const key_type &key) const;
  [[nodiscard]] bool contains(const key_type &key) const;
  [[nodiscard]] mapped_type &at(const key_type &key);
  [[nodiscard]] const mapped_type &at(const key_type &key) const;
  /// @}

  /// @name Modifiers
  /// @{
  template <typename... Args>
  std::pair<iterator, bool> try_emplace(const key_type &key, Args &&...args);
  template <typename... Args>
  std::pair<iterator, bool> try_emplace(key_type &&key, Args &&...args);
  std::pair<iterator, bool> insert(const value_type &value);
  std::pair<iterator, bool> insert(value_type &&value);
  template <typename M>
  std::pair<iterator, bool> insert_or_assign(const key_type &key, M &&obj);
  template <typename M>
  std::pair<iterator, bool> insert_or_assign(key_type &&key, M &&obj);
  mapped_type &operator[](const key_type &key);
  mapped_type &operator[](key_type &&key);
  size_type erase(const key_type &key);
  void clear() noexcept;
  void swap(VectorDictionary &other) noexcept(
      std::is_nothrow_swappable_v<key_equal>);
  /// @}

  /// @name Observers
  /// @{
  [[nodiscard]] key_equal key_eq() const
      noexcept(std::is_nothrow_copy_constructible_v<key_equal>);
  /// @}

private:
  template <typename KeyArg, typename... Args>
  iterator append(KeyArg &&key, Args &&...args);

  std::vector<value_type> entries_;
  key_equal equal_{};
};

template <typename Key, typename Value, typename KeyEqual>
[[nodiscard]] bool
operator==(const VectorDictionary<Key, Value, KeyEqual> &lhs,
           const VectorDictionary<Key, Value, KeyEqual> &rhs);

template <typename Key, typename Value, typename KeyEqual>
[[nodiscard]] bool
operator!=(const VectorDictionary<Key, Value, KeyEqual> &lhs,
           const VectorDictionary<Key, Value, KeyEqual> &rhs);

template <typename Key, typename Value, typename KeyEqual>
void swap(VectorDictionary<Key, Value, KeyEqual> &lhs,
          VectorDictionary<Key, Value, KeyEqual>
              &rhs) noexcept(noexcept(lhs.swap(rhs)));

} // namespace adt

#include "vector_dictionary.tpp"

#endif // ADT_VECTOR_DICTIONARY_HPP
