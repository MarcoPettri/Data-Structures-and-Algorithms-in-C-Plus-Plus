/**
 * @file ab_tree_map.hpp
 * @brief Ordered map interface implemented by a runtime-parameterized (a,b)-tree.
 * @standard C++17
 */

#ifndef DS_AB_TREE_MAP_HPP
#define DS_AB_TREE_MAP_HPP

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace ds {

/**
 * @brief Ordered map implemented as a balanced (a,b)-tree.
 *
 * Non-root internal nodes have between a and b children, keys within each
 * node are sorted, and all leaves have the same depth. The root may be a leaf;
 * otherwise it has between 2 and b children. The constructor requires
 * a >= 2 and b >= 2a-1.
 *
 * @tparam Key Copyable key type ordered by Compare.
 * @tparam T Mapped value type.
 * @tparam Compare Strict weak ordering for keys.
 */
template <typename Key, typename T, typename Compare = std::less<Key>>
class ABTreeMap {
public:
  using key_type = Key;
  using mapped_type = T;
  using value_type = std::pair<Key, T>;
  using size_type = std::size_t;

  /**
   * @brief Construct an empty map with branching bounds a and b.
   * @param a Minimum child count for non-root internal nodes.
   * @param b Maximum child count for internal nodes.
   * @param compare Strict weak ordering for keys.
   * @throws std::invalid_argument if a < 2 or b < 2a-1.
   * @section complexity Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  explicit ABTreeMap(size_type a, size_type b,
                     const Compare &compare = Compare());

  /** @brief Number of stored entries. @return Entry count. @section complexity Time: $O(1)$; space: $O(1)$. */
  [[nodiscard]] size_type size() const noexcept;
  /** @brief Whether the map is empty. @return True iff no entries are stored. @section complexity Time: $O(1)$; space: $O(1)$. */
  [[nodiscard]] bool empty() const noexcept;
  /** @brief Configured minimum child count. @return The constructor's a value. @section complexity Time: $O(1)$; space: $O(1)$. */
  [[nodiscard]] size_type min_children() const noexcept;
  /** @brief Configured maximum child count. @return The constructor's b value. @section complexity Time: $O(1)$; space: $O(1)$. */
  [[nodiscard]] size_type max_children() const noexcept;

  /**
   * @brief Find a key and return a copy of its entry, or nullopt if absent.
   * @param key Key to find.
   * @return The key-value pair if present.
   * @section complexity Time: $O(b \log_a(n+1))$; auxiliary space: $O(1)$.
   * n is entry count; a and b are the configured branching bounds.
   */
  [[nodiscard]] std::optional<value_type> find(const Key &key) const;
  /**
   * @brief Test whether a key exists.
   * @param key Key to test.
   * @return True iff the key is present.
   * @section complexity Time: $O(b \log_a(n+1))$; auxiliary space: $O(1)$.
   * n is entry count; a and b are the configured branching bounds.
   */
  [[nodiscard]] bool contains(const Key &key) const;

  /**
   * @brief Insert or replace a mapped value.
   * @tparam V Mapped value argument type.
   * @param key Key to add or update.
   * @param value Mapped value to store.
   * @return Entry after the operation and true iff a new key was inserted.
   * @throws Exceptions from key/value construction, comparison, or assignment.
   * @section complexity Time: $O(b \log_a(n+1))$; auxiliary space: $O(h)$.
   * n is entry count, h is tree height, and a,b are branching bounds.
   */
  template <typename V>
  std::pair<value_type, bool> put(const Key &key, V &&value);

  /**
   * @brief Insert only if a key is absent.
   * @tparam V Mapped value argument type.
   * @param key Key to add.
   * @param value Mapped value to store.
   * @return Entry and true iff insertion occurred; existing value is preserved.
   * @throws Exceptions from key/value construction, comparison, or allocation.
   * @section complexity Time: $O(b \log_a(n+1))$; auxiliary space: $O(h)$.
   * n is entry count, h is tree height, and a,b are branching bounds.
   */
  template <typename V>
  std::pair<value_type, bool> insert(const Key &key, V &&value);

  /**
   * @brief Access a mapped value, inserting a value-initialized one if absent.
   * @param key Key to access.
   * @return Mutable mapped value for key.
   * @throws Exceptions from value construction, comparison, or allocation.
   * @section complexity Time: $O(b \log_a(n+1))$; auxiliary space: $O(h)$ on insertion.
   * n is entry count, h is tree height, and a,b are branching bounds.
   */
  T &operator[](const Key &key);

  /**
   * @brief Access an existing mapped value.
   * @param key Key to access.
   * @return Mutable mapped value for key.
   * @throws std::out_of_range if key is absent.
   * @section complexity Time: $O(b \log_a(n+1))$; auxiliary space: $O(1)$.
   * n is entry count; a and b are branching bounds.
   */
  T &at(const Key &key);
  /** @copydoc at(const Key&) */
  const T &at(const Key &key) const;

  /**
   * @brief Remove an entry by key.
   * @param key Key to erase.
   * @return True iff an entry was removed.
   * @throws Exceptions from comparison or key/value assignment.
   * @section complexity Time: $O(b \log_a(n+1))$; auxiliary space: $O(h)$.
   * n is entry count, h is tree height, and a,b are branching bounds.
   */
  bool erase(const Key &key);
  /** @brief Remove all entries. @section complexity Time: $O(n)$; space: $O(h)$ destruction stack, where n is entry count and h is height. */
  void clear() noexcept;

  /** @brief Least entry, or nullopt if empty. @return Smallest key-value pair. @section complexity Time: $O(h)$; space: $O(1)$, where h is tree height. */
  [[nodiscard]] std::optional<value_type> firstEntry() const;
  /** @brief Greatest entry, or nullopt if empty. @return Largest key-value pair. @section complexity Time: $O(h)$; space: $O(1)$, where h is tree height. */
  [[nodiscard]] std::optional<value_type> lastEntry() const;
  /**
   * @brief Least entry whose key is at least key.
   * @param key Inclusive lower bound.
   * @return Qualifying entry or nullopt.
   * @section complexity Time: $O(bh)$; auxiliary space: $O(1)$, where h is height.
   */
  [[nodiscard]] std::optional<value_type> ceilingEntry(const Key &key) const;
  /** @brief Greatest entry whose key is at most key. @param key Inclusive upper bound.
   * @return Qualifying entry or nullopt. @section complexity Time: $O(bh)$; space: $O(1)$, where h is height.
   */
  [[nodiscard]] std::optional<value_type> floorEntry(const Key &key) const;
  /** @brief Least entry whose key is greater than key. @param key Exclusive lower bound.
   * @return Qualifying entry or nullopt. @section complexity Time: $O(bh)$; space: $O(1)$, where h is height.
   */
  [[nodiscard]] std::optional<value_type> higherEntry(const Key &key) const;
  /** @brief Greatest entry whose key is less than key. @param key Exclusive upper bound.
   * @return Qualifying entry or nullopt. @section complexity Time: $O(bh)$; space: $O(1)$, where h is height.
   */
  [[nodiscard]] std::optional<value_type> lowerEntry(const Key &key) const;

  /**
   * @brief Return entries in the half-open interval [low, high), in key order.
   * @param low Inclusive lower bound.
   * @param high Exclusive upper bound.
   * @return Ordered copies of qualifying entries.
   * @section complexity Time: $O(n)$; auxiliary space: $O(k+h)$.
   * n is entry count, k is result count, and h is tree height.
   */
  [[nodiscard]] std::vector<value_type> subMap(const Key &low,
                                               const Key &high) const;

  /**
   * @brief Return all entries in ascending key order.
   * @return Copies of every key-value pair.
   * @section complexity Time: $O(n)$; auxiliary space: $O(n+h)$.
   * n is entry count and h is tree height.
   */
  [[nodiscard]] std::vector<value_type> to_vector() const;

  /**
   * @brief Verify key ordering, occupancy, child counts, and equal leaf depth.
   * @return True iff all (a,b)-tree invariants hold.
   * @section complexity Time: $O(n)$; auxiliary space: $O(h)$.
   * n is entry count and h is tree height.
   */
  [[nodiscard]] bool validate() const;

private:
  struct Node {
    std::vector<value_type> entries;
    std::vector<std::unique_ptr<Node>> children;
    [[nodiscard]] bool leaf() const noexcept;
  };
  struct Split {
    value_type promoted;
    std::unique_ptr<Node> right;
  };

  size_type a_;
  size_type b_;
  Compare compare_;
  std::unique_ptr<Node> root_;
  size_type size_ = 0;

  bool equal(const Key &x, const Key &y) const;
  size_type lower_bound(const Node &node, const Key &key) const;
  Node *locate(const Key &key);
  const Node *locate(const Key &key) const;
  std::optional<Split> insert_recursive(Node *node, const value_type &entry);
  void insert_new(const Key &key, T value);
  void erase_from(Node *node, const Key &key);
  void fix_underflow(Node *parent, size_type child_index);
  std::optional<value_type> extreme(bool first) const;
  std::optional<value_type> bound_entry(const Key &key, bool predecessor,
                                        bool strict) const;
  void collect(const Node *node, std::vector<value_type> &out) const;
  void collect_range(const Node *node, const Key &low, const Key &high,
                     std::vector<value_type> &out) const;
  bool validate_node(const Node *node, bool is_root, const Key *low,
                     const Key *high, size_type depth,
                     std::optional<size_type> &leaf_depth,
                     size_type &count) const;
};

} // namespace ds

#include "ab_tree_map.tpp"

#endif // DS_AB_TREE_MAP_HPP
