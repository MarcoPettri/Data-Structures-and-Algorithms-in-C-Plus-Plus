/**
 * @file unsorted_priority_queue.hpp
 * @brief Priority queue implemented on top of an *unsorted* linked list
 * (interface).
 *
 * @details
 * New elements are simply appended to a @c std::list in arrival order; the list
 * is never sorted. Insertion is therefore trivial, and the price is paid when
 * the highest-priority element is requested: it must be found by a linear scan.
 *
 * | Operation            | Complexity |
 * |----------------------|------------|
 * | insert / emplace     | O(1)       |
 * | min                  | O(n)       |
 * | removeMin            | O(n)       |
 * | size / empty / clear | O(1) / O(1) / O(n) |
 *
 * For contrast, a *sorted* list gives O(n) insert and O(1) min/removeMin, and a
 * binary heap gives O(log n) for insert and removeMin. Inserting n elements and
 * then calling removeMin n times on this class is selection sort: O(n^2).
 *
 * @par Ordering convention
 * This is a **min**-priority queue in the textbook sense: for a comparator
 * @c comp, @c min() returns an element @c e such that no other element @c x
 * satisfies <tt>comp(x, e)</tt>. With the default @c std::less<T> that is the
 * smallest element. Pass @c std::greater<T> to get a max-priority queue. (Note
 * that this is the *opposite* of @c std::priority_queue, which pops the
 * "largest" element.)
 *
 * @par Ties
 * Among elements that compare equivalent, the one inserted earliest is returned
 * first (FIFO), because elements are appended and the scan keeps the first
 * minimum it sees.
 *
 */
#ifndef DS_UNSORTED_PRIORITY_QUEUE_HPP
#define DS_UNSORTED_PRIORITY_QUEUE_HPP

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <list>

namespace ds {

/**
 * @brief Priority queue backed by an unsorted @c std::list.
 *
 * @tparam T       Element type. Must be move- or copy-constructible; the range
 *                 constructors additionally require copy-constructibility.
 * @tparam Compare Strict weak ordering on @p T. Must be callable on const
 *                 objects. Defaults to @c std::less<T>.
 *
 * @par Exception safety
 * Operations that throw (@c min, @c removeMin on an empty queue) leave the
 * queue unchanged. @c removeMin returns @c void so that a throwing copy of the
 * removed element can never lose it.
 *
 * Not thread-safe.
 */
template <typename T, typename Compare = std::less<T>>
class UnsortedPriorityQueue {
public:
  using value_type = T;          ///< Element type.
  using size_type = std::size_t; ///< Type used for sizes.
  using value_compare = Compare; ///< Comparator type.

  /// @brief Create an empty queue with a value-initialised comparator. O(1).
  UnsortedPriorityQueue();

  /// @brief Create an empty queue that orders elements with @p comp. O(1).
  explicit UnsortedPriorityQueue(Compare comp);

  /**
   * @brief Create a queue holding a copy of every element in [first, last).
   * @param first Start of the input range.
   * @param last  One past the end of the input range.
   * @param comp  Comparator to use.
   * @par Complexity O(n) in the length of the range (no ordering work is done).
   */
  template <typename InputIt, typename = typename std::iterator_traits<
                                  InputIt>::iterator_category>
  UnsortedPriorityQueue(InputIt first, InputIt last, Compare comp = Compare());

  /**
   * @brief Create a queue holding the elements of @p init.
   * @par Complexity O(n).
   */
  UnsortedPriorityQueue(std::initializer_list<T> init,
                        Compare comp = Compare());

  /// @brief Number of elements in the queue. O(1).
  [[nodiscard]] size_type size() const noexcept { return items_.size(); }

  /// @brief True if the queue holds no elements. O(1).
  [[nodiscard]] bool empty() const noexcept { return items_.empty(); }

  /// @brief The comparator in use.
  [[nodiscard]] value_compare value_comp() const { return comp_; }

  /**
   * @brief Add a copy of @p value to the queue.
   * @par Complexity O(1); performs no comparisons.
   */
  void insert(const T &value);

  /**
   * @brief Add @p value to the queue by moving it in.
   * @par Complexity O(1); performs no comparisons.
   */
  void insert(T &&value);

  /**
   * @brief Construct a new element in place from @p args.
   * @par Complexity O(1); performs no comparisons.
   */
  template <typename... Args> void emplace(Args &&...args);

  /**
   * @brief Highest-priority element (the "minimum" under the comparator).
   * @return Reference to the element; valid until the queue is next modified.
   * @throws std::out_of_range if the queue is empty.
   * @par Complexity O(n): exactly n - 1 comparisons.
   */
  [[nodiscard]] const T &min() const;

  /**
   * @brief Remove the element that min() would return.
   * @throws std::out_of_range if the queue is empty.
   * @par Complexity O(n): n - 1 comparisons to find it, then O(1) to unlink it.
   */
  void removeMin();

  /// @brief Remove every element. O(n).
  void clear() noexcept;

private:
  using ConstIterator = typename std::list<T>::const_iterator;

  /**
   * @brief Linear scan for the first element that no later element beats.
   * @throws std::out_of_range if the queue is empty.
   */
  ConstIterator findMin() const;

  Compare comp_;       ///< Priority ordering.
  std::list<T> items_; ///< Elements in arrival order (deliberately unsorted).
};

} // namespace ds

// Template definitions (implementation part of this header).
#include "unsorted_priority_queue.tpp"

#endif // DS_UNSORTED_PRIORITY_QUEUE_HPP