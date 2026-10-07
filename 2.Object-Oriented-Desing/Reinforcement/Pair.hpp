// Pair.hpp : INTERFACE (declarations only)
#ifndef PAIR_HPP
#define PAIR_HPP

#include <iosfwd> // std::ostream is only *named* here, not used
#include <type_traits>
#include <utility>

template <typename T1, typename T2> class Pair {
private:
  // Single source of truth for the conditional noexcept specifications
  static constexpr bool nothrow_move =
      std::is_nothrow_move_constructible_v<T1> &&
      std::is_nothrow_move_constructible_v<T2>;
  static constexpr bool nothrow_swap =
      std::is_nothrow_swappable_v<T1> && std::is_nothrow_swappable_v<T2>;

public:
  using first_type = T1;
  using second_type = T2;

  // Constructors
  Pair() = default;
  Pair(T1 first, T2 second);

  template <typename U1, typename U2> // converting constructor
  Pair(const Pair<U1, U2> &other);

  // Rule of Five
  ~Pair() = default;                         // 1. destructor
  Pair(const Pair &other);                   // 2. copy constructor
  Pair(Pair &&other) noexcept(nothrow_move); // 3. move constructor
  Pair &operator=(Pair other) noexcept(
      nothrow_swap); // 4+5. copy-and-swap (copy AND move assignment)

  // Swap (member; the free swap below forwards to it)
  void swap(Pair &other) noexcept(nothrow_swap);

  // Accessors
  T1 &first() noexcept;
  const T1 &first() const noexcept;
  T2 &second() noexcept;
  const T2 &second() const noexcept;

private:
  T1 first_{};
  T2 second_{};
};

// Non-member interface
template <typename T1, typename T2>
void swap(Pair<T1, T2> &a, Pair<T1, T2> &b) noexcept(noexcept(a.swap(b)));

template <typename T1, typename T2>
std::ostream &operator<<(std::ostream &os, const Pair<T1, T2> &p);

template <typename T1, typename T2>
bool operator==(const Pair<T1, T2> &a, const Pair<T1, T2> &b);
template <typename T1, typename T2>
bool operator!=(const Pair<T1, T2> &a, const Pair<T1, T2> &b);
template <typename T1, typename T2>
bool operator<(const Pair<T1, T2> &a, const Pair<T1, T2> &b);
template <typename T1, typename T2>
bool operator>(const Pair<T1, T2> &a, const Pair<T1, T2> &b);
template <typename T1, typename T2>
bool operator<=(const Pair<T1, T2> &a, const Pair<T1, T2> &b);
template <typename T1, typename T2>
bool operator>=(const Pair<T1, T2> &a, const Pair<T1, T2> &b);

template <typename T1, typename T2>
Pair<std::decay_t<T1>, std::decay_t<T2>> makePair(T1 &&a, T2 &&b);

// Templates must be visible at the point of instantiation, so the
// implementation file is included at the END of the header.
#include "Pair.tpp"

#endif // PAIR_HPP