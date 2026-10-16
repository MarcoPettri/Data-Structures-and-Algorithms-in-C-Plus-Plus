// File: LimitedStack.hpp
/*
    An implementation of a capacity-limited deque that does not
    throw exceptions when we attempt to perform a push or pop
    operation on a full or empty deque, respectively.
 */

#pragma once

#include "LimitedDeque.hpp"
#include <cstddef>
#include <optional>

template <typename T> class LimitedStack {
public:
  explicit LimitedStack(std::size_t cap) : dq_(cap) {}

  // Returns true on success, false if the stack is full.
  bool push(const T &x) {
    if (dq_.full())
      return false; // guard BEFORE touching deque
    dq_.push_back(x);
    return true;
  }

  // Returns true on success, false if the stack is empty.
  bool pop() {
    if (dq_.empty())
      return false; // guard BEFORE touching deque
    dq_.pop_back();
    return true;
  }

  // Returns the top element, or std::nullopt if empty.
  std::optional<T> top() const {
    if (dq_.empty())
      return std::nullopt;
    return dq_.back();
  }

  // Pop and return the element in one step (optional convenience).
  std::optional<T> pop_value() {
    if (dq_.empty())
      return std::nullopt;
    T v = dq_.back();
    dq_.pop_back();
    return v;
  }

  bool empty() const { return dq_.empty(); }
  bool full() const { return dq_.full(); }
  std::size_t size() const { return dq_.size(); }
  std::size_t capacity() const { return dq_.capacity(); }

private:
  LimitedDeque<T> dq_;
};