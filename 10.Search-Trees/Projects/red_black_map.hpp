/**
 * @file red_black_map.hpp
 * @brief Ordered map ADT implemented with a red-black tree (interface).
 *
 * A red-black tree is a self-balancing binary search tree in which every node
 * is colored red or black such that:
 *  1. the root is black;
 *  2. a red node never has a red child;
 *  3. every path from a node to a null descendant crosses the same number of
 *     black nodes.
 * These rules bound the height by 2*log2(n+1) and allow insertion with at most
 * two rotations and erasure with at most three, so all operations are O(log n).
 *
 * The implementation lives in RedBlackMap.tpp, which is included at the bottom
 * of this header (the usual pattern for class templates).
 *
 * Complexity summary (n = number of entries):
 *  - find / put / insert / erase / operator[] / at ........ O(log n)
 *  - first / last / ceiling / floor / higher / lower ...... O(log n)
 *  - iterator increment / decrement ........................ O(log n) worst,
 *                                                            O(1) amortized
 *  - size / empty .......................................... O(1)
 *  - clear / destructor / copy / height .................... O(n)
 *
 * @standard C++17
 */

#ifndef DS_RED_BLACK_MAP_HPP
#define DS_RED_BLACK_MAP_HPP

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

namespace ds {

/**
 * @brief Ordered map (sorted associative container with unique keys) backed by
 *        a red-black tree.
 *
 * Besides the classical map operations (find, put, erase, size, empty), the
 * class implements the *ordered map ADT* operations: firstEntry, lastEntry,
 * ceilingEntry, floorEntry, higherEntry, lowerEntry and subMap.
 *
 * Entries are exposed as `std::pair<const Key, T>`; the key cannot be changed
 * through an iterator, the mapped value can (for non-const iterators).
 *
 * Nodes are never relocated or copied (erasure relinks nodes), so iterators and
 * references stay valid until the entry they refer to is erased.
 *
 * @tparam Key     Key type. Must be copy-constructible.
 * @tparam T       Mapped value type.
 * @tparam Compare Strict weak ordering on keys (default: std::less<Key>).
 */
template <typename Key, typename T, typename Compare = std::less<Key>>
class RedBlackMap {
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
  /// @}

  /// @name Construction, assignment, destruction
  /// @{

  /** @brief Constructs an empty map. @param comp Key comparator. */
  explicit RedBlackMap(const Compare &comp = Compare()) : comp_(comp) {}

  /**
   * @brief Constructs a map from an initializer list.
   * @param init Entries to insert; for duplicate keys the *last* wins.
   * @param comp Key comparator.
   */
  RedBlackMap(std::initializer_list<value_type> init,
              const Compare &comp = Compare());

  /** @brief Deep copy. O(n). */
  RedBlackMap(const RedBlackMap &other);

  /** @brief Move constructor. O(1); @p other is left empty. */
  RedBlackMap(RedBlackMap &&other) noexcept;

  /** @brief Copy/move assignment (copy-and-swap). */
  RedBlackMap &operator=(RedBlackMap other) noexcept;

  /** @brief Destroys the map and all its entries. O(n). */
  ~RedBlackMap();

  /** @brief Exchanges contents with @p other. O(1). */
  void swap(RedBlackMap &other) noexcept;
  /// @}

  /// @name Capacity
  /// @{
  /** @brief Number of entries. O(1). */
  size_type size() const noexcept { return size_; }

  /** @brief True if the map holds no entries. O(1). */
  bool empty() const noexcept { return size_ == 0; }

  /** @brief Height of the tree in nodes (0 when empty). O(n). */
  int height() const noexcept;
  /// @}

  /// @name Iterators (in ascending key order)
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

  /// @name Map ADT operations
  /// @{

  /**
   * @brief Looks up @p key.
   * @return Iterator to the entry, or end() if absent. O(log n).
   */
  iterator find(const Key &key);
  /** @copydoc find(const Key&) */
  const_iterator find(const Key &key) const;

  /** @brief True if @p key is present. O(log n). */
  bool contains(const Key &key) const;

  /**
   * @brief Bounds-checked access.
   * @throws std::out_of_range if @p key is absent.
   */
  T &at(const Key &key);
  /** @copydoc at(const Key&) */
  const T &at(const Key &key) const;

  /**
   * @brief Access or default-insert. Inserts a value-initialized T if absent.
   * @return Reference to the mapped value.
   */
  T &operator[](const Key &key);

  /**
   * @brief Inserts or overwrites the value for @p key (ADT `put`).
   * @return Pair of (iterator to the entry, true if a new entry was inserted
   *         and false if an existing value was overwritten).
   */
  template <typename V>
  std::pair<iterator, bool> put(const Key &key, V &&value);

  /**
   * @brief Inserts (@p key, @p value) only if @p key is absent.
   * @return Pair of (iterator to the entry, whether insertion took place).
   */
  template <typename V>
  std::pair<iterator, bool> insert(const Key &key, V &&value);

  /** @brief Inserts @p entry if its key is absent. */
  std::pair<iterator, bool> insert(const value_type &entry) {
    return insert(entry.first, entry.second);
  }

  /**
   * @brief Removes the entry with @p key (ADT `erase`).
   * @return true if an entry was removed. O(log n).
   */
  bool erase(const Key &key);

  /**
   * @brief Removes the entry at @p pos (must be dereferenceable).
   * @return Iterator to the entry following the removed one.
   */
  iterator erase(const_iterator pos);

  /** @brief Removes all entries. O(n). */
  void clear() noexcept;
  /// @}

  /// @name Ordered map ADT operations (return end() when no entry qualifies)
  /// @{

  /** @brief Entry with the smallest key. O(log n). */
  iterator firstEntry() noexcept { return begin(); }
  const_iterator firstEntry() const noexcept { return begin(); }

  /** @brief Entry with the greatest key. O(log n). */
  iterator lastEntry() noexcept;
  const_iterator lastEntry() const noexcept;

  /** @brief Entry with the least key >= @p key. O(log n). */
  iterator ceilingEntry(const Key &key);
  const_iterator ceilingEntry(const Key &key) const;

  /** @brief Entry with the greatest key <= @p key. O(log n). */
  iterator floorEntry(const Key &key);
  const_iterator floorEntry(const Key &key) const;

  /** @brief Entry with the least key > @p key. O(log n). */
  iterator higherEntry(const Key &key);
  const_iterator higherEntry(const Key &key) const;

  /** @brief Entry with the greatest key < @p key. O(log n). */
  iterator lowerEntry(const Key &key);
  const_iterator lowerEntry(const Key &key) const;

  /**
   * @brief Half-open range of entries with @p k1 <= key < @p k2.
   * @return Pair [first, last) of iterators; empty if k1 >= k2 under Compare.
   */
  std::pair<iterator, iterator> subMap(const Key &k1, const Key &k2);
  std::pair<const_iterator, const_iterator> subMap(const Key &k1,
                                                   const Key &k2) const;

  /** @brief Alias of ceilingEntry (std::map naming). */
  iterator lower_bound(const Key &key) { return ceilingEntry(key); }
  const_iterator lower_bound(const Key &key) const { return ceilingEntry(key); }
  /** @brief Alias of higherEntry (std::map naming). */
  iterator upper_bound(const Key &key) { return higherEntry(key); }
  const_iterator upper_bound(const Key &key) const { return higherEntry(key); }
  /// @}

  /** @brief Returns a copy of the key comparator. */
  key_compare key_comp() const { return comp_; }

  /**
   * @brief Verifies every red-black invariant (black root, no red-red edge,
   *        uniform black height) plus BST order, parent links and size.
   *        Intended for tests/debugging.
   * @return true if the tree is a valid red-black tree. O(n).
   */
  bool isValid() const;

private:
  /** @brief Tree node. Stores the entry, links and the color bit. */
  struct Node {
    value_type kv;
    Node *left = nullptr;
    Node *right = nullptr;
    Node *parent = nullptr;
    bool red = true; ///< new nodes start red

    template <typename... Args>
    explicit Node(Node *p, const Key &k, Args &&...args)
        : kv(std::piecewise_construct, std::forward_as_tuple(k),
             std::forward_as_tuple(std::forward<Args>(args)...)),
          parent(p) {}
  };

  /** @brief Bidirectional iterator (const and non-const flavours). */
  template <bool IsConst> class Iter {
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = typename RedBlackMap::value_type;
    using difference_type = std::ptrdiff_t;
    using pointer =
        std::conditional_t<IsConst, const value_type *, value_type *>;
    using reference =
        std::conditional_t<IsConst, const value_type &, value_type &>;

    Iter() noexcept = default;

    /** @brief iterator -> const_iterator conversion. */
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
    friend class RedBlackMap;
    template <bool> friend class Iter;

    Iter(Node *n, const RedBlackMap *t) noexcept : node_(n), tree_(t) {}

    Node *node_ = nullptr;
    const RedBlackMap *tree_ = nullptr;
  };

  /// @name Static navigation helpers
  /// @{
  static Node *minNode(Node *n) noexcept;
  static Node *maxNode(Node *n) noexcept;
  static Node *successor(Node *n) noexcept;
  static Node *predecessor(Node *n) noexcept;
  /// @}

  /// @name Red-black helpers
  /// @{
  static bool isRed(const Node *n) noexcept {
    return n && n->red;
  } ///< null leaves are black
  void replaceChild(Node *parent, Node *oldChild, Node *newChild) noexcept;
  void transplant(Node *u, Node *v) noexcept;
  void rotateLeft(Node *x) noexcept;
  void rotateRight(Node *x) noexcept;
  void insertFixup(Node *z) noexcept;
  void eraseNode(Node *z) noexcept;
  void eraseFixup(Node *x, Node *xParent) noexcept;
  /// @}

  /// @name Lookup helpers (return nullptr when nothing qualifies)
  /// @{
  Node *findNode(const Key &key) const;
  Node *ceilingNode(const Key &key) const;
  Node *floorNode(const Key &key) const;
  Node *higherNode(const Key &key) const;
  Node *lowerNode(const Key &key) const;
  /// @}

  /** @brief Inserts if absent; never consumes @p args when the key exists. */
  template <typename... Args>
  std::pair<Node *, bool> insertImpl(const Key &key, Args &&...args);

  static Node *cloneSubtree(const Node *src, Node *parent);
  static void destroySubtree(Node *n) noexcept;
  static int heightOf(const Node *n) noexcept;
  int validateSubtree(const Node *n, const Node *parent,
                      size_type &count) const;

  Node *root_ = nullptr;
  size_type size_ = 0;
  Compare comp_;
};

/** @brief Non-member swap. */
template <typename Key, typename T, typename Compare>
void swap(RedBlackMap<Key, T, Compare> &a,
          RedBlackMap<Key, T, Compare> &b) noexcept {
  a.swap(b);
}

} // namespace ds

#include "red_black_map.tpp"

#endif // DS_RED_BLACK_MAP_HPP