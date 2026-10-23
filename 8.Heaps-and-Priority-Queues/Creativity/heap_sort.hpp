/**
 * @file heap_sort.hpp
 * @brief In-place heap sort implementation in C++17.
 */

#pragma once

#include <iterator>
#include <utility>
#include <vector>

/**
 * @brief Sorts a container in-place using the heap sort algorithm.
 *
 * @tparam Container The type of the container (e.g., std::vector<T>).
 * @param A The array/container of elements to be sorted.
 *
 * @section complexity Complexity Analysis
 * - **Time Complexity:** $O(N \log N)$ for all cases (best, average, and
 * worst), where $N$ is the number of elements. Building the heap takes $O(N)$
 * time, and each of the $N-1$ extractions takes $O(\log N)$ time.
 * - **Space Complexity:** $O(1)$ auxiliary space. Sorting is performed entirely
 *   in-place using only a fixed number of scalar index variables (<= 5).
 */
template <typename Container> void heapSort(Container &A) {
  if (A.size() <= 1) {
    return;
  }

  const size_t n = A.size();

  // Helper lambda for sifting down an element at index i within a heap of size
  // heap_size. Uses 3 base-type variables: largest, left, right.
  auto siftDown = [&A](size_t heap_size, size_t i) {
    size_t largest = i;
    while (true) {
      size_t left = 2 * i + 1;
      size_t right = 2 * i + 2;

      if (left < heap_size && A[left] > A[largest]) {
        largest = left;
      }
      if (right < heap_size && A[right] > A[largest]) {
        largest = right;
      }
      if (largest == i) {
        break;
      }
      std::swap(A[i], A[largest]);
      i = largest;
    }
  };

  // Step 1: Build max heap (rearrange array)
  // Uses variable i (signed to safely handle countdown past 0)
  for (long long i = static_cast<long long>(n / 2) - 1; i >= 0; --i) {
    siftDown(n, static_cast<size_t>(i));
  }

  // Step 2: Extract elements from heap one by one
  // Uses variable i
  for (size_t i = n - 1; i > 0; --i) {
    std::swap(A[0], A[i]);
    siftDown(i, 0);
  }
}