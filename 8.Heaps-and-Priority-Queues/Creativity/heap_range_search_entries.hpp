/**
 * @file heap_range_search_entries.hpp
 * @brief Algorithm to report all entries in a min-heap with key <= k in O(m)
 * time.
 */

#pragma once

#include <utility>
#include <vector>

/**
 * @struct HeapEntry
 * @brief Represents a key-value entry stored in the heap.
 */
template <typename Key, typename Value> struct HeapEntry {
  Key key;
  Value value;

  bool operator==(const HeapEntry &other) const {
    return key == other.key && value == other.value;
  }
};

/**
 * @brief Helper function to recursively search the min-heap for entries with
 * key <= k.
 */
template <typename Key, typename Value>
void findEntriesLTEHelper(const std::vector<HeapEntry<Key, Value>> &heap,
                          size_t index, const Key &k,
                          std::vector<HeapEntry<Key, Value>> &result) {
  // Base Case: Index out of bounds or node key exceeds query key k (Prune
  // subtree)
  if (index >= heap.size() || heap[index].key > k) {
    return;
  }

  // Report current node entry
  result.push_back(heap[index]);

  // Recurse on left child (2 * index + 1) and right child (2 * index + 2)
  findEntriesLTEHelper(heap, 2 * index + 1, k, result);
  findEntriesLTEHelper(heap, 2 * index + 2, k, result);
}

/**
 * @brief Computes all entries in a min-heap T with a key less than or equal to
 * k.
 *
 * @tparam Key The key type.
 * @tparam Value The value type.
 * @param heap The min-heap represented as an array/vector of HeapEntry objects.
 * @param k The query key threshold.
 * @return std::vector<HeapEntry<Key, Value>> Collection of all entries with key
 * <= k.
 *
 * @section complexity Complexity Analysis
 * - **Time Complexity:** $O(m)$, where $m$ is the number of entries returned.
 *   Each recursive step either reports a valid entry or hits a boundary node $>
 * k$ that terminates the branch, avoiding unnecessary scans of the entire heap.
 * - **Space Complexity:** $O(m)$ for storing output results and call stack
 * depth (which is at most $O(\log n)$ for a balanced complete binary tree
 * heap).
 */
template <typename Key, typename Value>
std::vector<HeapEntry<Key, Value>>
findEntriesLessThanOrEqual(const std::vector<HeapEntry<Key, Value>> &heap,
                           const Key &k) {
  std::vector<HeapEntry<Key, Value>> result;
  if (!heap.empty()) {
    findEntriesLTEHelper(heap, 0, k, result);
  }
  return result;
}