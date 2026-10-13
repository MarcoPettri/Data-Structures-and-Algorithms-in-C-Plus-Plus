// File: C-3.3.cpp
/*
    C-3.3 Let A be an array of size n >= 2 containing integers from 1 to n - 1,
    inclusive, with exactly one repeated. Describe a fast algorithm for finding
    the integer in A that is repeated.

    Answer:
      By the Pigeonhole Principle, since there are n slots and only n - 1
   possible values {1, 2, ..., n - 1}, at least one value must repeat. The
   problem specifies that exactly one integer is repeated, meaning every integer
   in {1, ..., n - 1} appears at least once, and one specific integer k appears
   twice.

      Method 1: Sum Difference (Gauss's Formula)
        1. Calculate the expected sum of all integers from 1 to n - 1:
             S_expected = (n - 1) * n / 2
        2. Calculate the actual sum of the elements in array A:
             S_actual = sum_{i=0}^{n-1} A[i]
        3. The repeated integer k is the difference:
             k = S_actual - S_expected
        - Running time : O(n) — single linear pass over the array.
        - Space usage  : O(1) auxiliary space.

      Method 2: Bitwise XOR Accumulation
        1. Exploit the properties of XOR: x ^ x = 0 and x ^ 0 = x.
        2. Compute the XOR sum of all elements in array A, and XOR that with
           the XOR sum of all integers from 1 to n - 1:
             k = (A[0] ^ A[1] ^ ... ^ A[n-1]) ^ (1 ^ 2 ^ ... ^ (n-1))
        3. Every unique integer appears twice (once in A, once in 1..n-1) and
   cancels out to 0. The duplicate integer appears three times (twice in A, once
   in 1..n-1), leaving k ^ k ^ k = k.
        - Running time : O(n) — single linear pass over the array.
        - Space usage  : O(1) auxiliary space.
        - Advantage    : Completely immune to arithmetic overflow.
*/

#include <algorithm> // For std::shuffle
#include <cassert>   // For testing
#include <cstddef>   // For std::size_t
#include <cstdint>   // For std::int64_t
#include <iostream>
#include <iterator> // std::begin() and std::end()
#include <numeric>  // For std::accumulate
#include <random>   // For std::mt19937 (random number generator for shuffling)
#include <stdexcept>
#include <type_traits> // std::is_integral_v<T>
#include <vector>      // For std::vector

// =====================================================================
// Method 1: Sum Difference
// =====================================================================

// Core: iterator-pair version. Requires forward iterators (we traverse twice:
// once for distance, once for accumulate).
template <typename ForwardIt>
auto findDuplicateSum(ForwardIt first, ForwardIt last) {
  using T = typename std::iterator_traits<ForwardIt>::value_type;
  static_assert(std::is_integral_v<T>,
                "Element type must be an integral type.");

  const auto n = static_cast<std::uintmax_t>(std::distance(first, last));
  if (n < 2) {
    throw std::invalid_argument("Range must contain at least 2 elements.");
  }

  // Expected sum of 1..n-1 = n(n-1)/2. Halve the even factor *before*
  // multiplying so the intermediate product cannot overflow needlessly.
  const std::uintmax_t expected =
      (n % 2 == 0) ? (n / 2) * (n - 1) : n * ((n - 1) / 2);

  // Unsigned arithmetic wraps modulo 2^k with well-defined behaviour, so the
  // final difference is exact even if the running sums wrap around.
  const std::uintmax_t actual = std::accumulate(
      first, last, std::uintmax_t{0}, [](std::uintmax_t acc, T v) {
        return acc + static_cast<std::uintmax_t>(v);
      });

  return static_cast<T>(actual - expected);
}

// Convenience overload: std::vector, std::array, std::list, C arrays, ...
template <typename Container> auto findDuplicateSum(const Container &c) {
  return findDuplicateSum(std::begin(c), std::end(c));
}
// =====================================================================
// Method 2: Bitwise XOR
// =====================================================================
namespace detail {
// XOR of all integers 0..m in O(1).
constexpr std::uintmax_t xorUpTo(std::uintmax_t m) noexcept {
  switch (m & 3u) {
  case 0:
    return m;
  case 1:
    return 1;
  case 2:
    return m + 1;
  default:
    return 0;
  }
}
} // namespace detail

// Core: iterator-pair version. Works with any input iterator (single pass).
template <typename ForwardIt>
auto findDuplicateXOR(ForwardIt first, ForwardIt last) {
  using T = typename std::iterator_traits<ForwardIt>::value_type;
  static_assert(std::is_integral_v<T>,
                "Element type must be an integral type.");
  using U = std::make_unsigned_t<T>;

  U acc = 0;
  std::uintmax_t n = 0;
  for (; first != last; ++first, ++n) {
    acc ^= static_cast<U>(*first);
  }

  if (n < 2) {
    throw std::invalid_argument("Range must contain at least 2 elements.");
  }

  // Cancel the values 1..n-1 (the expected permutation).
  acc ^= static_cast<U>(detail::xorUpTo(n - 1));
  return static_cast<T>(acc);
}

// Convenience overload: std::vector, std::array, std::list, C arrays, ...
template <typename Container> auto findDuplicateXOR(const Container &c) {
  return findDuplicateXOR(std::begin(c), std::end(c));
}

// =====================================================================
// Tests & Verification
// =====================================================================
int main() {
  std::cout << "=== C-3.3: Finding the Repeated Integer in an Array ===\n\n";

  // -----------------------------------------------------------------
  // Test 1: Minimal size (n = 2), values from 1 to 1 -> {1, 1}
  // -----------------------------------------------------------------
  {
    int A[] = {1, 1};
    int dupSum = findDuplicateSum(A);
    int dupXOR = findDuplicateXOR(A);
    assert(dupSum == 1);
    assert(dupXOR == 1);
    std::cout << "Test 1 Passed (n = 2): A = {1, 1} -> Duplicate = " << dupSum
              << "\n";
  }

  // -----------------------------------------------------------------
  // Test 2: Standard case (n = 5), values from 1 to 4 -> {1, 3, 4, 2, 2}
  // -----------------------------------------------------------------
  {
    std::vector<int> A = {1, 3, 4, 2, 2};
    int dupSum = findDuplicateSum(A);
    int dupXOR = findDuplicateXOR(A);
    assert(dupSum == 2);
    assert(dupXOR == 2);
    std::cout << "Test 2 Passed (n = 5): A = {1, 3, 4, 2, 2} -> Duplicate = "
              << dupSum << "\n";
  }

  // -----------------------------------------------------------------
  // Test 3: Duplicate at boundary positions (first and last elements)
  // -----------------------------------------------------------------
  {
    std::vector<int> A = {5, 1, 4, 3, 2, 5}; // n = 6, range 1..5, dup = 5
    int dupSum = findDuplicateSum(A);
    int dupXOR = findDuplicateXOR(A);
    assert(dupSum == 5);
    assert(dupXOR == 5);
    std::cout << "Test 3 Passed (n = 6): A = {5, 1, 4, 3, 2, 5} -> Duplicate = "
              << dupSum << "\n";
  }

  // -----------------------------------------------------------------
  // Test 4: Exception on invalid size (n < 2)
  // -----------------------------------------------------------------
  {
    bool caught = false;
    try {
      std::vector<int> A = {1};
      findDuplicateSum(A);
    } catch (const std::invalid_argument &) {
      caught = true;
    }
    assert(caught);
    std::cout << "Test 4 Passed: Exception thrown as expected when n < 2.\n";
  }

  // -----------------------------------------------------------------
  // Test 5: Large array stress test (n = 100,000)
  // -----------------------------------------------------------------
  {
    constexpr std::size_t N = 100000;
    constexpr int targetDup = 42424;

    std::vector<int> A;
    A.reserve(N);

    // Fill with numbers 1 to N - 1
    for (std::size_t i = 1; i < N; ++i) {
      A.push_back(static_cast<int>(i));
    }
    // Append the duplicate
    A.push_back(targetDup);

    // Shuffle the array to test arbitrary ordering
    std::mt19937 rng{12345};
    std::shuffle(A.begin(), A.end(), rng);

    int dupSum = findDuplicateSum(A);
    int dupXOR = findDuplicateXOR(A);
    assert(dupSum == targetDup);
    assert(dupXOR == targetDup);
    std::cout << "Test 5 Passed (n = 100,000): Large array stress test "
                 "correctly found duplicate "
              << dupSum << " using both methods.\n";
  }

  std::cout << "\nAll C-3.3 tests passed successfully!\n";
  return 0;
}
