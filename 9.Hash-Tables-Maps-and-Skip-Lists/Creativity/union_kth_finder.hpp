/**
 * @file union_kth_finder.hpp
 * @brief Algorithm to find the k-th smallest key in the union of two ordered
 * arrays in O(log^2 n) time.
 */

#pragma once

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <vector>

/**
 * @brief Helper function to search for the k-th element by binary searching S.
 */
template <typename T>
std::optional<T> findKthInPrimary(const std::vector<T> &S,
                                  const std::vector<T> &T_seq, size_t k) {
  long long l = 0;
  long long h = static_cast<long long>(S.size()) - 1;

  while (l <= h) {
    long long mid = l + (h - l) / 2;
    const T &val = S[static_cast<size_t>(mid)];

    // Count elements in T_seq that are <= val using binary search
    // (std::upper_bound)
    auto it = std::upper_bound(T_seq.begin(), T_seq.end(), val);
    size_t count_T = static_cast<size_t>(std::distance(T_seq.begin(), it));

    // 1-based rank in the union S union T_seq
    size_t total_rank = static_cast<size_t>(mid + 1) + count_T;

    if (total_rank == k) {
      return val;
    } else if (total_rank < k) {
      l = mid + 1;
    } else {
      h = mid - 1;
    }
  }
  return std::nullopt;
}

/**
 * @brief Finds the k-th smallest key in the union of two ordered sequences S
 * and T.
 *
 * @tparam T The element type (must support comparison operators).
 * @param S First ordered array/sequence.
 * @param T_seq Second ordered array/sequence.
 * @param k The 1-based index of the smallest key to find in the union.
 * @return T The k-th smallest key.
 * @throws std::out_of_range If k is out of the valid range [1, S.size() +
 * T_seq.size()].
 *
 * @section complexity Complexity Analysis
 * - **Time Complexity:** $O(\log^2 n)$ (or more generally $O(\log n \cdot \log
 * m)$), where $n$ and $m$ are the sizes of sequences $S$ and $T$. The outer
 * binary search runs in $O(\log n)$ steps, and each step performs an inner
 * binary search (`std::upper_bound`) taking $O(\log m)$ time.
 * - **Space Complexity:** $O(1)$ auxiliary space, using only scalar tracking
 * variables.
 */
template <typename T>
T findKthSmallestInUnion(const std::vector<T> &S, const std::vector<T> &T_seq,
                         size_t k) {
  size_t total_size = S.size() + T_seq.size();
  if (k < 1 || k > total_size) {
    throw std::out_of_range(
        "k is out of the valid range for the union of S and T.");
  }

  // Try searching assuming the k-th element is in S
  if (auto result = findKthInPrimary(S, T_seq, k); result.has_value()) {
    return *result;
  }

  // Otherwise, it must be in T_seq; search symmetrically
  if (auto result = findKthInPrimary(T_seq, S, k); result.has_value()) {
    return *result;
  }

  throw std::runtime_error(
      "Unexpected error: k-th element could not be located.");
}