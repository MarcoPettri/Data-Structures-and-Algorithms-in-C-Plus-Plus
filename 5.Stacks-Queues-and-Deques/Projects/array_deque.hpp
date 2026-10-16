// File: array_deque.hpp

#ifndef ARRAY_DEQUE_HPP
#define ARRAY_DEQUE_HPP

#include <cstddef>
#include <memory>

// Deque stored in a circular array.
//
// Fixed capacity by default (every operation is O(1) worst case).
// With growable = true the array doubles when full, so pushes become
// amortized O(1); pops and queries remain strictly O(1).
//
// Requirements on T: default-constructible and move-assignable.
//
// Errors (all leave the deque unchanged):
//   std::invalid_argument  - constructing with capacity 0
//   std::length_error      - push on a full, non-growable deque
//   std::out_of_range      - first/last/pop on an empty deque, or at(i) with i
//   >= size()

template <typename T> class ArrayDeque {
public:
  explicit ArrayDeque(std::size_t capacity, bool growable = false);
  ArrayDeque(const ArrayDeque &other);     // deep copy
  ArrayDeque(ArrayDeque &&other) noexcept; // leaves other empty, capacity 0
  ArrayDeque &
  operator=(ArrayDeque other) noexcept; // copy-and-swap; handles copy and move
  ~ArrayDeque() = default;              // unique_ptr frees the buffer

  void swap(ArrayDeque &other) noexcept;
  friend void swap(ArrayDeque &a, ArrayDeque &b) noexcept { a.swap(b); }
  // Queries - O(1)
  std::size_t size() const;
  std::size_t capacity() const;
  bool empty() const;
  bool full() const;

  T &first();
  T &last();
  const T &first() const;
  const T &last() const;

  // i-th element counted from the front (0 = first element).
  const T &at(std::size_t i) const;

  // Updates - O(1) (pushes amortized O(1) when growable)
  void pushFront(T value);
  void pushBack(T value);
  void popFront();
  void popBack();

private:
  static std::size_t validated(std::size_t capacity);
  void requireNonEmpty() const;
  void makeRoom(); // ensures one free slot, or throws / grows
  void grow();     // doubles capacity and unwraps the contents

  std::unique_ptr<T[]> buf_;
  std::size_t cap_;
  std::size_t front_ = 0; // index of the first element
  std::size_t size_ = 0;  // number of stored elements
  bool growable_;
};

#include "array_deque.tpp" // template definitions must be visible to users

#endif // ARRAY_DEQUE_HPP