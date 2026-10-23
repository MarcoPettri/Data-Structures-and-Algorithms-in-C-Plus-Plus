/**
 * @file insertion_sort.hpp
 * @brief In-place insertion sort implementation in C++17.
 */

#pragma once

#include <iterator>
#include <vector>

/**
 * @brief Sorts a container in-place using the insertion sort algorithm.
 *
 * @tparam Container The type of the container (e.g., std::vector<T>).
 * @param A The array/container of elements to be sorted.
 *
 * @section complexity Complexity Analysis
 * - **Time Complexity:**
 *   - **Best Case:** $O(N)$ when the array is already sorted (the inner loop
 * never executes).
 *   - **Worst & Average Case:** $O(N^2)$ when the array is in reverse order or
 * randomly ordered.
 * - **Space Complexity:** $O(1)$ auxiliary space. Sorting is performed entirely
 *   in-place using only three scalar variables (`i`, `j`, and `key`).
 */
template <typename Container> void insertionSort(Container &A) {
  if (A.size() <= 1) {
    return;
  }

  using value_type = typename Container::value_type;

  // i (outer loop index) and j (inner loop index) + key = 3 additional
  // variables
  for (size_t i = 1; i < A.size(); ++i) {
    value_type key = A[i];
    size_t j = i;

    // Shift elements of the sorted segment to the right that are greater than
    // key
    while (j > 0 && A[j - 1] > key) {
      A[j] = A[j - 1];
      --j;
    }

    // Insert the key into its correct position
    A[j] = key;
  }
}