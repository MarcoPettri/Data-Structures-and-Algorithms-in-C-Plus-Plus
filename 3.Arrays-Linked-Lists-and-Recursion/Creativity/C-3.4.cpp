// File: C-3.4.cpp
/*
    C-3.4 Let B be an array of size n >= 6 containing integers from 1 to n - 5,
    inclusive, with exactly five repeated. Describe a good algorithm for finding
    the five integers in B that are repeated.

    Answer:
      B has n elements but only n - 5 possible values, so by the Pigeonhole
      Principle there are exactly 5 surplus copies. Every value in
      {1, ..., n - 5} appears at least once; the five surplus copies are the
      "repeated" integers we must report.

      Method 1: Counting Array
        1. Allocate count[1..n-5], all zeros.
        2. For each element v, increment count[v]. If the new count exceeds 1,
           v is a surplus copy: report it.
        - Running time : O(n)
        - Space usage  : O(n) auxiliary.

      Method 2: In-place Sign Flip
        1. Every value lies in 1..n-5 <= n, so it is a valid index into B.
        2. For each element, let x = |B[i]|. If B[x-1] is already negative,
           x has been seen before, so report it; otherwise negate B[x-1].
        3. Take abs() of everything afterwards to restore the array.
        - Running time : O(n)
        - Space usage  : O(1) auxiliary (besides the output). Temporarily
          modifies the array and needs a signed element type.

      Method 3: Sort and Scan
        1. Sort a copy of B.
        2. Any element equal to its predecessor is a surplus copy.
        - Running time : O(n log n)
        - Space usage  : O(n) for the copy (O(1) if sorting in place).
*/

#include <algorithm> // For std::shuffle, std::sort
#include <cassert>   // For testing
#include <cstddef>   // For std::size_t, std::ptrdiff_t
#include <iostream>
#include <iterator>    // std::begin(), std::end(), std::distance
#include <random>      // For std::mt19937
#include <stdexcept>   // For std::invalid_argument, std::out_of_range
#include <type_traits> // std::is_integral_v<T>
#include <vector>      // For std::vector

// =====================================================================
// Method 1: Counting Array
// =====================================================================
template <typename ForwardIt>
auto findDuplicatesCount(ForwardIt first, ForwardIt last) {
  using T = typename std::iterator_traits<ForwardIt>::value_type;
  static_assert(std::is_integral_v<T>,
                "Element type must be an integral type.");

  const auto n = static_cast<std::size_t>(std::distance(first, last));
  if (n < 6) {
    throw std::invalid_argument("Range must contain at least 6 elements.");
  }
  const std::size_t maxVal = n - 5;

  std::vector<std::size_t> count(maxVal + 1, 0);
  std::vector<T> result;
  result.reserve(5);

  for (; first != last; ++first) {
    if (*first < 1 || static_cast<std::size_t>(*first) > maxVal) {
      throw std::out_of_range("Element outside the range 1..n-5.");
    }
    const auto v = static_cast<std::size_t>(*first);
    if (++count[v] > 1) {
      result.push_back(*first);
    }
  }
  return result;
}

template <typename Container> auto findDuplicatesCount(const Container &c) {
  return findDuplicatesCount(std::begin(c), std::end(c));
}

// =====================================================================
// Method 2: In-place Sign Flip (restores the array before returning)
// =====================================================================
template <typename RandomIt>
auto findDuplicatesInPlace(RandomIt first, RandomIt last) {
  using T = typename std::iterator_traits<RandomIt>::value_type;
  static_assert(std::is_integral_v<T> && std::is_signed_v<T>,
                "Element type must be a signed integral type.");

  const auto n = static_cast<std::ptrdiff_t>(last - first);
  if (n < 6) {
    throw std::invalid_argument("Range must contain at least 6 elements.");
  }

  // Validate first so we never index out of bounds or leave the array
  // half-modified when we throw.
  for (auto it = first; it != last; ++it) {
    if (*it < 1 || static_cast<std::ptrdiff_t>(*it) > n - 5) {
      throw std::out_of_range("Element outside the range 1..n-5.");
    }
  }

  std::vector<T> result;
  result.reserve(5);

  for (std::ptrdiff_t i = 0; i < n; ++i) {
    const T v = first[i];
    const T x = (v < 0) ? static_cast<T>(-v) : v; // |v|
    const std::ptrdiff_t idx = static_cast<std::ptrdiff_t>(x) - 1;
    if (first[idx] < 0) {
      result.push_back(x); // x was seen before
    } else {
      first[idx] = static_cast<T>(-first[idx]);
    }
  }

  // Restore the original values.
  for (std::ptrdiff_t i = 0; i < n; ++i) {
    if (first[i] < 0) {
      first[i] = static_cast<T>(-first[i]);
    }
  }
  return result;
}

template <typename Container> auto findDuplicatesInPlace(Container &c) {
  return findDuplicatesInPlace(std::begin(c), std::end(c));
}

// =====================================================================
// Method 3: Sort and Scan
// =====================================================================
template <typename ForwardIt>
auto findDuplicatesSort(ForwardIt first, ForwardIt last) {
  using T = typename std::iterator_traits<ForwardIt>::value_type;
  static_assert(std::is_integral_v<T>,
                "Element type must be an integral type.");

  std::vector<T> s(first, last);
  if (s.size() < 6) {
    throw std::invalid_argument("Range must contain at least 6 elements.");
  }
  std::sort(s.begin(), s.end());

  std::vector<T> result;
  result.reserve(5);
  for (std::size_t i = 1; i < s.size(); ++i) {
    if (s[i] == s[i - 1]) {
      result.push_back(s[i]);
    }
  }
  return result;
}

template <typename Container> auto findDuplicatesSort(const Container &c) {
  return findDuplicatesSort(std::begin(c), std::end(c));
}

// =====================================================================
// Tests & Verification
// =====================================================================
namespace {
// Runs all three methods and compares (order-insensitive) with `expected`.
// Also verifies the in-place method restored the array.
void check(const std::vector<int> &A, std::vector<int> expected) {
  std::sort(expected.begin(), expected.end());

  auto r1 = findDuplicatesCount(A);
  std::vector<int> copy = A;
  auto r2 = findDuplicatesInPlace(copy);
  auto r3 = findDuplicatesSort(A);

  std::sort(r1.begin(), r1.end());
  std::sort(r2.begin(), r2.end());
  std::sort(r3.begin(), r3.end());

  assert(r1 == expected);
  assert(r2 == expected);
  assert(r3 == expected);
  assert(copy == A); // sign flip must restore the input
}
} // namespace

int main() {
  std::cout << "=== C-3.4: Finding the Five Repeated Integers ===\n\n";

  // Test 1: Minimal size (n = 6), values 1..1 -> five surplus 1s
  check({1, 1, 1, 1, 1, 1}, {1, 1, 1, 1, 1});
  std::cout << "Test 1 Passed (n = 6): A = {1,1,1,1,1,1} -> {1,1,1,1,1}\n";

  // Test 2: Five distinct values each repeated once (n = 10, range 1..5)
  check({1, 2, 3, 4, 5, 1, 2, 3, 4, 5}, {1, 2, 3, 4, 5});
  std::cout << "Test 2 Passed (n = 10): five distinct duplicates found\n";

  // Test 3: One value repeated many times (n = 8, range 1..3)
  check({3, 3, 3, 3, 3, 3, 1, 2}, {3, 3, 3, 3, 3});
  std::cout << "Test 3 Passed (n = 8): value 3 appears six times\n";

  // Test 4: Duplicates at boundary positions (first and last elements)
  check({5, 1, 2, 3, 4, 6, 7, 8, 9, 10, 1, 2, 3, 4, 5}, {1, 2, 3, 4, 5});
  std::cout << "Test 4 Passed (n = 15): duplicates at both ends\n";

  // Test 5: Exceptions on invalid input
  {
    bool tooSmall = false, outOfRange = false;
    try {
      std::vector<int> A = {1, 2, 3, 4, 5};
      findDuplicatesCount(A);
    } catch (const std::invalid_argument &) {
      tooSmall = true;
    }
    try {
      std::vector<int> A = {1, 2, 3, 99, 1, 1}; // 99 > n - 5 = 1
      findDuplicatesCount(A);
    } catch (const std::out_of_range &) {
      outOfRange = true;
    } // For std::invalid_argument
    assert(tooSmall && outOfRange);
    std::cout << "Test 5 Passed: exceptions thrown as expected.\n";
  }

  // Test 6: Large array stress test (n = 100,000)
  {
    constexpr std::size_t N = 100000;
    const std::vector<int> dups = {7, 500, 42424, 77777, 99995};

    std::vector<int> A;
    A.reserve(N);
    for (std::size_t i = 1; i <= N - 5; ++i) {
      A.push_back(static_cast<int>(i));
    }
    for (int d : dups) {
      A.push_back(d);
    }

    std::mt19937 rng{12345};
    std::shuffle(A.begin(), A.end(), rng);

    check(A, dups);
    std::cout << "Test 6 Passed (n = 100,000): all three methods found the "
                 "five duplicates.\n";
  }

  std::cout << "\nAll C-3.4 tests passed successfully!\n";
  return 0;
}