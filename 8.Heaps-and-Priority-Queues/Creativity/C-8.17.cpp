// File : C-8.17.cpp

/*
C-8.17 Develop an algorithm that computes the kth smallest element of a set of n
distinct integers in O(n + k log n) time.
*/

#include <algorithm>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <vector>

/**
 * @brief Finds the k-th smallest element in a vector of distinct integers.
 *
 * @param data Input vector of integers (passed by value to preserve original
 * input).
 * @param k The 1-based index of the smallest element to find (1 <= k <= n).
 * @return int The k-th smallest element.
 * @throws std::invalid_argument If k is out of bounds (k < 1 or k > n).
 *
 * @section complexity Complexity Analysis
 * - **Time Complexity:** $O(n + k \log n)$.
 *   - $O(n)$ to build the min-heap using Floyd's algorithm.
 *   - $O(k \log n)$ to perform $k$ pop operations.
 * - **Space Complexity:** $O(n)$ to store the working copy of the elements.
 */
inline int findKthSmallest(std::vector<int> data, size_t k) {
  if (k < 1 || k > data.size()) {
    throw std::invalid_argument(
        "k must be between 1 and the number of elements.");
  }

  // Step 1: Build Min-Heap in O(n) time
  // std::greater<int> creates a min-heap instead of the default max-heap
  std::make_heap(data.begin(), data.end(), std::greater<int>());

  // Step 2: Extract minimum k - 1 times in O((k - 1) log n) time
  for (size_t i = 1; i < k; ++i) {
    std::pop_heap(data.begin(), data.end() - (i - 1), std::greater<int>());
  }

  // The k-th smallest element is now at the root of the heap (index 0)
  return data.front();
}

void runTests() {
  std::cout << "Running k-th smallest element tests...\n";

  std::vector<int> input = {12, 3, 5, 7, 19, 1, 8, 15};
  // Sorted order: 1, 3, 5, 7, 8, 12, 15, 19

  // Test 1: k = 1 (Minimum element)
  assert(findKthSmallest(input, 1) == 1);

  // Test 2: k = 2
  assert(findKthSmallest(input, 2) == 3);

  // Test 3: k = 5 (Middle element)
  assert(findKthSmallest(input, 5) == 8);

  // Test 4: k = 8 (Maximum element)
  assert(findKthSmallest(input, 8) == 19);

  // Test 5: Out of bounds error handling
  bool exceptionCaught = false;
  try {
    findKthSmallest(input, 0);
  } catch (const std::invalid_argument &) {
    exceptionCaught = true;
  }
  assert(exceptionCaught);

  std::cout << "All k-th smallest element tests passed successfully!\n";
}

int main() {
  runTests();
  return 0;
}