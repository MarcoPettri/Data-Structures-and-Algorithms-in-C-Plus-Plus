/**
 * @file linked_list_dictionary.hpp
 * @brief Dictionary (associative array) ADT backed by a singly linked list.
 * @par Design notes
 *  - Entries are kept in **insertion order**; iteration is deterministic.
 *  - A tail pointer makes appending O(1) once the key is known to be absent.
 *  - Nodes are owned by `std::unique_ptr`. Destruction/clearing is iterative, so
 *    very long lists cannot overflow the stack.
 *  - Insertion never invalidates iterators, pointers or references. Erasure
 *    invalidates only those referring to the erased entry.
 *  - The API intentionally mirrors `std::map` / `std::unordered_map`
 *    (`try_emplace`, `insert_or_assign`, `find`, `at`, `operator[]`, ...).
 *
 * @par Complexity summary (n = size())
 *  | Operation                                   | Complexity |
 *  |---------------------------------------------|------------|
 *  | size, empty, begin, end, swap, move         | O(1)       |
 *  | find, contains, at, operator[], erase       | O(n)       |
 *  | insert, try_emplace, insert_or_assign       | O(n) (lookup) + O(1) (append) |
 *  | clear, copy, destruction                    | O(n)       |
 *  | operator==                                  | O(n^2)     |
 *
 * A linked list cannot beat O(n) lookup. Prefer a hash table or balanced tree for
 * large collections; this container fits small dictionaries and teaching use.
 */
#ifndef ADT_LINKED_LIST_DICTIONARY_HPP
#define ADT_LINKED_LIST_DICTIONARY_HPP

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

namespace adt {

/**
 * @brief Dictionary ADT mapping unique keys to values, implemented as a singly
 * linked list.
 *
 * @tparam Key      Key type. Must be copy- or move-constructible (depending on
 * the insertion overload used) and destructible.
 * @tparam Value    Mapped type. Must be destructible; further requirements
 * depend on the member function used (e.g. default-constructible for
 *                  operator[], copy-constructible for copying the dictionary).
 * @tparam KeyEqual Equivalence predicate. Must be invocable as
 *                  `bool(const Key&, const Key&)` on a `const` object.
 *
 * @invariant `size()` equals the number of nodes reachable from the head.
 * @invariant The tail pointer addresses the last node, or is null iff the
 * dictionary is empty.
 * @invariant No two stored keys are equivalent under `KeyEqual`.
 *
 * @par Thread safety
 * Concurrent const access is safe; any concurrent modification requires
 * external synchronisation.
 */
template <typename Key, typename Value, typename KeyEqual = std::equal_to<Key>>
class LinkedListDictionary {
  static_assert(
      std::is_invocable_r_v<bool, const KeyEqual &, const Key &, const Key &>,
      "KeyEqual must be callable as bool(const Key&, const Key&) on a const "
      "object");

  struct Node; ///< Internal list node (defined below, after the public
               ///< interface).

  /// True when comparing two keys cannot throw; drives conditional `noexcept`.
  static constexpr bool kNothrowKeyEqual =
      std::is_nothrow_invocable_v<const KeyEqual &, const Key &, const Key &>;

public:
  /// @name Member types
  /// @{
  using key_type = Key;
  using mapped_type = Value;
  using value_type = std::pair<const Key, Value>;
  using key_equal = KeyEqual;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = value_type &;
  using const_reference = const value_type &;
  using pointer = value_type *;
  using const_pointer = const value_type *;
  /// @}

  /**
   * @brief Forward iterator over the entries of the dictionary.
   * @tparam IsConst When true, dereferencing yields `const value_type&`.
   *
   * The key is always immutable; the mapped value is mutable through the
   * non-const iterator. A non-const iterator converts implicitly to a const
   * one.
   */
  template <bool IsConst> class BasicIterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::pair<const Key, Value>;
    using difference_type = std::ptrdiff_t;
    using pointer =
        std::conditional_t<IsConst, const value_type *, value_type *>;
    using reference =
        std::conditional_t<IsConst, const value_type &, value_type &>;

    /** @brief Constructs a singular iterator (equal to `end()` of any
     * dictionary only if value-initialised). */
    BasicIterator() noexcept = default;

    /** @brief Converting constructor: iterator -> const_iterator. */
    template <bool OtherConst,
              typename = std::enable_if_t<IsConst && !OtherConst>>
    BasicIterator(const BasicIterator<OtherConst> &other) noexcept
        : node_(other.node_) {}

    /** @brief Accesses the referenced entry. @pre The iterator is
     * dereferenceable. */
    [[nodiscard]] reference operator*() const noexcept { return node_->entry; }

    /** @brief Accesses a member of the referenced entry. @pre The iterator is
     * dereferenceable. */
    [[nodiscard]] pointer operator->() const noexcept {
      return std::addressof(node_->entry);
    }

    /** @brief Advances to the next entry. @pre The iterator is dereferenceable.
     */
    BasicIterator &operator++() noexcept {
      node_ = node_->next.get();
      return *this;
    }

    /** @brief Post-increment. @pre The iterator is dereferenceable. */
    BasicIterator operator++(int) noexcept {
      BasicIterator previous{*this};
      ++(*this);
      return previous;
    }

    /** @brief Equality: both iterators refer to the same entry (or both are
     * past-the-end). */
    [[nodiscard]] friend bool operator==(const BasicIterator &lhs,
                                         const BasicIterator &rhs) noexcept {
      return lhs.node_ == rhs.node_;
    }

    /** @brief Inequality. */
    [[nodiscard]] friend bool operator!=(const BasicIterator &lhs,
                                         const BasicIterator &rhs) noexcept {
      return lhs.node_ != rhs.node_;
    }

  private:
    friend class LinkedListDictionary;
    template <bool> friend class BasicIterator;

    explicit BasicIterator(Node *node) noexcept : node_(node) {}

    Node *node_ = nullptr;
  };

  using iterator = BasicIterator<false>; ///< Mutable-value forward iterator.
  using const_iterator = BasicIterator<true>; ///< Read-only forward iterator.

  /// @name Construction, copy, move and destruction
  /// @{

  /** @brief Constructs an empty dictionary. */
  LinkedListDictionary() = default;

  /**
   * @brief Constructs an empty dictionary using a custom key-equivalence
   * predicate.
   * @param equal Predicate used to compare keys.
   */
  explicit LinkedListDictionary(key_equal equal) noexcept(
      std::is_nothrow_move_constructible_v<key_equal>);

  /**
   * @brief Constructs a dictionary from a list of entries.
   * @param init  Entries to insert, in order. On duplicate keys the **first**
   * entry wins.
   * @param equal Predicate used to compare keys.
   * @par Complexity O(n^2) for n entries in @p init.
   */
  LinkedListDictionary(std::initializer_list<value_type> init,
                       key_equal equal = key_equal());

  /**
   * @brief Copy constructor (deep copy, insertion order preserved).
   * @param other Dictionary to copy.
   * @par Complexity O(n).
   * @par Exception safety Strong: nothing leaks if an element copy throws.
   */
  LinkedListDictionary(const LinkedListDictionary &other);

  /**
   * @brief Move constructor. Steals the list in O(1).
   * @param other Dictionary to move from; left empty but valid.
   */
  LinkedListDictionary(LinkedListDictionary &&other) noexcept(
      std::is_nothrow_move_constructible_v<key_equal>);

  /**
   * @brief Copy assignment (copy-and-swap).
   * @param other Dictionary to copy.
   * @return `*this`.
   * @par Exception safety Strong.
   */
  LinkedListDictionary &operator=(const LinkedListDictionary &other);

  /**
   * @brief Move assignment. Releases current contents, then steals @p other's
   * list in O(1).
   * @param other Dictionary to move from; left empty but valid.
   * @return `*this`.
   */
  LinkedListDictionary &operator=(LinkedListDictionary &&other) noexcept(
      std::is_nothrow_move_assignable_v<key_equal>);

  /** @brief Destroys the dictionary and all of its entries (iteratively, O(n)).
   */
  ~LinkedListDictionary();
  /// @}

  /// @name Capacity
  /// @{

  /** @brief Returns the number of entries. @par Complexity O(1). */
  [[nodiscard]] size_type size() const noexcept;

  /** @brief Returns true if the dictionary holds no entries. @par Complexity
   * O(1). */
  [[nodiscard]] bool empty() const noexcept;
  /// @}

  /// @name Iterators
  /// Entries are visited in insertion order.
  /// @{
  [[nodiscard]] iterator begin() noexcept;
  [[nodiscard]] const_iterator begin() const noexcept;
  [[nodiscard]] const_iterator cbegin() const noexcept;
  [[nodiscard]] iterator end() noexcept;
  [[nodiscard]] const_iterator end() const noexcept;
  [[nodiscard]] const_iterator cend() const noexcept;
  /// @}

  /// @name Lookup
  /// @{

  /**
   * @brief Finds the entry with the given key.
   * @param key Key to look for.
   * @return Iterator to the entry, or `end()` if absent.
   * @par Complexity O(n).
   */
  [[nodiscard]] iterator find(const key_type &key) noexcept(kNothrowKeyEqual);

  /** @copydoc find(const key_type&) */
  [[nodiscard]] const_iterator find(const key_type &key) const
      noexcept(kNothrowKeyEqual);

  /**
   * @brief Checks whether a key is present.
   * @param key Key to look for.
   * @return true if an entry with an equivalent key exists.
   * @par Complexity O(n).
   */
  [[nodiscard]] bool contains(const key_type &key) const
      noexcept(kNothrowKeyEqual);

  /**
   * @brief Accesses the value mapped to a key, with bounds checking.
   * @param key Key to look for.
   * @return Reference to the mapped value.
   * @throws std::out_of_range if the key is absent.
   * @par Complexity O(n).
   */
  [[nodiscard]] mapped_type &at(const key_type &key);

  /** @copydoc at(const key_type&) */
  [[nodiscard]] const mapped_type &at(const key_type &key) const;
  /// @}

  /// @name Modifiers
  /// @{

  /**
   * @brief Inserts a new entry only if the key is absent; otherwise does
   * nothing.
   *
   * The mapped value is constructed in place from @p args **only if** insertion
   * takes place, so move-only arguments are not consumed on failure.
   *
   * @tparam Args Constructor argument types of the mapped value.
   * @param key  Key to insert (copied).
   * @param args Arguments forwarded to the mapped value's constructor.
   * @return Pair of an iterator to the (new or existing) entry and a flag that
   * is true iff a new entry was inserted.
   * @par Complexity O(n).
   * @par Exception safety Strong.
   */
  template <typename... Args>
  std::pair<iterator, bool> try_emplace(const key_type &key, Args &&...args);

  /**
   * @brief Same as above, but the key is moved only if insertion takes place.
   * @tparam Args Constructor argument types of the mapped value.
   * @param key  Key to insert (moved from on success only).
   * @param args Arguments forwarded to the mapped value's constructor.
   * @return See the `const key_type&` overload.
   */
  template <typename... Args>
  std::pair<iterator, bool> try_emplace(key_type &&key, Args &&...args);

  /**
   * @brief Inserts an entry if its key is absent (existing values are never
   * overwritten).
   * @param value Entry to insert (copied).
   * @return Pair of iterator and insertion flag (see try_emplace()).
   */
  std::pair<iterator, bool> insert(const value_type &value);

  /**
   * @brief Inserts an entry if its key is absent; the mapped value is moved on
   * success.
   * @param value Entry to insert. The (const) key is copied.
   * @return Pair of iterator and insertion flag (see try_emplace()).
   */
  std::pair<iterator, bool> insert(value_type &&value);

  /**
   * @brief Inserts an entry, or assigns to the mapped value if the key already
   * exists.
   * @tparam M Type assignable to (and constructible into) the mapped type.
   * @param key Key to insert or update (copied if inserted).
   * @param obj Value to store.
   * @return Pair of iterator and flag; true iff a new entry was inserted, false
   * if assigned.
   * @par Complexity O(n).
   */
  template <typename M>
  std::pair<iterator, bool> insert_or_assign(const key_type &key, M &&obj);

  /** @brief Same as above, but the key is moved only if a new entry is
   * inserted. */
  template <typename M>
  std::pair<iterator, bool> insert_or_assign(key_type &&key, M &&obj);

  /**
   * @brief Accesses the value mapped to a key, inserting a value-initialised
   * one if absent.
   * @param key Key to look for (copied if inserted).
   * @return Reference to the mapped value.
   * @par Complexity O(n).
   */
  mapped_type &operator[](const key_type &key);

  /** @brief Same as above, but the key is moved only if a new entry is
   * inserted. */
  mapped_type &operator[](key_type &&key);

  /**
   * @brief Removes the entry with the given key, if any.
   * @param key Key to remove.
   * @return Number of removed entries (0 or 1).
   * @par Complexity O(n).
   */
  size_type erase(const key_type &key) noexcept(kNothrowKeyEqual);

  /**
   * @brief Removes all entries.
   * @post `empty() == true`.
   * @par Complexity O(n).
   */
  void clear() noexcept;

  /**
   * @brief Exchanges the contents of two dictionaries in O(1).
   * @param other Dictionary to swap with.
   */
  void swap(LinkedListDictionary &other) noexcept(
      std::is_nothrow_swappable_v<key_equal>);
  /// @}

  /// @name Observers
  /// @{

  /** @brief Returns a copy of the key-equivalence predicate. */
  [[nodiscard]] key_equal key_eq() const
      noexcept(std::is_nothrow_copy_constructible_v<key_equal>);
  /// @}

private:
  /** @brief Singly linked list node owning its successor. */
  struct Node {
    /** @brief Builds the entry in place: @p key -> `entry.first`, @p args ->
     * `entry.second`. */
    template <typename KeyArg, typename... Args>
    explicit Node(KeyArg &&key, Args &&...args)
        : entry(std::piecewise_construct,
                std::forward_as_tuple(std::forward<KeyArg>(key)),
                std::forward_as_tuple(std::forward<Args>(args)...)) {}

    value_type entry; ///< The stored key/value pair.
    std::unique_ptr<Node>
        next; ///< Owning link to the successor (null at the tail).
  };

  /** @brief Linear search; returns the node holding @p key or nullptr. */
  [[nodiscard]] Node *findNode(const key_type &key) const
      noexcept(kNothrowKeyEqual);

  /**
   * @brief Allocates a node and appends it at the tail **without** checking for
   * duplicates.
   * @return Pointer to the new node. Strong exception safety: state is
   * untouched on throw.
   */
  template <typename KeyArg, typename... Args>
  Node *appendNode(KeyArg &&key, Args &&...args);

  std::unique_ptr<Node> head_; ///< Owns the whole chain.
  Node *tail_ = nullptr;       ///< Non-owning pointer to the last node.
  size_type size_ = 0;         ///< Number of nodes.
  key_equal equal_{};          ///< Key-equivalence predicate.
};

/**
 * @brief Compares two dictionaries for equality, ignoring insertion order.
 * @return true iff both hold the same keys mapped to equal values.
 * @par Complexity O(n^2).
 * @relates LinkedListDictionary
 */
template <typename Key, typename Value, typename KeyEqual>
[[nodiscard]] bool
operator==(const LinkedListDictionary<Key, Value, KeyEqual> &lhs,
           const LinkedListDictionary<Key, Value, KeyEqual> &rhs);

/** @brief Negation of operator==. @relates LinkedListDictionary */
template <typename Key, typename Value, typename KeyEqual>
[[nodiscard]] bool
operator!=(const LinkedListDictionary<Key, Value, KeyEqual> &lhs,
           const LinkedListDictionary<Key, Value, KeyEqual> &rhs);

/** @brief Non-member swap enabling ADL `swap(a, b)`. @relates
 * LinkedListDictionary */
template <typename Key, typename Value, typename KeyEqual>
void swap(LinkedListDictionary<Key, Value, KeyEqual> &lhs,
          LinkedListDictionary<Key, Value, KeyEqual>
              &rhs) noexcept(noexcept(lhs.swap(rhs)));

} // namespace adt

#include "linked_list_dictionary.tpp"

#endif // ADT_LINKED_LIST_DICTIONARY_HPP