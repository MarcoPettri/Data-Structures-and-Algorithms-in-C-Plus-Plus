// File: LimitedQueue.hpp

/*
    Capacity-limited queue built on top of the deque.
*/

#pragma once

#include <cstddef>
#include <optional>

#include "LimitedDeque.hpp"

template <typename T> class LimitedQueue {
public:
  explicit LimitedQueue(std::size_t cap) : dq_(cap) {}

  // Returns true on success, false if the queue is full.
  [[nodiscard]] bool enqueue(const T &x) {
    if (dq_.full())
      return false; // guard BEFORE touching deque
    dq_.push_back(x);
    return true;
  }

  // Returns true on success, false if the queue is empty.
  [[nodiscard]] bool dequeue() {
    if (dq_.empty())
      return false; // guard BEFORE touching deque
    dq_.pop_front();
    return true;
  }

  // Returns the front element, or std::nullopt if empty.
  std::optional<T> front() const {
    if (dq_.empty())
      return std::nullopt;
    return dq_.front();
  }

  // Remove and return the front element in one step (optional convenience).
  std::optional<T> dequeue_value() {
    if (dq_.empty())
      return std::nullopt;
    T v = dq_.front();
    dq_.pop_front();
    return v;
  }

  bool empty() const { return dq_.empty(); }
  bool full() const { return dq_.full(); }
  std::size_t size() const { return dq_.size(); }
  std::size_t capacity() const { return dq_.capacity(); }

private:
  LimitedDeque<T> dq_;
};