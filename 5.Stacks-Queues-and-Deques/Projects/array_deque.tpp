//  File: array_deque.tpp

// included by array_deque.hpp; do not compile directly.
#ifndef ARRAY_DEQUE_TPP
#define ARRAY_DEQUE_TPP

#include <memory>
#include <stdexcept>
#include <utility>

template <typename T>
ArrayDeque<T>::ArrayDeque(std::size_t capacity, bool growable)
    : buf_(new T[validated(capacity)]), cap_(capacity), growable_(growable) {}
template <typename T>
ArrayDeque<T>::ArrayDeque(const ArrayDeque &other)
    : buf_(new T[other.cap_]), cap_(other.cap_), front_(0), size_(other.size_),
      growable_(other.growable_) {
  for (std::size_t i = 0; i < size_; ++i) // copy in logical order, unwrapped
    buf_[i] = other.buf_[(other.front_ + i) % other.cap_];
}

template <typename T>
ArrayDeque<T>::ArrayDeque(ArrayDeque &&other) noexcept
    : buf_(std::move(other.buf_)), cap_(std::exchange(other.cap_, 0)),
      front_(std::exchange(other.front_, 0)),
      size_(std::exchange(other.size_, 0)), growable_(other.growable_) {}

template <typename T>
ArrayDeque<T> &ArrayDeque<T>::operator=(ArrayDeque other) noexcept {
  swap(other); // other's destructor releases our old buffer
  return *this;
}

template <typename T> void ArrayDeque<T>::swap(ArrayDeque &other) noexcept {
  buf_.swap(other.buf_);
  std::swap(cap_, other.cap_);
  std::swap(front_, other.front_);
  std::swap(size_, other.size_);
  std::swap(growable_, other.growable_);
}
template <typename T>
std::size_t ArrayDeque<T>::validated(std::size_t capacity) {
  if (capacity == 0)
    throw std::invalid_argument("capacity must be >= 1");
  return capacity;
}

// ---- Queries ----

template <typename T> std::size_t ArrayDeque<T>::size() const { return size_; }

template <typename T> std::size_t ArrayDeque<T>::capacity() const {
  return cap_;
}

template <typename T> bool ArrayDeque<T>::empty() const { return size_ == 0; }

template <typename T> bool ArrayDeque<T>::full() const { return size_ == cap_; }

template <typename T> T &ArrayDeque<T>::first() {
  requireNonEmpty();
  return buf_[front_];
}

template <typename T> T &ArrayDeque<T>::last() {
  requireNonEmpty();
  return buf_[(front_ + size_ - 1) % cap_];
}

template <typename T> const T &ArrayDeque<T>::first() const {
  requireNonEmpty();
  return buf_[front_];
}

template <typename T> const T &ArrayDeque<T>::last() const {
  requireNonEmpty();
  return buf_[(front_ + size_ - 1) % cap_];
}

template <typename T> const T &ArrayDeque<T>::at(std::size_t i) const {
  if (i >= size_)
    throw std::out_of_range("ArrayDeque::at");
  return buf_[(front_ + i) % cap_];
}

// ---- Updates ----

template <typename T> void ArrayDeque<T>::pushFront(T value) {
  makeRoom();
  front_ = (front_ + cap_ - 1) % cap_; // step backward without going negative
  buf_[front_] = std::move(value);
  ++size_;
}

template <typename T> void ArrayDeque<T>::pushBack(T value) {
  makeRoom();
  buf_[(front_ + size_) % cap_] = std::move(value);
  ++size_;
}

template <typename T> void ArrayDeque<T>::popFront() {
  requireNonEmpty();
  buf_[front_] = T(); // release any held resource now
  front_ = (front_ + 1) % cap_;
  --size_;
}

template <typename T> void ArrayDeque<T>::popBack() {
  requireNonEmpty();
  buf_[(front_ + size_ - 1) % cap_] = T();
  --size_;
}

// ---- Private helpers ----

template <typename T> void ArrayDeque<T>::requireNonEmpty() const {
  if (size_ == 0)
    throw std::out_of_range("ArrayDeque is empty");
}

// Runs before any state changes, so a failed push leaves the deque intact.
template <typename T> void ArrayDeque<T>::makeRoom() {
  if (size_ < cap_)
    return;
  if (!growable_)
    throw std::length_error("ArrayDeque is full");
  grow();
}

template <typename T> void ArrayDeque<T>::grow() {
  const std::size_t newCap = cap_ == 0 ? 1 : cap_ * 2;
  std::unique_ptr<T[]> nb(new T[newCap]);
  for (std::size_t i = 0; i < size_; ++i) // unwrap into logical order
    nb[i] = std::move(buf_[(front_ + i) % cap_]);
  buf_ = std::move(nb);
  cap_ = newCap;
  front_ = 0;
}

#endif // ARRAY_DEQUE_TPP