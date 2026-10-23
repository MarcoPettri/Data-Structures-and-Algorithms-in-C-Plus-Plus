/**
 * @file heap_range_search.hpp
 * @brief Algorithm to report all keys in a min-heap <= x in O(k) time.
 */

#pragma once

#include <cstddef>
#include <vector>

/**
 * @brief Recursively searches the min-heap for elements <= x.
 *
 * @tparam T Element type.
 * @param heap Vector representing the min-heap array.
 * @param index Current node index being inspected.
 * @param x The query threshold key.
 * @param result Vector storing the reported keys.
 */
template <typename T>
void reportKeysHelper(const std::vector<T> &heap, size_t index, const T &x,
                      std::vector<T> &result) {
  // Base Case: Index out of bounds or node key exceeds query key x (Prune
  // subtree)
  if (index >= heap.size() || heap[index] > x) {
    return;
  }

  // Report current node key
  result.push_back(heap[index]);

  // Recurse on left child (2 * index + 1) and right child (2 * index + 2)
  reportKeysHelper(heap, 2 * index + 1, x, result);
  reportKeysHelper(heap, 2 * index + 2, x, result);
}

/**
 * @brief Reports all keys in a min-heap T that are smaller than or equal to
 * query key x.
 *
 * @tparam T Element type stored in the heap.
 * @param heap The input min-heap represented as an array/vector.
 * @param x The query key threshold.
 * @return std::vector<T> Collection of all keys <= x.
 *
 * @section complexity Complexity Analysis
 * - **Time Complexity:** $O(k)$, where $k$ is the number of keys reported.
 *   Each recursive step either reports an element or hits a boundary node $>
 * x$. The total work is directly proportional to $k$.
 * - **Space Complexity:** $O(k)$ for storing output results and call stack
 * depth.
 */
template <typename T>
std::vector<T> reportKeysLessThanOrEqual(const std::vector<T> &heap,
                                         const T &x) {
  std::vector<T> result;
  if (!heap.empty()) {
    reportKeysHelper(heap, 0, x, result);
  }
  return result;
}