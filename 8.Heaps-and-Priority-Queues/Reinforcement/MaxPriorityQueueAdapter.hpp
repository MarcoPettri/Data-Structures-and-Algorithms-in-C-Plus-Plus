/**
 * @file MaxPriorityQueueAdapter.hpp
 * @brief Defines the MaxPriorityQueueAdapter class template for C++17.
 */

#pragma once

#include <cstddef>
#include <queue>
#include <utility>

/**
 * @class MaxPriorityQueueAdapter
 * @brief Adapter class that takes a priority queue type P and comparator C
 *        to implement a max-priority queue using operations like removeMax().
 *
 * @tparam T The element type stored in the priority queue.
 * @tparam Container The underlying container type (e.g., std::vector<T>).
 * @tparam C The original comparator type (e.g., std::less<T> or
 * std::greater<T>).
 */
template <typename T, typename Container, typename C>
class MaxPriorityQueueAdapter {
public:
  using value_type = T;
  using size_type = typename Container::size_type;

private:
  /**
   * @struct ReversedComparator
   * @brief Defines the new comparator C' in terms of C by reversing element
   * order.
   */
  struct ReversedComparator {
    C original_comp;

    bool operator()(const T &a, const T &b) const {
      // Reversing C: if original considers a < b, C' considers b < a
      return original_comp(b, a);
    }
  };

  // Underlying standard library priority queue utilizing the reversed
  // comparator C'
  std::priority_queue<T, Container, ReversedComparator> pq_;

public:
  /**
   * @brief Construct a new Max Priority Queue Adapter object.
   * @param c The original comparator instance.
   */
  explicit MaxPriorityQueueAdapter(C c = C{}) : pq_(ReversedComparator{c}) {}

  /**
   * @brief Checks if the priority queue is empty.
   * @return true if empty, false otherwise.
   */
  [[nodiscard]] bool empty() const { return pq_.empty(); }

  /**
   * @brief Returns the number of elements in the priority queue.
   * @return size_type The number of elements.
   */
  [[nodiscard]] size_type size() const { return pq_.size(); }

  /**
   * @brief Inserts a key/value into the priority queue.
   * @param value The value to insert.
   */
  void insert(const T &value) { pq_.push(value); }

  /**
   * @brief Inserts a key/value into the priority queue (move semantics).
   * @param value The rvalue reference to insert.
   */
  void insert(T &&value) { pq_.push(std::move(value)); }

  /**
   * @brief Returns a reference to the element with the maximum key.
   * @return const T& Reference to the maximum element.
   */
  [[nodiscard]] const T &getMax() const { return pq_.top(); }

  /**
   * @brief Removes the element with the maximum key from the priority queue.
   */
  void removeMax() { pq_.pop(); }
};