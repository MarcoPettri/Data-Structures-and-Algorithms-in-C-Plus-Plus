// File: R-3.11.cpp
/*

R-3.11 Describe a recursive algorithm for finding the maximum element in an
array A of n elements. What is your running
time and space usage?

Answer:
  Algorithm (divide-and-conquer):
    findMax(A, lo, hi):
      if lo == hi: return A[lo]          // base case: single element
      mid = (lo + hi) / 2
      leftMax  = findMax(A, lo,    mid)  // recurse left half
      rightMax = findMax(A, mid+1, hi)   // recurse right half
      return max(leftMax, rightMax)

  Running time : O(n) — every element is visited exactly once.
  Space usage  : O(log n) — the recursion depth is log2(n) (binary split),
                 each frame uses O(1) extra space.

*/
#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>

// Recursive helper: finds the maximum in A[lo..hi] (inclusive).
template <typename T>
const T &findMax(const std::unique_ptr<T[]> &A, std::size_t lo,
                 std::size_t hi) {
  if (lo == hi)
    return A[lo]; // base case

  std::size_t mid = lo + (hi - lo) / 2;
  const T &leftMax = findMax(A, lo, mid);
  const T &rightMax = findMax(A, mid + 1, hi);
  return leftMax > rightMax ? leftMax : rightMax;
}

// Public interface: finds the maximum in an n-element smart-pointer array.
template <typename T>
const T &findMax(const std::unique_ptr<T[]> &A, std::size_t n) {
  if (n == 0)
    throw std::invalid_argument("findMax called on empty array");
  return findMax(A, 0, n - 1);
}

int main() {
  // ── test 1: general case ──────────────────────────────────────────────
  constexpr std::size_t N = 8;
  auto A = std::make_unique<int[]>(N);
  int values[] = {3, 7, 1, 9, 4, 6, 2, 8};
  for (std::size_t i = 0; i < N; ++i)
    A[i] = values[i];

  std::cout << "Array:   ";
  for (std::size_t i = 0; i < N; ++i)
    std::cout << A[i] << " ";
  std::cout << "\nMax = " << findMax(A, N) << "\n\n"; // expected: 9

  // ── test 2: single element ────────────────────────────────────────────
  auto B = std::make_unique<int[]>(1);
  B[0] = 42;
  std::cout << "Single-element {42}       -> max = " << findMax(B, 1)
            << "\n"; // 42

  // ── test 3: already-sorted descending ────────────────────────────────
  constexpr std::size_t M = 5;
  auto C = std::make_unique<int[]>(M);
  int desc[] = {10, 8, 6, 4, 2};
  for (std::size_t i = 0; i < M; ++i)
    C[i] = desc[i];
  std::cout << "Descending {10,8,6,4,2}   -> max = " << findMax(C, M)
            << "\n"; // 10

  return 0;
}
