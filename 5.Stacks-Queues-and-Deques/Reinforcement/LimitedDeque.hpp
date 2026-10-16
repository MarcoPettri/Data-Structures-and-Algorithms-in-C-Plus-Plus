// File: LimitedDeque.hpp
/*
    Minimal capacity-limited deque.

*/
#pragma once

#include <cstddef>
#include <deque>
#include <stdexcept>

template <typename T> class LimitedDeque {
public:
  explicit LimitedDeque(std::size_t cap) : cap_(cap) {}

  void push_back(const T &x) {
    if (full())
      throw std::overflow_error("deque full");
    d_.push_back(x);
  }
  void push_front(const T &x) {
    if (full())
      throw std::overflow_error("deque full");
    d_.push_front(x);
  }
  void pop_back() {
    if (d_.empty())
      throw std::underflow_error("deque empty");
    d_.pop_back();
  }
  void pop_front() {
    if (d_.empty())
      throw std::underflow_error("deque empty");
    d_.pop_front();
  }
  const T &back() const {
    if (d_.empty())
      throw std::underflow_error("deque empty");
    return d_.back();
  }
  const T &front() const {
    if (d_.empty())
      throw std::underflow_error("deque empty");
    return d_.front();
  }
  std::size_t size() const { return d_.size(); }
  std::size_t capacity() const { return cap_; }
  bool empty() const { return d_.empty(); }
  bool full() const { return d_.size() >= cap_; }

private:
  std::deque<T> d_;
  std::size_t cap_;
};
