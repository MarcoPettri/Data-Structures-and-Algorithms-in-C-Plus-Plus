/**
 * @file    array_list.tpp
 * @brief   Template definitions for adt::ArrayList.
 * @warning Do not include directly; include array_list.hpp.
 */

#ifndef ADT_ARRAY_LIST_TPP
#define ADT_ARRAY_LIST_TPP

#include <algorithm>
#include <cassert>
#include <stdexcept>

namespace adt {

// ───────────────────────── private helpers ─────────────────────────

template <typename T, typename A>
auto ArrayList<T, A>::allocate_(size_type n) -> pointer {
  if (n == 0)
    return nullptr;
  if (n > max_size())
    throw std::length_error("ArrayList: requested size too large");
  return traits_::allocate(alloc_, n);
}

template <typename T, typename A>
void ArrayList<T, A>::deallocate_(pointer p, size_type n) noexcept {
  if (p)
    traits_::deallocate(alloc_, p, n);
}

template <typename T, typename A>
void ArrayList<T, A>::destroy_range_(pointer first, pointer last) noexcept {
  for (; first != last; ++first)
    traits_::destroy(alloc_, first);
}

template <typename T, typename A> void ArrayList<T, A>::release_() noexcept {
  destroy_range_(data_, data_ + size_);
  deallocate_(data_, cap_);
  data_ = nullptr;
  size_ = cap_ = 0;
}

// Constructs [dst, dst+n) from [src, src+n), moving only if that cannot throw
// (or T is move-only). On failure, destroys what it built and rethrows.
template <typename T, typename A>
void ArrayList<T, A>::relocate_(pointer src, size_type n, pointer dst) {
  size_type i = 0;
  try {
    for (; i < n; ++i)
      traits_::construct(alloc_, dst + i, std::move_if_noexcept(src[i]));
  } catch (...) {
    destroy_range_(dst, dst + i);
    throw;
  }
}

template <typename T, typename A>
void ArrayList<T, A>::reallocate_(size_type new_cap) {
  pointer fresh = allocate_(new_cap);
  try {
    relocate_(data_, size_, fresh);
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
auto ArrayList<T, A>::grow_capacity_(size_type required) const -> size_type {
  if (required > max_size())
    throw std::length_error("ArrayList: size too large");
  const size_type doubled = (cap_ > max_size() / 2) ? max_size() : cap_ * 2;
  return std::max(doubled, required);
}

template <typename T, typename A>
void ArrayList<T, A>::check_index_(size_type index, size_type limit) const {
  if (index >= limit)
    throw std::out_of_range("ArrayList: index out of range");
}

template <typename T, typename A>
void ArrayList<T, A>::erase_at_(size_type index) {
  std::move(data_ + index + 1, data_ + size_, data_ + index);
  pop_back();
}

template <typename T, typename A>
void ArrayList<T, A>::steal_(ArrayList &o) noexcept {
  data_ = std::exchange(o.data_, nullptr);
  size_ = std::exchange(o.size_, 0);
  cap_ = std::exchange(o.cap_, 0);
}

template <typename T, typename A>
template <typename It>
void ArrayList<T, A>::append_(It first, It last) {
  if constexpr (std::is_base_of_v<
                    std::forward_iterator_tag,
                    typename std::iterator_traits<It>::iterator_category>) {
    const auto n = static_cast<size_type>(std::distance(first, last));
    if (n > max_size() - size_)
      throw std::length_error("ArrayList: size too large");
    reserve(size_ + n);
  }
  for (; first != last; ++first)
    emplace_back(*first);
}

// ───────────────────────── construction ─────────────────────────

template <typename T, typename A>
ArrayList<T, A>::ArrayList(size_type count, const T &value,
                           const allocator_type &alloc)
    : alloc_(alloc) {
  try {
    reserve(count);
    for (size_type i = 0; i < count; ++i)
      emplace_back(value);
  } catch (...) {
    release_();
    throw;
  }
}

template <typename T, typename A>
template <typename InputIt, typename>
ArrayList<T, A>::ArrayList(InputIt first, InputIt last,
                           const allocator_type &alloc)
    : alloc_(alloc) {
  try {
    append_(first, last);
  } catch (...) {
    release_();
    throw;
  }
}

template <typename T, typename A>
ArrayList<T, A>::ArrayList(std::initializer_list<T> init,
                           const allocator_type &alloc)
    : ArrayList(init.begin(), init.end(), alloc) {}

template <typename T, typename A>
ArrayList<T, A>::ArrayList(const ArrayList &other)
    : alloc_(traits_::select_on_container_copy_construction(other.alloc_)) {
  try {
    append_(other.begin(), other.end());
  } catch (...) {
    release_();
    throw;
  }
}

template <typename T, typename A>
ArrayList<T, A>::ArrayList(ArrayList &&other) noexcept
    : alloc_(std::move(other.alloc_)) {
  steal_(other);
}

template <typename T, typename A> ArrayList<T, A>::~ArrayList() { release_(); }

// ───────────────────────── assignment ─────────────────────────

template <typename T, typename A>
auto ArrayList<T, A>::operator=(const ArrayList &other) -> ArrayList & {
  if (this != &other) {
    if constexpr (traits_::propagate_on_container_copy_assignment::value) {
      if (alloc_ != other.alloc_)
        release_(); // old storage must die with its allocator
      alloc_ = other.alloc_;
    }
    clear();
    append_(other.begin(), other.end());
  }
  return *this;
}

template <typename T, typename A>
auto ArrayList<T, A>::operator=(ArrayList &&other) noexcept(
    traits_::propagate_on_container_move_assignment::value ||
    traits_::is_always_equal::value) -> ArrayList & {
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
      clear();
      append_(std::make_move_iterator(other.begin()),
              std::make_move_iterator(other.end()));
      other.clear();
    }
  }
  return *this;
}

template <typename T, typename A>
auto ArrayList<T, A>::operator=(std::initializer_list<T> init) -> ArrayList & {
  clear();
  append_(init.begin(), init.end());
  return *this;
}

// ───────────────────────── element access ─────────────────────────

template <typename T, typename A>
auto ArrayList<T, A>::at(size_type index) -> reference {
  check_index_(index, size_);
  return data_[index];
}

template <typename T, typename A>
auto ArrayList<T, A>::at(size_type index) const -> const_reference {
  check_index_(index, size_);
  return data_[index];
}

template <typename T, typename A>
auto ArrayList<T, A>::operator[](size_type index) noexcept -> reference {
  assert(index < size_);
  return data_[index];
}

template <typename T, typename A>
auto ArrayList<T, A>::operator[](size_type index) const noexcept
    -> const_reference {
  assert(index < size_);
  return data_[index];
}

template <typename T, typename A>
auto ArrayList<T, A>::front() noexcept -> reference {
  assert(!empty());
  return data_[0];
}

template <typename T, typename A>
auto ArrayList<T, A>::front() const noexcept -> const_reference {
  assert(!empty());
  return data_[0];
}

template <typename T, typename A>
auto ArrayList<T, A>::back() noexcept -> reference {
  assert(!empty());
  return data_[size_ - 1];
}

template <typename T, typename A>
auto ArrayList<T, A>::back() const noexcept -> const_reference {
  assert(!empty());
  return data_[size_ - 1];
}

// ───────────────────────── capacity ─────────────────────────

template <typename T, typename A>
void ArrayList<T, A>::reserve(size_type new_cap) {
  if (new_cap > max_size())
    throw std::length_error("ArrayList::reserve: size too large");
  if (new_cap > cap_)
    reallocate_(new_cap);
}

template <typename T, typename A> void ArrayList<T, A>::shrink_to_fit() {
  if (cap_ == size_)
    return;
  if (size_ == 0)
    release_();
  else
    reallocate_(size_);
}

// ───────────────────────── insertion ─────────────────────────

template <typename T, typename A>
template <typename... Args>
auto ArrayList<T, A>::emplace(size_type index, Args &&...args) -> reference {
  check_index_(index, size_ + 1); // index == size() is allowed

  if (size_ == cap_) {
    // Reallocating path: build the new element first (args may refer to an
    // existing element), then relocate the prefix and suffix around it.
    const size_type new_cap = grow_capacity_(size_ + 1);
    pointer fresh = allocate_(new_cap);
    try {
      traits_::construct(alloc_, fresh + index, std::forward<Args>(args)...);
    } catch (...) {
      deallocate_(fresh, new_cap);
      throw;
    }
    try {
      relocate_(data_, index, fresh);
      try {
        relocate_(data_ + index, size_ - index, fresh + index + 1);
      } catch (...) {
        destroy_range_(fresh, fresh + index);
        throw;
      }
    } catch (...) {
      traits_::destroy(alloc_, fresh + index);
      deallocate_(fresh, new_cap);
      throw;
    }
    destroy_range_(data_, data_ + size_);
    deallocate_(data_, cap_);
    data_ = fresh;
    cap_ = new_cap;
    ++size_;
  } else if (index == size_) {
    traits_::construct(alloc_, data_ + size_, std::forward<Args>(args)...);
    ++size_;
  } else {
    // In-place path: materialise the value first (it may alias an element),
    // open a gap by shifting right, then move the value into the gap.
    T tmp(std::forward<Args>(args)...);
    traits_::construct(alloc_, data_ + size_, std::move(data_[size_ - 1]));
    ++size_;
    std::move_backward(data_ + index, data_ + size_ - 2, data_ + size_ - 1);
    data_[index] = std::move(tmp);
  }
  return data_[index];
}

// ───────────────────────── removal ─────────────────────────

template <typename T, typename A>
T ArrayList<T, A>::remove_at(size_type index) {
  check_index_(index, size_);
  T result(std::move(data_[index]));
  erase_at_(index);
  return result;
}

template <typename T, typename A> void ArrayList<T, A>::pop_back() noexcept {
  assert(!empty());
  --size_;
  traits_::destroy(alloc_, data_ + size_);
}

template <typename T, typename A> void ArrayList<T, A>::pop_front() {
  assert(!empty());
  erase_at_(0);
}

template <typename T, typename A> bool ArrayList<T, A>::remove(const T &value) {
  const size_type i = index_of(value);
  if (i == npos)
    return false;
  erase_at_(i);
  return true;
}

template <typename T, typename A> void ArrayList<T, A>::clear() noexcept {
  destroy_range_(data_, data_ + size_);
  size_ = 0;
}

// ───────────────────────── searching ─────────────────────────

template <typename T, typename A>
auto ArrayList<T, A>::index_of(const T &value) const -> size_type {
  for (size_type i = 0; i < size_; ++i)
    if (data_[i] == value)
      return i;
  return npos;
}

// ───────────────────────── swap / non-member ─────────────────────────

template <typename T, typename A>
void ArrayList<T, A>::swap(ArrayList &other) noexcept(
    traits_::propagate_on_container_swap::value ||
    traits_::is_always_equal::value) {
  using std::swap;
  if constexpr (traits_::propagate_on_container_swap::value)
    swap(alloc_, other.alloc_);
  swap(data_, other.data_);
  swap(size_, other.size_);
  swap(cap_, other.cap_);
}

template <typename T, typename A>
bool operator==(const ArrayList<T, A> &l, const ArrayList<T, A> &r) {
  return l.size() == r.size() && std::equal(l.begin(), l.end(), r.begin());
}

template <typename T, typename A>
bool operator!=(const ArrayList<T, A> &l, const ArrayList<T, A> &r) {
  return !(l == r);
}

template <typename T, typename A>
void swap(ArrayList<T, A> &l,
          ArrayList<T, A> &r) noexcept(noexcept(l.swap(r))) {
  l.swap(r);
}

} // namespace adt

#endif // ADT_ARRAY_LIST_TPP