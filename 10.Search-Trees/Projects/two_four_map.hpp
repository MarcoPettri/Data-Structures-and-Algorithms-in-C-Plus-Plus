/**
 * @file two_four_map.hpp
 * @brief Ordered map ADT implemented with a (2,4) tree (interface).
 *
 * A (2,4) tree is a multiway search tree (a B-tree of minimum degree 2) where
 * every internal node has 2, 3 or 4 children (1, 2 or 3 entries) and all leaves
 * are on the same level. Its height is therefore always between log4(n+1) and
 * log2(n+1), and every operation runs in O(log n) time.
 *
 * The implementation lives in TwoFourMap.tpp, which is included at the bottom
 * of this header (the usual pattern for class templates).
 *
 * Complexity summary (n = number of entries):
 *  - find / put / insert / erase / operator[] / at ........ O(log n)
 *  - first / last / ceiling / floor / higher / lower ...... O(log n)
 *  - iterator increment / decrement ........................ O(log n) worst,
 *                                                            O(1) amortized
 *  - size / empty .......................................... O(1)
 *  - clear / destructor / copy ............................. O(n)
 *
 * @standard C++17
 */

#ifndef DS_TWO_FOUR_MAP_HPP
#define DS_TWO_FOUR_MAP_HPP

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <new>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

namespace ds {

/**
 * @brief Ordered map (sorted associative container with unique keys) backed by
 *        a (2,4) tree.
 *
 * Besides the classical map operations (find, put, erase, size, empty), the
 * class implements the *ordered map ADT* operations: firstEntry, lastEntry,
 * ceilingEntry, floorEntry, higherEntry, lowerEntry and subMap.
 *
 * ### Entries and iterators
 * Entries are stored inline inside the nodes (up to three per node), which
 * makes the tree compact and cache friendly, but means that entries are
 * *relocated* when nodes split, merge or shift. Consequently:
 *  - Dereferencing an iterator yields a *proxy pair* `std::pair<const Key&,
 * T&>`
 *    (`std::pair<const Key&, const T&>` for const iterators), returned by
 * value. `it->first` / `it->second` and structured bindings
 *    (`for (auto [k, v] : map)`) work as usual; `v` can be modified, `k`
 * cannot.
 *  - **Inserting a new key or erasing any key invalidates all iterators and all
 *    references to keys/values.** Overwriting the value of an existing key
 *    (put, operator[] on an existing key, insert of a duplicate) never changes
 *    the structure and invalidates nothing.
 *
 * ### Requirements
 *  - @p Key and @p T must be nothrow move constructible (checked at compile
 *    time). Neither needs to be default constructible. @p Key must be copy
 *    constructible; @p T must be copy constructible only if the map is copied.
 *
 * @tparam Key     Key type.
 * @tparam T       Mapped value type.
 * @tparam Compare Strict weak ordering on keys (default: std::less<Key>).
 */
template <typename Key, typename T, typename Compare = std::less<Key>>
class TwoFourMap {
  static_assert(std::is_nothrow_move_constructible_v<Key>,
                "TwoFourMap requires a nothrow move constructible Key");
  static_assert(std::is_nothrow_move_constructible_v<T>,
                "TwoFourMap requires a nothrow move constructible mapped type");

private:
  struct Node;
  template <bool IsConst> class Iter;

  /** @brief Position of an entry: node plus index inside the node. */
  struct Pos {
    Node *node = nullptr;
    int idx = 0;
  };

public:
  /// @name Member types
  /// @{
  using key_type = Key;
  using mapped_type = T;
  using value_type = std::pair<Key, T>;
  using key_compare = Compare;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using iterator = Iter<false>;
  using const_iterator = Iter<true>;
  /// @}

  /// @name Construction, assignment, destruction
  /// @{

  /** @brief Constructs an empty map. @param comp Key comparator. */
  explicit TwoFourMap(const Compare &comp = Compare()) : comp_(comp) {}

  /**
   * @brief Constructs a map from an initializer list.
   * @param init Entries to insert; for duplicate keys the *last* wins.
   * @param comp Key comparator.
   */
  TwoFourMap(std::initializer_list<value_type> init,
             const Compare &comp = Compare());

  /** @brief Deep copy. O(n). */
  TwoFourMap(const TwoFourMap &other);

  /** @brief Move constructor. O(1); @p other is left empty. */
  TwoFourMap(TwoFourMap &&other) noexcept;

  /** @brief Copy/move assignment (copy-and-swap). */
  TwoFourMap &operator=(TwoFourMap other) noexcept;

  /** @brief Destroys the map and all its entries. O(n). */
  ~TwoFourMap();

  /** @brief Exchanges contents with @p other. O(1). */
  void swap(TwoFourMap &other) noexcept;
  /// @}

  /// @name Capacity
  /// @{
  /** @brief Number of entries. O(1). */
  size_type size() const noexcept { return size_; }

  /** @brief True if the map holds no entries. O(1). */
  bool empty() const noexcept { return size_ == 0; }

  /** @brief Number of tree levels (0 when empty). O(log n). */
  int height() const noexcept;
  /// @}

  /// @name Iterators (in ascending key order)
  /// @{
  iterator begin() noexcept { return iterator(minPos(root_), this); }
  const_iterator begin() const noexcept {
    return const_iterator(minPos(root_), this);
  }
  const_iterator cbegin() const noexcept { return begin(); }
  iterator end() noexcept { return iterator(Pos{}, this); }
  const_iterator end() const noexcept { return const_iterator(Pos{}, this); }
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
   * @return Reference to the mapped value (invalidated by later insert/erase).
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
   *
   * @p key may safely refer to a key stored inside this map.
   * @return true if an entry was removed. O(log n).
   */
  bool erase(const Key &key);

  /**
   * @brief Removes the entry at @p pos (must be dereferenceable).
   *
   * Requires copying one key (the successor's) to re-locate the next entry.
   * @return Iterator to the entry following the removed one. O(log n).
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
   * @brief Verifies every structural invariant: node fill (1..3 entries),
   *        uniform leaf depth, strict key order across the whole tree, child
   *        and parent links, and size. Intended for tests/debugging.
   * @return true if the tree is a valid (2,4) tree. O(n).
   */
  bool isValid() const;

private:
  /**
   * @brief Tree node holding 1..3 entries and (if internal) n+1 children.
   *
   * Entries live in raw aligned storage; slots [0, n) are constructed.
   * A node is a leaf iff child[0] == nullptr.
   */
  struct Node {
    alignas(value_type) unsigned char slots[3][sizeof(value_type)];
    Node *child[4] = {nullptr, nullptr, nullptr, nullptr};
    Node *parent = nullptr;
    int n = 0;

    explicit Node(Node *p = nullptr) noexcept : parent(p) {}
    Node(const Node &) = delete;
    Node &operator=(const Node &) = delete;
    ~Node() {
      for (int i = 0; i < n; ++i)
        std::destroy_at(&entry(i));
    }

    bool isLeaf() const noexcept { return child[0] == nullptr; }

    value_type &entry(int i) noexcept {
      return *std::launder(reinterpret_cast<value_type *>(slots[i]));
    }
    const value_type &entry(int i) const noexcept {
      return *std::launder(reinterpret_cast<const value_type *>(slots[i]));
    }
    const Key &key(int i) const noexcept { return entry(i).first; }

    /** @brief Moves slot @p src into the unconstructed slot @p dst. */
    void relocate(int dst, int src) noexcept {
      ::new (static_cast<void *>(slots[dst])) value_type(std::move(entry(src)));
      std::destroy_at(&entry(src));
    }

    /** @brief Appends an entry (node must have < 3 entries). */
    void pushBack(value_type &&e) noexcept {
      ::new (static_cast<void *>(slots[n])) value_type(std::move(e));
      ++n;
    }

    /** @brief Inserts an entry at index @p i, shifting later entries right. */
    void insertAt(int i, value_type &&e) noexcept {
      for (int j = n - 1; j >= i; --j)
        relocate(j + 1, j);
      ::new (static_cast<void *>(slots[i])) value_type(std::move(e));
      ++n;
    }

    /** @brief Removes and returns entry @p i, shifting later entries left. */
    value_type removeAt(int i) noexcept {
      value_type out(std::move(entry(i)));
      std::destroy_at(&entry(i));
      for (int j = i + 1; j < n; ++j)
        relocate(j - 1, j);
      --n;
      return out;
    }

    /** @brief Removes and returns the last entry. */
    value_type popBack() noexcept { return removeAt(n - 1); }

    /** @brief Destroys entry @p i and constructs @p e in its place. */
    void replaceAt(int i, value_type &&e) noexcept {
      std::destroy_at(&entry(i));
      ::new (static_cast<void *>(slots[i])) value_type(std::move(e));
    }
  };

  /** @brief Bidirectional iterator (const and non-const flavours). */
  template <bool IsConst> class Iter {
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = typename TwoFourMap::value_type;
    using difference_type = std::ptrdiff_t;
    using mapped_ref = std::conditional_t<IsConst, const T &, T &>;
    /** @brief Proxy reference: (const key&, mapped&), returned by value. */
    using reference = std::pair<const Key &, mapped_ref>;
    /** @brief Arrow proxy so that `it->first` / `it->second` work. */
    struct pointer {
      reference ref;
      reference *operator->() noexcept { return &ref; }
    };

    Iter() noexcept = default;

    /** @brief iterator -> const_iterator conversion. */
    template <bool Other, typename = std::enable_if_t<IsConst && !Other>>
    Iter(const Iter<Other> &other) noexcept
        : pos_(other.pos_), tree_(other.tree_) {}

    /** @brief Proxy reference to the current entry. */
    reference operator*() const noexcept {
      auto &e = pos_.node->entry(pos_.idx);
      return reference(e.first, e.second);
    }
    pointer operator->() const noexcept { return pointer{**this}; }

    /** @brief Key of the current entry. */
    const Key &key() const noexcept { return pos_.node->key(pos_.idx); }
    /** @brief Mapped value of the current entry. */
    mapped_ref value() const noexcept {
      return pos_.node->entry(pos_.idx).second;
    }

    Iter &operator++() noexcept {
      pos_ = successorPos(pos_);
      return *this;
    }
    Iter operator++(int) noexcept {
      Iter t(*this);
      ++*this;
      return t;
    }
    Iter &operator--() noexcept {
      pos_ = pos_.node ? predecessorPos(pos_) : maxPos(tree_->root_);
      return *this;
    }
    Iter operator--(int) noexcept {
      Iter t(*this);
      --*this;
      return t;
    }

    friend bool operator==(const Iter &a, const Iter &b) noexcept {
      return a.pos_.node == b.pos_.node && a.pos_.idx == b.pos_.idx;
    }
    friend bool operator!=(const Iter &a, const Iter &b) noexcept {
      return !(a == b);
    }

  private:
    friend class TwoFourMap;
    template <bool> friend class Iter;

    Iter(Pos p, const TwoFourMap *t) noexcept : pos_(p), tree_(t) {}

    Pos pos_{};
    const TwoFourMap *tree_ = nullptr;
  };

  /// @name Static navigation helpers
  /// @{
  static Pos minPos(Node *n) noexcept;
  static Pos maxPos(Node *n) noexcept;
  static Pos successorPos(Pos p) noexcept;
  static Pos predecessorPos(Pos p) noexcept;
  static int indexInParent(const Node *parent, const Node *child) noexcept;
  /// @}

  /// @name Lookup helpers (return Pos{} when nothing qualifies)
  /// @{
  int lowerIndex(const Node *x,
                 const Key &key) const; ///< first i with !(key(i) < key)
  int upperIndex(const Node *x,
                 const Key &key) const; ///< first i with key < key(i)
  Pos findPos(const Key &key) const;
  Pos ceilingPos(const Key &key) const;
  Pos floorPos(const Key &key) const;
  Pos higherPos(const Key &key) const;
  Pos lowerPos(const Key &key) const;
  /// @}

  /** @brief Inserts if absent (top-down, splitting full nodes on the way). */
  template <typename... Args>
  std::pair<Pos, bool> insertImpl(const Key &key, Args &&...args);

  void splitChild(Node *parent, int i);
  void eraseAt(Pos p) noexcept;
  void fixUnderflow(Node *x) noexcept;

  static Node *cloneSubtree(const Node *src, Node *parent);
  static void destroySubtree(Node *n) noexcept;
  bool validateNode(const Node *x, const Node *parent, int depth,
                    int &leafDepth, size_type &count, const Key *lo,
                    const Key *hi) const;

  Node *root_ = nullptr;
  size_type size_ = 0;
  Compare comp_;
};

/** @brief Non-member swap. */
template <typename Key, typename T, typename Compare>
void swap(TwoFourMap<Key, T, Compare> &a,
          TwoFourMap<Key, T, Compare> &b) noexcept {
  a.swap(b);
}

} // namespace ds

#include "two_four_map.tpp"

#endif // DS_TWO_FOUR_MAP_HPP