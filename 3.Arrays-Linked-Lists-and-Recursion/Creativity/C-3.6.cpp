// File: C-3.6.cpp
/*
    C-3.6 Give a recursive algorithm to compute the product of two positive
    integers, m and n, using only addition and subtraction.

    Answer:
      Multiplication is repeated addition: m * n = m + m + ... + m (n times).

      Method 1: Linear Recursion
        Base case     : n == 1  ->  product(m, 1) = m
        Recursive case: n > 1   ->  product(m, n) = m + product(m, n - 1)
        - Optimization: swap so that n <= m, so the recursion depth is
          min(m, n) rather than n.
        - Running time : O(min(m, n)) additions
        - Space usage  : O(min(m, n)) for the call stack

      Method 2: Doubling (Ancient Egyptian multiplication)
        Keep a pair (a, b) with a = m * b, starting at (m, 1). Doubling both
        needs only addition: (a + a, b + b) = (m * 2b, 2b).
          1. Keep doubling while 2b <= n.
          2. Now b is the largest power of two <= n, and a = m * b. The
             remainder r = n - b satisfies r < b.
          3. Result = a + product(m, r)   (or just a if r == 0).
        - Running time : O(log^2 n) additions (O(log n) levels, each
          doubling up to O(log n) times)
        - Space usage  : O(log n) for the call stack
        - Only addition, subtraction and comparison are used; no division,
          no shifts.
*/

#include <cassert> //
#include <iostream>
#include <stdexcept>   // For std::invalid_argument
#include <type_traits> // std::is_integral_v<T>
#include <utility>     // For std::swap

// =====================================================================
// Method 1: Linear Recursion
// =====================================================================
namespace detail {
// Precondition: m >= 1, n >= 1.
template <typename T> T mulLinear(T m, T n) {
  if (n == 1) {
    return m; // base case
  }
  return m + mulLinear(m, n - 1);
}
} // namespace detail

template <typename T> T multiplyLinear(T m, T n) {
  static_assert(std::is_integral_v<T>, "Operands must be integral types.");
  if (m < 1 || n < 1) {
    throw std::invalid_argument("Both operands must be positive.");
  }
  if (n > m) {
    std::swap(m, n); // recurse on the smaller operand
  }
  return detail::mulLinear(m, n);
}

// =====================================================================
// Method 2: Doubling
// =====================================================================
namespace detail {
template <typename T> T mulDoubling(T m, T rem); // forward declaration

// Invariant: a == m * b, 1 <= b <= rem.
template <typename T> T doubleUp(T m, T a, T b, T rem) {
  // "b > rem - b" is the same as "2b > rem" but cannot overflow.
  if (b > rem - b) {
    const T r = rem - b; // 0 <= r < b
    return (r == 0) ? a : a + mulDoubling(m, r);
  }
  return doubleUp(m, a + a, b + b, rem); // safe: 2b <= rem
}

// Precondition: m >= 1, rem >= 1.
template <typename T> T mulDoubling(T m, T rem) {
  return doubleUp(m, m, T{1}, rem);
}
} // namespace detail

template <typename T> T multiplyDoubling(T m, T n) {
  static_assert(std::is_integral_v<T>, "Operands must be integral types.");
  if (m < 1 || n < 1) {
    throw std::invalid_argument("Both operands must be positive.");
  }
  return detail::mulDoubling(m, n);
}

// =====================================================================
// Tests & Verification
// =====================================================================
namespace {
void check(long long m, long long n) {
  const long long expected =
      m * n; // the reference uses '*'; the algorithms don't
  assert(multiplyLinear(m, n) == expected);
  assert(multiplyDoubling(m, n) == expected);
}
} // namespace

int main() {
  std::cout << "=== C-3.6: Recursive Multiplication via Add/Subtract ===\n\n";

  // Test 1: Smallest operands
  check(1, 1);
  std::cout << "Test 1 Passed: 1 * 1 = 1\n";

  // Test 2: Standard cases
  check(3, 4);
  check(12, 12);
  std::cout << "Test 2 Passed: 3 * 4 = " << multiplyLinear(3, 4)
            << ", 12 * 12 = " << multiplyDoubling(12, 12) << "\n";

  // Test 3: One operand is 1 (both orders)
  check(7, 1);
  check(1, 7);
  std::cout << "Test 3 Passed: multiplying by 1 in both orders\n";

  // Test 4: Powers of two and values just around them
  check(5, 64);
  check(5, 63);
  check(5, 65);
  check(1024, 1024);
  std::cout << "Test 4 Passed: powers of two and neighbors\n";

  // Test 5: Exhaustive small range
  for (long long m = 1; m <= 60; ++m) {
    for (long long n = 1; n <= 60; ++n) {
      check(m, n);
    }
  }
  std::cout << "Test 5 Passed: all pairs 1..60 x 1..60 agree with '*'\n";

  // Test 6: Exceptions on non-positive input
  {
    bool zeroCaught = false, negCaught = false;
    try {
      multiplyLinear(0, 5);
    } catch (const std::invalid_argument &) {
      zeroCaught = true;
    }
    try {
      multiplyDoubling(5, -3);
    } catch (const std::invalid_argument &) {
      negCaught = true;
    }
    assert(zeroCaught && negCaught);
    std::cout << "Test 6 Passed: exceptions thrown for non-positive input.\n";
  }

  // Test 7: Large operands (the doubling method shines here)
  {
    const long long m = 123456, n = 789012;
    assert(multiplyDoubling(m, n) == m * n);
    assert(multiplyLinear(m, n) == m * n); // recurses ~123,456 deep: fine
    std::cout << "Test 7 Passed: " << m << " * " << n << " = "
              << multiplyDoubling(m, n) << "\n";
  }

  std::cout << "\nAll C-3.6 tests passed successfully!\n";
  return 0;
}