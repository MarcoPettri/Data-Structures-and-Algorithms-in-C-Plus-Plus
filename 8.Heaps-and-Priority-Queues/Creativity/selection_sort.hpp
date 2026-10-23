/**
 * @file selection_sort.hpp
 * @brief In-place selection sort implementation in C++17.
 */

#pragma once

#include <iterator>
#include <utility>
#include <vector>

/**
 * @brief Sorts a container in-place using the selection sort algorithm.
 *
 * @tparam Container The type of the container (e.g., std::vector<T>).
 * @param data The container of elements to be sorted.
 *
 * @section complexity Complexity Analysis
 * - **Time Complexity:** $O(N^2)$ for all cases (best, average, and worst),
 *   where $N$ is the number of elements in the container. The algorithm always
 *   performs nested scans regardless of the initial order.
 * - **Space Complexity:** $O(1)$ auxiliary space. Sorting is performed entirely
 *   in-place using only scalar loop indices and swap operations.
 */
template <typename Container> void selectionSort(Container &data) {
  if (data.empty()) {
    return;
  }

  const size_t n = data.size();

  // Move the boundary of the unsorted subarray one by one
  for (size_t i = 0; i < n - 1; ++i) {
    size_t min_index = i;

    // Find the index of the minimum element in the remaining unsorted array
    for (size_t j = i + 1; j < n; ++j) {
      if (data[j] < data[min_index]) {
        min_index = j;
      }
    }

    // Swap the found minimum element with the first element of the unsorted
    // region
    if (min_index != i) {
      std::swap(data[i], data[min_index]);
    }
  }
}