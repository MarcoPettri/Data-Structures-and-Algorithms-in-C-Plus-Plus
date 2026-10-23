/**
 * @file adaptable_priority_queue.hpp
 * @brief Heap-based adaptable priority queue with locator-based operations.
 *
 * @details
 * A plain priority queue only lets you look at and remove its minimum. An
 * *adaptable* priority queue also lets you reach into the middle: every
 * `insert()` returns a **locator**, a handle to that entry, and the locator can
 * later be used to remove the entry, change its key, or change its value,
 * without searching for it.
 *
 * The queue is an array-backed binary heap of entries. Each entry records its
 * own current position in the array and updates it whenever a swap moves it, so
 * a locator can jump straight to the entry's slot and repair the heap with a
 * single sift.
 *
 * | Operation                        | Complexity |
 * |----------------------------------|------------|
 * | insert(key, value)               | O(log n)   |
 * | min()                            | O(1)       |
 * | removeMin()                      | O(log n)   |
 * | remove(locator)                  | O(log n)   |
 * | replaceKey(locator, key)         | O(log n)   |
 * | replaceValue(locator, value)     | O(1)       |
 * | contains(locator), size, empty   | O(1)       |
 * | clear / destructor               | O(n)       |
 *
 * @par Ordering convention
 * This is a **min**-priority queue: `min()` returns an entry whose key no other
 * key is @c Compare-less than. With the default @c std::less<Key> that is the
 * smallest key; pass @c std::greater<Key> for a max-priority queue. The order
 * of entries with equivalent keys is unspecified.
 *
 * @par Locator lifetime
 * A Locator shares ownership of its entry, so it is always safe to copy, store
 * and inspect. It is *valid* only while its entry is in a queue; after the
 * entry is removed, or the queue is cleared or destroyed, Locator::valid()
 * returns false, but key() and value() still return the entry's last contents.
 * Passing a null, stale, or foreign (other queue's) locator to a queue
 * operation throws @c std::invalid_argument and changes nothing.
 *
 * @par Requirements
 * @c Compare must be a strict weak ordering, callable on const objects, and
 * must not throw (if it did, a sift could be abandoned half-way and leave the
 * heap order violated).
 *
 * @par File layout
 * The class template is split into interface (.hpp) and implementation (.tpp).
 */
#ifndef DS_ADAPTABLE_PRIORITY_QUEUE_HPP
#define DS_ADAPTABLE_PRIORITY_QUEUE_HPP

#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace ds {

/**
 * @brief Binary-heap priority queue whose entries can be addressed by locators.
 *
 * @tparam Key     Priority type, ordered by @p Compare.
 * @tparam Value   Payload type; never inspected by the queue.
 * @tparam Compare Strict weak ordering on @p Key. Defaults to @c
 * std::less<Key>.
 *
 * @par Copying and moving
 * Copying is disabled: a copy could not give the existing locators a meaning.
 * Moving transfers all entries, and existing locators follow them to the new
 * queue. A moved-from queue may only be destroyed or assigned to.
 *
 * Not thread-safe.
 */
template <typename Key, typename Value, typename Compare = std::less<Key>>
class AdaptablePriorityQueue {
private:
  struct Node;

public:
  using key_type = Key;          ///< Priority type.
  using mapped_type = Value;     ///< Payload type.
  using key_compare = Compare;   ///< Comparator type.
  using size_type = std::size_t; ///< Type used for sizes.

  /**
   * @brief Handle to one entry, returned by insert(), min() and removeMin().
   *
   * Cheap to copy (it is a shared pointer). Two locators compare equal iff they
   * refer to the same entry. A default-constructed locator is *null*.
   */
  class Locator {
  public:
    /// @brief Create a null locator (refers to no entry).
    Locator() noexcept = default;

    /// @brief True iff the entry is currently stored in a queue. O(1).
    [[nodiscard]] bool valid() const noexcept {
      return node_ != nullptr && node_->owner != nullptr;
    }

    /**
     * @brief The entry's key. Change it only through
     * AdaptablePriorityQueue::replaceKey().
     * @throws std::invalid_argument if the locator is null.
     */
    [[nodiscard]] const Key &key() const { return checked().key; }

    /**
     * @brief The entry's value, mutable. Changing it never affects ordering.
     * @throws std::invalid_argument if the locator is null.
     */
    [[nodiscard]] Value &value() const { return checked().value; }

    /// @brief True iff both locators refer to the same entry (or are both
    /// null).
    friend bool operator==(const Locator &a, const Locator &b) noexcept {
      return a.node_ == b.node_;
    }

    /// @brief Negation of operator==.
    friend bool operator!=(const Locator &a, const Locator &b) noexcept {
      return !(a == b);
    }

  private:
    friend class AdaptablePriorityQueue;

    explicit Locator(std::shared_ptr<Node> node) noexcept;

    Node &checked() const;

    std::shared_ptr<Node> node_;
  };

  /// @brief Create an empty queue with a value-initialised comparator. O(1).
  AdaptablePriorityQueue();

  /// @brief Create an empty queue ordered by @p comp. O(1).
  explicit AdaptablePriorityQueue(Compare comp);

  AdaptablePriorityQueue(const AdaptablePriorityQueue &) = delete;
  AdaptablePriorityQueue &operator=(const AdaptablePriorityQueue &) = delete;

  /// @brief Move construction; locators of @p other now refer to this queue.
  /// O(1).
  AdaptablePriorityQueue(AdaptablePriorityQueue &&) = default;

  /**
   * @brief Move assignment. Entries previously held by this queue are detached
   *        (their locators become invalid); locators of @p other follow its
   * entries.
   * @par Complexity O(n) in the number of entries this queue held before.
   */
  AdaptablePriorityQueue &operator=(AdaptablePriorityQueue &&other);

  /// @brief Detaches all entries so that outstanding locators report invalid.
  /// O(n).
  ~AdaptablePriorityQueue();

  /// @brief Number of entries. O(1).
  [[nodiscard]] size_type size() const noexcept;

  /// @brief True if the queue holds no entries. O(1).
  [[nodiscard]] bool empty() const noexcept;

  /// @brief The comparator in use.
  [[nodiscard]] key_compare key_comp() const;

  /**
   * @brief Add an entry.
   * @return Locator for the new entry.
   * @par Complexity O(log n).
   */
  Locator insert(Key key, Value value);

  /**
   * @brief Entry with the minimum key.
   * @throws std::out_of_range if the queue is empty.
   * @par Complexity O(1).
   */
  [[nodiscard]] Locator min() const;

  /**
   * @brief Remove the entry with the minimum key.
   * @return Locator for the removed entry; it is no longer valid() but still
   *         exposes the entry's key and value.
   * @throws std::out_of_range if the queue is empty.
   * @par Complexity O(log n).
   */
  Locator removeMin();

  /**
   * @brief Remove the entry that @p loc refers to.
   * @throws std::invalid_argument if @p loc is null, stale, or from another
   * queue.
   * @par Complexity O(log n).
   */
  void remove(const Locator &loc);

  /**
   * @brief Give the entry a new key and restore the heap order.
   * @return The entry's previous key.
   * @throws std::invalid_argument if @p loc is null, stale, or from another
   * queue.
   * @par Complexity O(log n).
   */
  Key replaceKey(const Locator &loc, Key newKey);

  /**
   * @brief Give the entry a new value.
   * @return The entry's previous value.
   * @throws std::invalid_argument if @p loc is null, stale, or from another
   * queue.
   * @par Complexity O(1).
   */
  Value replaceValue(const Locator &loc, Value newValue);

  /// @brief True iff @p loc refers to an entry currently stored in *this*
  /// queue. O(1).
  [[nodiscard]] bool contains(const Locator &loc) const noexcept;

  /// @brief Remove every entry; their locators become invalid. O(n).
  void clear() noexcept;

  /**
   * @brief Verify the heap order and the entries' recorded positions.
   * @note Intended for tests and debugging.
   * @par Complexity O(n).
   */
  [[nodiscard]] bool checkInvariants() const;

private:
  /// A heap entry. Shared between the heap and any Locators that refer to it.
  struct Node {
    Node(Key k, Value v, std::size_t position, const void *ownerTag)
        : key(std::move(k)), value(std::move(v)), index(position),
          owner(ownerTag) {}

    Key key;     ///< Priority of the entry.
    Value value; ///< Payload of the entry.
    std::size_t
        index; ///< Position in the heap array (meaningful while attached).
    const void
        *owner; ///< Identity tag of the owning queue; nullptr once detached.
  };

  using NodePtr = std::shared_ptr<Node>;

  bool higher(const NodePtr &a, const NodePtr &b) const;
  void requireNonEmpty() const;
  size_type indexOf(const Locator &loc) const;
  void swapNodes(size_type i, size_type j);
  size_type siftUp(size_type i);
  void siftDown(size_type i);
  NodePtr removeAt(size_type i);
  void detachAll() noexcept;

  Compare comp_;              ///< Priority ordering.
  std::vector<NodePtr> heap_; ///< Array-backed binary heap of entries.
  std::unique_ptr<char>
      tag_; ///< Heap-allocated identity token; stable across moves.
};

} // namespace ds

#include "adaptable_priority_queue.tpp"

#endif // DS_ADAPTABLE_PRIORITY_QUEUE_HPP