// Pair.tpp : IMPLEMENTATION (definitions of everything declared in Pair.hpp)
// Do not compile this file on its own; it is included by Pair.hpp.
#ifndef PAIR_TPP
#define PAIR_TPP

#include <ostream>

// ---- Constructors ----------------------------------------------------------
template <typename T1, typename T2>
Pair<T1, T2>::Pair(T1 first, T2 second)
    : first_(std::move(first)), second_(std::move(second)) {}

template <typename T1, typename T2>
template <typename U1, typename U2>
Pair<T1, T2>::Pair(const Pair<U1, U2> &other)
    : first_(other.first()), second_(other.second()) {}

// ---- Rule of Five ----------------------------------------------------------
template <typename T1, typename T2>
Pair<T1, T2>::Pair(const Pair &other)
    : first_(other.first_), second_(other.second_) {}

template <typename T1, typename T2>
Pair<T1, T2>::Pair(Pair &&other) noexcept(nothrow_move)
    : first_(std::move(other.first_)), second_(std::move(other.second_)) {}

template <typename T1, typename T2>
Pair<T1, T2> &Pair<T1, T2>::operator=(Pair other) noexcept(nothrow_swap) {
  swap(other); // 'other' is already a copy (lvalue arg) or moved-in (rvalue arg)
  return *this;
}

// ---- Swap ------------------------------------------------------------------
template <typename T1, typename T2>
void Pair<T1, T2>::swap(Pair &other) noexcept(nothrow_swap) {
  using std::swap; // enables ADL for user-defined T1/T2
  swap(first_, other.first_);
  swap(second_, other.second_);
}

template <typename T1, typename T2>
void swap(Pair<T1, T2> &a, Pair<T1, T2> &b) noexcept(noexcept(a.swap(b))) {
  a.swap(b);
}

// ---- Accessors -------------------------------------------------------------
template <typename T1, typename T2> T1 &Pair<T1, T2>::first() noexcept {
  return first_;
}
template <typename T1, typename T2>
const T1 &Pair<T1, T2>::first() const noexcept {
  return first_;
}
template <typename T1, typename T2> T2 &Pair<T1, T2>::second() noexcept {
  return second_;
}
template <typename T1, typename T2>
const T2 &Pair<T1, T2>::second() const noexcept {
  return second_;
}

// ---- Stream output ---------------------------------------------------------
template <typename T1, typename T2>
std::ostream &operator<<(std::ostream &os, const Pair<T1, T2> &p) {
  return os << '(' << p.first() << ", " << p.second() << ')';
}

// ---- Comparisons (need only == and < from the member types) -----------------
template <typename T1, typename T2>
bool operator==(const Pair<T1, T2> &a, const Pair<T1, T2> &b) {
  return a.first() == b.first() && a.second() == b.second();
}
template <typename T1, typename T2>
bool operator<(const Pair<T1, T2> &a, const Pair<T1, T2> &b) {
  return a.first() < b.first() ||
         (!(b.first() < a.first()) && a.second() < b.second());
}
template <typename T1, typename T2>
bool operator!=(const Pair<T1, T2> &a, const Pair<T1, T2> &b) {
  return !(a == b);
}
template <typename T1, typename T2>
bool operator>(const Pair<T1, T2> &a, const Pair<T1, T2> &b) {
  return b < a;
}
template <typename T1, typename T2>
bool operator<=(const Pair<T1, T2> &a, const Pair<T1, T2> &b) {
  return !(b < a);
}
template <typename T1, typename T2>
bool operator>=(const Pair<T1, T2> &a, const Pair<T1, T2> &b) {
  return !(a < b);
}

// ---- Factory ---------------------------------------------------------------
template <typename T1, typename T2>
Pair<std::decay_t<T1>, std::decay_t<T2>> makePair(T1 &&a, T2 &&b) {
  return {std::forward<T1>(a), std::forward<T2>(b)};
}

#endif // PAIR_TPP
