/**
 * @file BSTMultiMap.hpp
 * @brief Dictionary ADT backed by an unbalanced binary search tree that
 *        supports duplicate keys (interface).
 *
 *
 * Complexity summary (n = number of entries, h = tree height):
 *  - insert ................................................... O(h)
 *  - find / findAll / erase ................................. O(h)
 *  - entrySetPreorder / Inorder / Postorder ................. O(n)
 *  - size / empty ............................................ O(1)
 *  - clear / destructor / copy .............................. O(n)
 *  h = O(log n) on average for random data; O(n) worst-case (sorted input).
 *
 * @note This tree is intentionally *unbalanced*
 *
 */

#ifndef DS_BST_MULTI_MAP_HPP
#define DS_BST_MULTI_MAP_HPP

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

namespace ds {

/**
 * @brief Dictionary ADT backed by a plain BST; duplicate keys are allowed.
 *
 * Entries are exposed as `std::pair<const Key, T>`.  The bidirectional
 * iterator visits nodes in ascending (inorder) key order.
 *
 * @tparam Key     Key type.  Must be copy-constructible.
 * @tparam T       Mapped value type.
 * @tparam Compare Strict weak ordering on keys (default: `std::less<Key>`).
 *                 For duplicate keys, `comp(a, b) == false && comp(b, a) ==
 *                 false` holds; new duplicates are placed to the right.
 */
template <typename Key, typename T, typename Compare = std::less<Key>>
class BSTMultiMap {
private:
  struct Node;
  template <bool IsConst> class Iter;

public:
  /// @name Member types
  /// @{
  using key_type = Key;
  using mapped_type = T;
  using value_type = std::pair<const Key, T>;
  using key_compare = Compare;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = value_type &;
  using const_reference = const value_type &;
  using iterator = Iter<false>;
  using const_iterator = Iter<true>;
  using entry_set = std::vector<const value_type *>;
  /// @}

  /// @name Construction, assignment, destruction
  /// @{

  /** @brief Constructs an empty dictionary. @param comp Key comparator. */
  explicit BSTMultiMap(const Compare &comp = Compare()) : comp_(comp) {}

  /**
   * @brief Constructs a dictionary from an initializer list (all entries
   *        are inserted, including duplicates).
   * @param init Entries to insert.
   * @param comp Key comparator.
   */
  BSTMultiMap(std::initializer_list<value_type> init,
              const Compare &comp = Compare());

  /** @brief Deep copy.  O(n). */
  BSTMultiMap(const BSTMultiMap &other);

  /** @brief Move constructor.  O(1); @p other is left empty. */
  BSTMultiMap(BSTMultiMap &&other) noexcept;

  /** @brief Copy/move assignment (copy-and-swap idiom). */
  BSTMultiMap &operator=(BSTMultiMap other) noexcept;

  /** @brief Destroys the dictionary and frees all nodes.  O(n). */
  ~BSTMultiMap();

  /** @brief Exchanges contents with @p other in O(1). */
  void swap(BSTMultiMap &other) noexcept;
  /// @}

  /// @name Capacity
  /// @{
  /** @brief Number of entries (counts duplicates individually).  O(1). */
  size_type size() const noexcept { return size_; }

  /** @brief True if the dictionary holds no entries.  O(1). */
  bool empty() const noexcept { return size_ == 0; }

  /** @brief Height of the tree (0 when empty).  O(n). */
  int height() const noexcept;
  /// @}

  /// @name Iterators (inorder / ascending key order)
  /// @{
  iterator begin() noexcept { return iterator(minNode(root_), this); }
  const_iterator begin() const noexcept {
    return const_iterator(minNode(root_), this);
  }
  const_iterator cbegin() const noexcept { return begin(); }

  iterator end() noexcept { return iterator(nullptr, this); }
  const_iterator end() const noexcept { return const_iterator(nullptr, this); }
  const_iterator cend() const noexcept { return end(); }
  /// @}

  /// @name Dictionary ADT operations
  /// @{

  /**
   * @brief Always inserts (@p key, @p value) as a new entry, even if an
   *        equal key already exists.
   * @return Iterator to the newly created entry.  O(h).
   */
  template <typename V> iterator insert(const Key &key, V &&value);

  /** @brief Inserts @p entry unconditionally. */
  iterator insert(const value_type &entry) {
    return insert(entry.first, entry.second);
  }

  /**
   * @brief Finds the first (leftmost inorder) entry whose key equals @p key.
   * @return Iterator to the entry, or end() if none.  O(h).
   */
  iterator find(const Key &key);
  /** @copydoc find(const Key&) */
  const_iterator find(const Key &key) const;

  /** @brief True if at least one entry with @p key exists.  O(h). */
  bool contains(const Key &key) const;

  /**
   * @brief Collects all entries whose key equals @p key.
   * @return Vector of pointers (inorder); empty if no such key.  O(h + k).
   */
  entry_set findAll(const Key &key) const;

  /**
   * @brief Removes the entry at iterator @p pos (must be dereferenceable).
   * @return Iterator to the entry following the removed one.  O(h).
   */
  iterator erase(const_iterator pos);

  /**
   * @brief Removes the first entry found with @p key.
   * @return true if an entry was removed.  O(h).
   */
  bool erase(const Key &key);

  /**
   * @brief Removes all entries with @p key.
   * @return Number of entries removed.  O(h + k).
   */
  size_type eraseAll(const Key &key);

  /** @brief Removes all entries.  O(n). */
  void clear() noexcept;
  /// @}

  /// @name Traversal collectors (P-10.11 requirement)
  /// @{

  /**
   * @brief Returns pointers to all entries in preorder (root, left, right).
   * @return `entry_set` (std::vector of `const value_type *`) in preorder.
   *         The pointed-to objects are owned by the tree; use before any
   *         mutation.  O(n).
   */
  entry_set entrySetPreorder() const;

  /**
   * @brief Returns pointers to all entries in inorder (left, root, right).
   * @details For a BST with duplicate keys this yields a non-decreasing key
   *          sequence.
   * @return `entry_set` in inorder.  O(n).
   */
  entry_set entrySetInorder() const;

  /**
   * @brief Returns pointers to all entries in postorder (left, right, root).
   * @return `entry_set` in postorder.  O(n).
   */
  entry_set entrySetPostorder() const;
  /// @}

  /**
   * @brief Verifies BST structural invariants (key ordering, parent links,
   *        size).  Intended for tests and debugging.
   * @return true if the tree is a valid BST.  O(n).
   */
  bool isValid() const;

private:
  // =========================================================================
  // Internal node
  // =========================================================================

  /** @brief Tree node storing one entry and tree links. */
  struct Node {
    value_type kv; ///< The stored key-value pair.
    Node *left = nullptr;
    Node *right = nullptr;
    Node *parent = nullptr;

    /** @brief Construct with forwarded mapped-value arguments. */
    template <typename... Args>
    explicit Node(Node *p, const Key &k, Args &&...args)
        : kv(std::piecewise_construct, std::forward_as_tuple(k),
             std::forward_as_tuple(std::forward<Args>(args)...)),
          parent(p) {}
  };

  // =========================================================================
  // Bidirectional iterator
  // =========================================================================

  /**
   * @brief Bidirectional iterator over BSTMultiMap entries.
   * @tparam IsConst Selects const vs. non-const dereference.
   */
  template <bool IsConst> class Iter {
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = typename BSTMultiMap::value_type;
    using difference_type = std::ptrdiff_t;
    using pointer =
        std::conditional_t<IsConst, const value_type *, value_type *>;
    using reference =
        std::conditional_t<IsConst, const value_type &, value_type &>;

    Iter() noexcept = default;

    /** @brief Non-const to const conversion. */
    template <bool Other, typename = std::enable_if_t<IsConst && !Other>>
    Iter(const Iter<Other> &other) noexcept
        : node_(other.node_), tree_(other.tree_) {}

    reference operator*() const noexcept { return node_->kv; }
    pointer operator->() const noexcept { return &node_->kv; }

    Iter &operator++() noexcept {
      node_ = successor(node_);
      return *this;
    }
    Iter operator++(int) noexcept {
      Iter t(*this);
      ++*this;
      return t;
    }

    Iter &operator--() noexcept {
      node_ = node_ ? predecessor(node_) : maxNode(tree_->root_);
      return *this;
    }
    Iter operator--(int) noexcept {
      Iter t(*this);
      --*this;
      return t;
    }

    friend bool operator==(const Iter &a, const Iter &b) noexcept {
      return a.node_ == b.node_;
    }
    friend bool operator!=(const Iter &a, const Iter &b) noexcept {
      return a.node_ != b.node_;
    }

  private:
    friend class BSTMultiMap;
    template <bool> friend class Iter;

    Iter(Node *n, const BSTMultiMap *t) noexcept : node_(n), tree_(t) {}

    Node *node_ = nullptr;
    const BSTMultiMap *tree_ = nullptr;
  };

  // =========================================================================
  // Static navigation helpers
  // =========================================================================
  /// @{
  static Node *minNode(Node *n) noexcept;
  static Node *maxNode(Node *n) noexcept;
  static Node *successor(Node *n) noexcept;
  static Node *predecessor(Node *n) noexcept;
  /// @}

  // =========================================================================
  // Internal helpers
  // =========================================================================

  /** @brief Core BST insert (always creates a new node). */
  template <typename... Args> Node *insertNode(const Key &key, Args &&...args);

  /** @brief Removes a single node @p z from the tree. */
  void eraseNode(Node *z) noexcept;

  /** @brief Replaces @p oldChild of @p parent with @p newChild. */
  void replaceChild(Node *parent, Node *oldChild, Node *newChild) noexcept;

  /** @brief Finds the leftmost node with key equal to @p key. */
  Node *findNode(const Key &key) const;

  // Traversal helpers (recursive).
  static void collectPreorder(const Node *n, entry_set &out);
  static void collectInorder(const Node *n, entry_set &out);
  static void collectPostorder(const Node *n, entry_set &out);

  // Memory helpers.
  static Node *cloneSubtree(const Node *src, Node *parent);
  static void destroySubtree(Node *n) noexcept;
  static int heightOf(const Node *n) noexcept;

  // Validation helper.
  bool validateSubtree(const Node *n, const Node *parent,
                       size_type &count) const;

  // =========================================================================
  // Data members
  // =========================================================================
  Node *root_ = nullptr;
  size_type size_ = 0;
  Compare comp_;
};

/** @brief Non-member swap. */
template <typename Key, typename T, typename Compare>
void swap(BSTMultiMap<Key, T, Compare> &a,
          BSTMultiMap<Key, T, Compare> &b) noexcept {
  a.swap(b);
}

} // namespace ds

#include "BSTMultiMap.tpp"

#endif // DS_BST_MULTI_MAP_HPP
