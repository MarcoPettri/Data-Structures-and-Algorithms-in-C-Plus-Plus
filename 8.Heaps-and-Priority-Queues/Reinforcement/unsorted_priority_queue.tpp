/**
 * @file unsorted_priority_queue.tpp
 * @brief Template definitions for ds::UnsortedPriorityQueue (implementation part).
 *
 * This file is part of @c unsorted_priority_queue.hpp, which includes it after
 * the class declaration. Do not include it directly. Behaviour, complexity and
 * exception guarantees are documented on the declarations in the header.
 */
#ifndef DS_UNSORTED_PRIORITY_QUEUE_TPP
#define DS_UNSORTED_PRIORITY_QUEUE_TPP

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "unsorted_priority_queue.hpp"  // no-op when reached via the header; helps IDEs

namespace ds {

template <typename T, typename Compare>
UnsortedPriorityQueue<T, Compare>::UnsortedPriorityQueue() : comp_() {}

template <typename T, typename Compare>
UnsortedPriorityQueue<T, Compare>::UnsortedPriorityQueue(Compare comp)
    : comp_(std::move(comp)) {}

template <typename T, typename Compare>
template <typename InputIt, typename>
UnsortedPriorityQueue<T, Compare>::UnsortedPriorityQueue(InputIt first, InputIt last,
                                                         Compare comp)
    : comp_(std::move(comp)), items_(first, last) {}

template <typename T, typename Compare>
UnsortedPriorityQueue<T, Compare>::UnsortedPriorityQueue(std::initializer_list<T> init,
                                                         Compare comp)
    : comp_(std::move(comp)), items_(init) {}

template <typename T, typename Compare>
void UnsortedPriorityQueue<T, Compare>::insert(const T& value) {
  items_.push_back(value);
}

template <typename T, typename Compare>
void UnsortedPriorityQueue<T, Compare>::insert(T&& value) {
  items_.push_back(std::move(value));
}

template <typename T, typename Compare>
template <typename... Args>
void UnsortedPriorityQueue<T, Compare>::emplace(Args&&... args) {
  items_.emplace_back(std::forward<Args>(args)...);
}

template <typename T, typename Compare>
const T& UnsortedPriorityQueue<T, Compare>::min() const {
  return *findMin();
}

template <typename T, typename Compare>
void UnsortedPriorityQueue<T, Compare>::removeMin() {
  items_.erase(findMin());
}

template <typename T, typename Compare>
void UnsortedPriorityQueue<T, Compare>::clear() noexcept {
  items_.clear();
}

template <typename T, typename Compare>
auto UnsortedPriorityQueue<T, Compare>::findMin() const -> ConstIterator {
  if (items_.empty()) {
    throw std::out_of_range("UnsortedPriorityQueue: queue is empty");
  }
  // std::min_element returns the *first* of several equivalent minima,
  // which is what gives FIFO behaviour among ties.
  return std::min_element(items_.cbegin(), items_.cend(), comp_);
}

}  // namespace ds

#endif  // DS_UNSORTED_PRIORITY_QUEUE_TPP
