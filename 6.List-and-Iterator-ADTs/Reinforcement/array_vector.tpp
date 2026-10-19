/**
 * @file    array_vector.tpp
 * @brief   Template definitions for adt::ArrayVector.
 * @warning Do not include directly; include array_vector.hpp.
 */

#ifndef ADT_ARRAY_VECTOR_TPP
#define ADT_ARRAY_VECTOR_TPP

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <utility>

namespace adt {

// ───────────────────────── private helpers ─────────────────────────

template <typename T, typename A>
auto ArrayVector<T, A>::allocate_(size_type n) -> pointer {
  if (n == 0)
    return nullptr;
  if (n > max_size())
    throw std::length_error("ArrayVector: requested size too large");
  return traits_::allocate(alloc_, n);
}

template <typename T, typename A>
void ArrayVector<T, A>::deallocate_(pointer p, size_type n) noexcept {
  if (p)
    traits_::deallocate(alloc_, p, n);
}

template <typename T, typename A>
void ArrayVector<T, A>::destroy_range_(pointer first, pointer last) noexcept {
  for (; first != last; ++first)
    traits_::destroy(alloc_, first);
}

template <typename T, typename A> void ArrayVector<T, A>::release_() noexcept {
  destroy_range_(data_, data_ + size_);
  deallocate_(data_, cap_);
  data_ = nullptr;
  size_ = cap_ = 0;
}

template <typename T, typename A>
void ArrayVector<T, A>::truncate_(size_type new_size) noexcept {
  destroy_range_(data_ + new_size, data_ + size_);
  size_ = new_size;
}

template <typename T, typename A>
void ArrayVector<T, A>::relocate_(pointer dest) {
  size_type i = 0;
  try {
    // Moves if the move cannot throw (or T is move-only), copies otherwise:
    // this is what gives push_back() its strong guarantee.
    for (; i < size_; ++i)
      traits_::construct(alloc_, dest + i, std::move_if_noexcept(data_[i]));
  } catch (...) {
    destroy_range_(dest, dest + i);
    throw;
  }
}

template <typename T, typename A>
void ArrayVector<T, A>::reallocate_(size_type new_cap) {
  pointer fresh = allocate_(new_cap);
  try {
    relocate_(fresh);
  } catch (...) {
    deallocate_(fresh, new_cap);
    throw;
  }
  destroy_range_(data_, data_ + size_);
  deallocate_(data_, cap_);
  data_ = fresh;
  cap_ = new_cap;
}

template <typename T, typename A>
auto ArrayVector<T, A>::next_capacity_(size_type required) const -> size_type {
  if (required > max_size())
    throw std::length_error("ArrayVector: size too large");
  const size_type doubled = (cap_ > max_size() / 2) ? max_size() : cap_ * 2;
  return std::max(doubled, required);
}

template <typename T, typename A>
void ArrayVector<T, A>::check_room_(size_type extra) const {
  if (extra > max_size() - size_)
    throw std::length_error("ArrayVector: size too large");
}

template <typename T, typename A>
template <typename It>
void ArrayVector<T, A>::append_range_(It first, It last) {
  if constexpr (std::is_base_of_v<
                    std::forward_iterator_tag,
                    typename std::iterator_traits<It>::iterator_category>) {
    const auto n = static_cast<size_type>(std::distance(first, last));
    check_room_(n);
    reserve(size_ + n);
  }
  for (; first != last; ++first)
    emplace_back(*first);
}

template <typename T, typename A>
void ArrayVector<T, A>::steal_(ArrayVector &o) noexcept {
  data_ = std::exchange(o.data_, nullptr);
  size_ = std::exchange(o.size_, 0);
  cap_ = std::exchange(o.cap_, 0);
}

// ───────────────────────── construction ─────────────────────────

template <typename T, typename A>
ArrayVector<T, A>::ArrayVector(size_type count, const allocator_type &alloc)
    : alloc_(alloc) {
  try {
    data_ = allocate_(count);
    cap_ = count;
    for (; size_ < count; ++size_)
      traits_::construct(alloc_, data_ + size_);
  } catch (...) {
    release_();
    throw;
  }
}

template <typename T, typename A>
ArrayVector<T, A>::ArrayVector(size_type count, const T &value,
                               const allocator_type &alloc)
    : alloc_(alloc) {
  try {
    data_ = allocate_(count);
    cap_ = count;
    for (; size_ < count; ++size_)
      traits_::construct(alloc_, data_ + size_, value);
  } catch (...) {
    release_();
    throw;
  }
}

template <typename T, typename A>
template <typename InputIt, typename>
ArrayVector<T, A>::ArrayVector(InputIt first, InputIt last,
                               const allocator_type &alloc)
    : alloc_(alloc) {
  try {
    append_range_(first, last);
  } catch (...) {
    release_();
    throw;
  }
}

template <typename T, typename A>
ArrayVector<T, A>::ArrayVector(std::initializer_list<T> init,
                               const allocator_type &alloc)
    : ArrayVector(init.begin(), init.end(), alloc) {}

template <typename T, typename A>
ArrayVector<T, A>::ArrayVector(const ArrayVector &other)
    : alloc_(traits_::select_on_container_copy_construction(other.alloc_)) {
  try {
    append_range_(other.begin(), other.end());
  } catch (...) {
    release_();
    throw;
  }
}

template <typename T, typename A>
ArrayVector<T, A>::ArrayVector(ArrayVector &&other) noexcept
    : alloc_(std::move(other.alloc_)) {
  steal_(other);
}

template <typename T, typename A> ArrayVector<T, A>::~ArrayVector() {
  release_();
}

// ───────────────────────── assignment ─────────────────────────

template <typename T, typename A>
auto ArrayVector<T, A>::operator=(const ArrayVector &other) -> ArrayVector & {
  if (this != &other) {
    if constexpr (traits_::propagate_on_container_copy_assignment::value) {
      if (alloc_ != other.alloc_)
        release_(); // old storage must die with its allocator
      alloc_ = other.alloc_;
    }
    assign(other.begin(), other.end());
  }
  return *this;
}

template <typename T, typename A>
auto ArrayVector<T, A>::operator=(ArrayVector &&other) noexcept(
    traits_::propagate_on_container_move_assignment::value ||
    traits_::is_always_equal::value) -> ArrayVector & {
  if (this == &other)
    return *this;
  if constexpr (traits_::propagate_on_container_move_assignment::value) {
    release_();
    alloc_ = std::move(other.alloc_);
    steal_(other);
  } else if constexpr (traits_::is_always_equal::value) {
    release_();
    steal_(other);
  } else {
    if (alloc_ == other.alloc_) {
      release_();
      steal_(other);
    } else { // cannot adopt foreign memory: move element-wise
      assign(std::make_move_iterator(other.begin()),
             std::make_move_iterator(other.end()));
      other.clear();
    }
  }
  return *this;
}

template <typename T, typename A>
auto ArrayVector<T, A>::operator=(std::initializer_list<T> init)
    -> ArrayVector & {
  assign(init);
  return *this;
}

template <typename T, typename A>
void ArrayVector<T, A>::assign(size_type count, const T &value) {
  const T copy(value); // value may alias one of our elements
  clear();
  reserve(count);
  for (size_type i = 0; i < count; ++i)
    emplace_back(copy);
}

template <typename T, typename A>
template <typename InputIt, typename>
void ArrayVector<T, A>::assign(InputIt first, InputIt last) {
  clear();
  append_range_(first, last);
}

template <typename T, typename A>
void ArrayVector<T, A>::assign(std::initializer_list<T> init) {
  assign(init.begin(), init.end());
}

// ───────────────────────── element access ─────────────────────────

template <typename T, typename A>
auto ArrayVector<T, A>::at(size_type i) -> reference {
  if (i >= size_)
    throw std::out_of_range("ArrayVector::at: index out of range");
  return data_[i];
}

template <typename T, typename A>
auto ArrayVector<T, A>::at(size_type i) const -> const_reference {
  if (i >= size_)
    throw std::out_of_range("ArrayVector::at: index out of range");
  return data_[i];
}

template <typename T, typename A>
auto ArrayVector<T, A>::operator[](size_type i) noexcept -> reference {
  assert(i < size_);
  return data_[i];
}

template <typename T, typename A>
auto ArrayVector<T, A>::operator[](size_type i) const noexcept
    -> const_reference {
  assert(i < size_);
  return data_[i];
}

template <typename T, typename A>
auto ArrayVector<T, A>::front() noexcept -> reference {
  assert(!empty());
  return data_[0];
}

template <typename T, typename A>
auto ArrayVector<T, A>::front() const noexcept -> const_reference {
  assert(!empty());
  return data_[0];
}

template <typename T, typename A>
auto ArrayVector<T, A>::back() noexcept -> reference {
  assert(!empty());
  return data_[size_ - 1];
}

template <typename T, typename A>
auto ArrayVector<T, A>::back() const noexcept -> const_reference {
  assert(!empty());
  return data_[size_ - 1];
}

// ───────────────────────── capacity ─────────────────────────

template <typename T, typename A>
void ArrayVector<T, A>::reserve(size_type new_cap) {
  if (new_cap > max_size())
    throw std::length_error("ArrayVector::reserve: size too large");
  if (new_cap > cap_)
    reallocate_(new_cap);
}

template <typename T, typename A> void ArrayVector<T, A>::shrink_to_fit() {
  if (cap_ == size_)
    return;
  if (size_ == 0)
    release_();
  else
    reallocate_(size_);
}

// ───────────────────────── modifiers ─────────────────────────

template <typename T, typename A> void ArrayVector<T, A>::clear() noexcept {
  truncate_(0);
}

template <typename T, typename A>
template <typename... Args>
auto ArrayVector<T, A>::emplace_back(Args &&...args) -> reference {
  if (size_ == cap_) {
    const size_type new_cap = next_capacity_(size_ + 1);
    pointer fresh = allocate_(new_cap);
    // Construct the new element FIRST: args may refer to an existing element.
    try {
      traits_::construct(alloc_, fresh + size_, std::forward<Args>(args)...);
    } catch (...) {
      deallocate_(fresh, new_cap);
      throw;
    }
    try {
      relocate_(fresh);
    } catch (...) {
      traits_::destroy(alloc_, fresh + size_);
      deallocate_(fresh, new_cap);
      throw;
    }
    destroy_range_(data_, data_ + size_);
    deallocate_(data_, cap_);
    data_ = fresh;
    cap_ = new_cap;
  } else {
    traits_::construct(alloc_, data_ + size_, std::forward<Args>(args)...);
  }
  ++size_;
  return data_[size_ - 1];
}

template <typename T, typename A> void ArrayVector<T, A>::pop_back() noexcept {
  assert(!empty());
  --size_;
  traits_::destroy(alloc_, data_ + size_);
}

template <typename T, typename A>
template <typename... Args>
auto ArrayVector<T, A>::emplace(const_iterator pos, Args &&...args)
    -> iterator {
  assert(pos >= cbegin() && pos <= cend());
  const auto idx = pos - cbegin();
  emplace_back(std::forward<Args>(args)...);    // append …
  std::rotate(begin() + idx, end() - 1, end()); // … then rotate into place
  return begin() + idx;
}

template <typename T, typename A>
auto ArrayVector<T, A>::insert(const_iterator pos, const T &value) -> iterator {
  return emplace(pos, value);
}

template <typename T, typename A>
auto ArrayVector<T, A>::insert(const_iterator pos, T &&value) -> iterator {
  return emplace(pos, std::move(value));
}

template <typename T, typename A>
auto ArrayVector<T, A>::insert(const_iterator pos, size_type count,
                               const T &value) -> iterator {
  assert(pos >= cbegin() && pos <= cend());
  const auto idx = pos - cbegin();
  if (count == 0)
    return begin() + idx;
  check_room_(count);
  const T copy(value); // value may alias one of our elements
  const size_type old_size = size_;
  try {
    reserve(size_ + count);
    for (size_type i = 0; i < count; ++i)
      emplace_back(copy);
  } catch (...) {
    truncate_(old_size);
    throw;
  }
  std::rotate(begin() + idx, begin() + old_size, end());
  return begin() + idx;
}

template <typename T, typename A>
template <typename InputIt, typename>
auto ArrayVector<T, A>::insert(const_iterator pos, InputIt first, InputIt last)
    -> iterator {
  assert(pos >= cbegin() && pos <= cend());
  const auto idx = pos - cbegin();
  const size_type old_size = size_;
  try {
    append_range_(first, last);
  } catch (...) {
    truncate_(old_size);
    throw;
  }
  std::rotate(begin() + idx, begin() + old_size, end());
  return begin() + idx;
}

template <typename T, typename A>
auto ArrayVector<T, A>::insert(const_iterator pos,
                               std::initializer_list<T> init) -> iterator {
  return insert(pos, init.begin(), init.end());
}

template <typename T, typename A>
auto ArrayVector<T, A>::erase(const_iterator pos) -> iterator {
  assert(pos >= cbegin() && pos < cend());
  return erase(pos, pos + 1);
}

template <typename T, typename A>
auto ArrayVector<T, A>::erase(const_iterator first, const_iterator last)
    -> iterator {
  assert(first >= cbegin() && first <= last && last <= cend());
  iterator f = begin() + (first - cbegin());
  iterator l = begin() + (last - cbegin());
  if (f != l) {
    iterator new_end = std::move(l, end(), f);
    truncate_(static_cast<size_type>(new_end - begin()));
  }
  return f;
}

template <typename T, typename A>
void ArrayVector<T, A>::resize(size_type count) {
  if (count <= size_) {
    truncate_(count);
    return;
  }
  reserve(count);
  for (; size_ < count; ++size_)
    traits_::construct(alloc_, data_ + size_);
}

template <typename T, typename A>
void ArrayVector<T, A>::resize(size_type count, const T &value) {
  if (count <= size_) {
    truncate_(count);
    return;
  }
  const T copy(value); // value may alias one of our elements
  reserve(count);
  for (; size_ < count; ++size_)
    traits_::construct(alloc_, data_ + size_, copy);
}

template <typename T, typename A>
void ArrayVector<T, A>::swap(ArrayVector &other) noexcept(
    traits_::propagate_on_container_swap::value ||
    traits_::is_always_equal::value) {
  using std::swap;
  if constexpr (traits_::propagate_on_container_swap::value)
    swap(alloc_, other.alloc_);
  swap(data_, other.data_);
  swap(size_, other.size_);
  swap(cap_, other.cap_);
}

// ───────────────────────── non-member ─────────────────────────

template <typename T, typename A>
bool operator==(const ArrayVector<T, A> &l, const ArrayVector<T, A> &r) {
  return l.size() == r.size() && std::equal(l.begin(), l.end(), r.begin());
}
template <typename T, typename A>
bool operator!=(const ArrayVector<T, A> &l, const ArrayVector<T, A> &r) {
  return !(l == r);
}
template <typename T, typename A>
bool operator<(const ArrayVector<T, A> &l, const ArrayVector<T, A> &r) {
  return std::lexicographical_compare(l.begin(), l.end(), r.begin(), r.end());
}
template <typename T, typename A>
bool operator>(const ArrayVector<T, A> &l, const ArrayVector<T, A> &r) {
  return r < l;
}
template <typename T, typename A>
bool operator<=(const ArrayVector<T, A> &l, const ArrayVector<T, A> &r) {
  return !(r < l);
}
template <typename T, typename A>
bool operator>=(const ArrayVector<T, A> &l, const ArrayVector<T, A> &r) {
  return !(l < r);
}

template <typename T, typename A>
void swap(ArrayVector<T, A> &l,
          ArrayVector<T, A> &r) noexcept(noexcept(l.swap(r))) {
  l.swap(r);
}

} // namespace adt

#endif // ADT_ARRAY_VECTOR_TPP