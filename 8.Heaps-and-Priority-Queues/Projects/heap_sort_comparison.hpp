/**
 * @file heap_sort_comparison.hpp
 * @brief Implementation and comparison of in-place heap sort vs external heap
 * sort in C++17.
 */

#pragma once

#include <iterator>
#include <queue>
#include <utility>
#include <vector>

/**
 * @brief Sorts a container in-place using the heap sort algorithm.
 *
 * @tparam Container The type of the container (e.g., std::vector<T>).
 * @param A The array/container of elements to be sorted.
 *
 * @section complexity Complexity Analysis (In-Place)
 * - **Time Complexity:** O(N log N) for all cases. Building the heap takes O(N)
 *   using Floyd's bottom-up method, and the N-1 extractions take O(N log N).
 * - **Space Complexity:** O(1) auxiliary space. Sorting is performed directly
 *   within the input array using only scalar variables.
 */
template <typename Container> void inPlaceHeapSort(Container &A) {
  if (A.size() <= 1) {
    return;
  }

  const size_t n = A.size();

  // Sift down lambda helper for in-place array representation
  auto siftDown = [&A](size_t heap_size, size_t i) {
    while (true) {
      size_t largest = i;
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

  // Step 1: Build max-heap in-place in O(N) time
  for (long long i = static_cast<long long>(n / 2) - 1; i >= 0; --i) {
    siftDown(n, static_cast<size_t>(i));
  }

  // Step 2: Extract elements one by one in O(N log N) time
  for (size_t i = n - 1; i > 0; --i) {
    std::swap(A[0], A[i]);
    siftDown(i, 0);
  }
}

/**
 * @brief Sorts a container using a standard heap sort with an external heap
 * structure.
 *
 * @tparam Container The type of the container.
 * @param A The array/container of elements to be sorted.
 *
 * @section complexity Complexity Analysis (External Heap)
 * - **Time Complexity:** O(N log N). Pushing N elements into an external
 * priority queue and extracting them back takes O(N log N) time.
 * - **Space Complexity:** O(N) auxiliary space, because it requires allocating
 *   a separate external data structure (`std::priority_queue`).
 */
template <typename Container> void externalHeapSort(Container &A) {
  if (A.size() <= 1) {
    return;
  }

  using value_type = typename Container::value_type;

  // External heap container (std::priority_queue utilizes a separate underlying
  // vector/heap)
  std::priority_queue<value_type> ext_pq;

  // Phase 1: Push all elements into the external priority queue: O(N log N)
  for (const auto &val : A) {
    ext_pq.push(val);
  }

  // Phase 2: Extract elements from the external heap back into the array: O(N
  // log N)
  for (long long i = static_cast<long long>(A.size()) - 1; i >= 0; --i) {
    A[static_cast<size_t>(i)] = ext_pq.top();
    ext_pq.pop();
  }
}