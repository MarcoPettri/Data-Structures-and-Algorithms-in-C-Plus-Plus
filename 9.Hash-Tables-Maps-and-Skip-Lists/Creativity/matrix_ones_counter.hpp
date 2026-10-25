/**
 * @file matrix_ones_counter.hpp
 * @brief O(n log n) algorithm to count 1s in a row-monotonic binary matrix.
 */

#pragma once

#include <cstddef>
#include <vector>

/**
 * @brief Counts the number of 1s in a single row using binary search.
 *
 * @param row A row where all 1s come before any 0s.
 * @return size_t The number of 1s in the row.
 */
inline size_t countOnesInRow(const std::vector<int> &row) {
  if (row.empty())
    return 0;

  int l = 0;
  int h = static_cast<int>(row.size()) - 1;
  int last_one_idx = -1;

  // Binary search for the rightmost '1'
  while (l <= h) {
    int mid = l + (h - l) / 2;
    if (row[mid] == 1) {
      last_one_idx = mid;
      l = mid + 1; // Search right half for more 1s
    } else {
      h = mid - 1; // Search left half
    }
  }

  return static_cast<size_t>(last_one_idx + 1);
}

/**
 * @brief Counts the total number of 1s in an n x n matrix A in O(n log n) time.
 *
 * @param matrix The n x n matrix where each row has 1s followed by 0s.
 * @return size_t Total count of 1s.
 *
 * @section complexity Complexity Analysis
 * - **Time Complexity:** $O(n \log n)$. There are $n$ rows, and performing a
 * binary search on each row takes $O(\log n)$ time. Total time = $n \times
 * O(\log n) = O(n \log n)$.
 * - **Space Complexity:** $O(1)$ auxiliary space.
 */
inline size_t countOnesInMatrix(const std::vector<std::vector<int>> &matrix) {
  size_t total_ones = 0;
  for (const auto &row : matrix) {
    total_ones += countOnesInRow(row);
  }
  return total_ones;
}