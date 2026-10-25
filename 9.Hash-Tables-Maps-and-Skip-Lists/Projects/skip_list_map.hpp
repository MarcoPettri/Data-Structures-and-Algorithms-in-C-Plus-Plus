/**
 * @file skip_list_map.hpp
 * @brief Ordered map ADT implemented with a skip list.
 *
 * Entries are iterated in key-comparator order. Search, insertion and
 * erasure take expected O(log n) time and O(n) worst-case time.
 */
#ifndef ADT_SKIP_LIST_MAP_HPP
#define ADT_SKIP_LIST_MAP_HPP

#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <optional>
#include <random>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace adt {

/**
 * @brief Associative map with unique keys, implemented as a skip list.
 *
 * @tparam Key Key type, ordered by @p KeyCompare.
 * @tparam Value Mapped type.
 * @tparam KeyCompare Strict-weak-ordering comparator for keys.
 *
 * Iterators traverse entries in ascending comparator order. Insertion does
 * not invalidate existing iterators, pointers or references; erasure
 * invalidates only those referring to the erased entry.
 */
template <typename Key, typename Value, typename KeyCompare = std::less<Key>>
class SkipListMap {
  struct Node;

public:
  using key_type = Key;
  using mapped_type = Value;
  using value_type = std::pair<const Key, Value>;
  using key_compare = KeyCompare;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = value_type &;
  using const_reference = const value_type &;
  using pointer = value_type *;
  using const_pointer = const value_type *;

  template <bool IsConst> class BasicIterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::pair<const Key, Value>;
    using difference_type = std::ptrdiff_t;
    using pointer =
        std::conditional_t<IsConst, const value_type *, value_type *>;
    using reference =
        std::conditional_t<IsConst, const value_type &, value_type &>;

    BasicIterator() noexcept = default;

    template <bool OtherConst,
              typename = std::enable_if_t<IsConst && !OtherConst>>
    BasicIterator(const BasicIterator<OtherConst> &other) noexcept
        : node_(other.node_) {}

    [[nodiscard]] reference operator*() const noexcept { return *node_->entry; }
    [[nodiscard]] pointer operator->() const noexcept {
      return std::addressof(*node_->entry);
    }

    BasicIterator &operator++() noexcept {
      node_ = node_->forward[0];
      return *this;
    }
    BasicIterator operator++(int) noexcept {
      BasicIterator previous{*this};
      ++(*this);
      return previous;
    }

    [[nodiscard]] friend bool operator==(const BasicIterator &lhs,
                                         const BasicIterator &rhs) noexcept {
      return lhs.node_ == rhs.node_;
    }
    [[nodiscard]] friend bool operator!=(const BasicIterator &lhs,
                                         const BasicIterator &rhs) noexcept {
      return !(lhs == rhs);
    }

  private:
    friend class SkipListMap;
    template <bool> friend class BasicIterator;

    explicit BasicIterator(Node *node) noexcept : node_(node) {}
    Node *node_ = nullptr;
  };

  using iterator = BasicIterator<false>;
  using const_iterator = BasicIterator<true>;

  /// Constructs an empty map with default skip-list settings.
  SkipListMap();

  /**
   * @brief Constructs an empty map with custom skip-list settings.
   * @param maxLevel Maximum tower height; must be greater than zero.
   * @param probability Promotion probability; strictly between zero and one.
   * @param compare Key ordering predicate.
   * @param seed Seed for the pseudo-random height generator.
   * @throws std::invalid_argument if maxLevel or probability is invalid.
   */
  explicit SkipListMap(size_type maxLevel, double probability = 0.5,
                       key_compare compare = key_compare(),
                       std::uint32_t seed = 1337);

  /**
   * @brief Constructs from entries in initializer-list order.
   * Duplicate keys update the mapped value; the first key object is retained.
   */
  SkipListMap(std::initializer_list<value_type> init,
              key_compare compare = key_compare(), size_type maxLevel = 32,
              double probability = 0.5, std::uint32_t seed = 1337);

  /// Deep-copies entries and their skip-list levels.
  SkipListMap(const SkipListMap &other);
  /// Transfers entries and leaves @p other empty and reusable.
  SkipListMap(SkipListMap &&other);
  /// Copy assignment with the strong exception guarantee.
  SkipListMap &operator=(const SkipListMap &other);
  /// Move assignment; @p other is left empty and reusable.
  SkipListMap &operator=(SkipListMap &&other);
  /// Releases all entries.
  ~SkipListMap();

  [[nodiscard]] size_type size() const noexcept;
  [[nodiscard]] bool empty() const noexcept;

  [[nodiscard]] iterator begin() noexcept;
  [[nodiscard]] const_iterator begin() const noexcept;
  [[nodiscard]] const_iterator cbegin() const noexcept;
  [[nodiscard]] iterator end() noexcept;
  [[nodiscard]] const_iterator end() const noexcept;
  [[nodiscard]] const_iterator cend() const noexcept;

  /// Returns the entry for key, or end() if it is absent.
  [[nodiscard]] iterator find(const key_type &key);
  [[nodiscard]] const_iterator find(const key_type &key) const;
  [[nodiscard]] bool contains(const key_type &key) const;

  /// Accesses a mapped value; throws std::out_of_range if the key is absent.
  [[nodiscard]] mapped_type &at(const key_type &key);
  [[nodiscard]] const mapped_type &at(const key_type &key) const;

  /// Returns the first entry not ordered before key.
  [[nodiscard]] iterator lower_bound(const key_type &key);
  [[nodiscard]] const_iterator lower_bound(const key_type &key) const;
  /// Returns the first entry ordered after key.
  [[nodiscard]] iterator upper_bound(const key_type &key);
  [[nodiscard]] const_iterator upper_bound(const key_type &key) const;
  /// Returns the greatest entry strictly before key, or end().
  [[nodiscard]] iterator predecessor(const key_type &key);
  [[nodiscard]] const_iterator predecessor(const key_type &key) const;
  /// Returns the least entry strictly after key, or end().
  [[nodiscard]] iterator successor(const key_type &key);
  [[nodiscard]] const_iterator successor(const key_type &key) const;

  /// Returns a copy of the minimum/maximum entry, or nullopt when empty.
  [[nodiscard]] std::optional<value_type> min() const;
  [[nodiscard]] std::optional<value_type> max() const;
  [[nodiscard]] key_compare key_comp() const
      noexcept(std::is_nothrow_copy_constructible_v<key_compare>);

  /**
   * @brief Inserts only if key is absent; arguments are not consumed on
   * failure.
   * @return Iterator to the existing or inserted entry, and whether inserted.
   */
  template <typename... Args>
  std::pair<iterator, bool> try_emplace(const key_type &key, Args &&...args);
  template <typename... Args>
  std::pair<iterator, bool> try_emplace(key_type &&key, Args &&...args);

  /// Inserts or assigns; returns iterator and true iff a new entry was added.
  template <typename M>
  std::pair<iterator, bool> insert_or_assign(const key_type &key, M &&value);
  template <typename M>
  std::pair<iterator, bool> insert_or_assign(key_type &&key, M &&value);

  /// Inserts without overwriting a duplicate key.
  std::pair<iterator, bool> insert(const value_type &entry);
  std::pair<iterator, bool> insert(value_type &&entry);

  /// Returns the mapped value, inserting a value-initialized one if absent.
  mapped_type &operator[](const key_type &key);
  mapped_type &operator[](key_type &&key);

  /// Removes key, returning 1 if it existed and 0 otherwise.
  size_type erase(const key_type &key);
  /// Removes all entries while retaining the map configuration.
  void clear() noexcept;
  /// Exchanges map contents in constant time.
  void swap(SkipListMap &other) noexcept(
      std::is_nothrow_swappable_v<key_compare>);

private:
  struct Node {
    explicit Node(size_type height) : forward(height, nullptr) {}

    template <typename KeyArg, typename... Args>
    Node(size_type height, KeyArg &&key, Args &&...args)
        : entry(std::in_place, std::piecewise_construct,
                std::forward_as_tuple(std::forward<KeyArg>(key)),
                std::forward_as_tuple(std::forward<Args>(args)...)),
          forward(height, nullptr) {}

    std::optional<value_type> entry;
    std::vector<Node *> forward;
  };

  [[nodiscard]] Node *lowerBoundNode(const key_type &key) const;
  [[nodiscard]] Node *findNode(const key_type &key) const;
  [[nodiscard]] std::vector<Node *> predecessors(const key_type &key) const;
  [[nodiscard]] size_type randomHeight();
  template <typename KeyArg, typename... Args>
  Node *insertNode(const std::vector<Node *> &update, KeyArg &&key,
                   Args &&...args);
  void copyFrom(const SkipListMap &other);
  void swapContents(SkipListMap &other) noexcept(
      std::is_nothrow_swappable_v<key_compare>);

  size_type max_level_;
  double probability_;
  size_type size_ = 0;
  Node head_;
  key_compare compare_{};
  std::mt19937 random_engine_;
  std::uniform_real_distribution<double> distribution_{0.0, 1.0};
};

template <typename Key, typename Value, typename KeyCompare>
[[nodiscard]] bool operator==(const SkipListMap<Key, Value, KeyCompare> &lhs,
                              const SkipListMap<Key, Value, KeyCompare> &rhs);
template <typename Key, typename Value, typename KeyCompare>
[[nodiscard]] bool operator!=(const SkipListMap<Key, Value, KeyCompare> &lhs,
                              const SkipListMap<Key, Value, KeyCompare> &rhs);
template <typename Key, typename Value, typename KeyCompare>
void swap(SkipListMap<Key, Value, KeyCompare> &lhs,
          SkipListMap<Key, Value, KeyCompare> &rhs) noexcept(
    noexcept(lhs.swap(rhs)));

} // namespace adt

#include "skip_list_map.tpp"

#endif // ADT_SKIP_LIST_MAP_HPP
