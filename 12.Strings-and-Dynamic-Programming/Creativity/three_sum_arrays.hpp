/**
 * @file three_sum_arrays.hpp
 * @brief Algorithms for the 3-Sum problem across three arrays A, B, and C (C-12.25).
 */

#ifndef THREE_SUM_ARRAYS_HPP
#define THREE_SUM_ARRAYS_HPP

#include <algorithm>
#include <cstddef>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace dsa {

/**
 * @struct ThreeSumResult
 * @brief Represents the solution to the 3-Sum across arrays problem.
 * 
 * @tparam T Arithmetic type for array elements.
 */
template <typename T = int> struct ThreeSumResult {
  static_assert(std::is_arithmetic_v<T>,
                "ThreeSumResult requires an arithmetic type.");

  /// Whether a valid triplet was found.
  bool found{false};

  /// Values of the triplet (a, b, c).
  T a{0};
  T b{0};
  T c{0};

  /// 0-based indices in the original arrays A, B, and C.
  std::size_t index_a{0};
  std::size_t index_b{0};
  std::size_t index_c{0};

  [[nodiscard]] constexpr explicit operator bool() const noexcept {
    return found;
  }

  /**
     * @brief Verifies that the sum of the triplet equals the target x.
     * 
     * @param target The target value x.
     * @return [[nodiscard]] bool True if valid, false otherwise.
     */
  [[nodiscard]] constexpr bool verify(T target) const noexcept {
    if (!found)
      return false;
    return (a + b + c) == target;
  }
};

/**
 * @class ThreeSumSolver
 * @brief Solver for the 3-Sum problem across three arrays.
 */
class ThreeSumSolver {
public:
  /**
     * @brief Determines if there exist a in A, b in B, and c in C such that a + b + c = x
     *        using an O(n^2 log n)-time binary search algorithm.
     * 
     * Algorithm:
     * 1. Sort array C (with its original indices) in O(n log n) time.
     * 2. For each element a in A and b in B (n^2 pairs):
     *    - Compute target_c = x - a - b.
     *    - Binary search for target_c in the sorted C in O(log n) time.
     * 3. Return the triplet and original indices if found.
     * 
     * @tparam T Arithmetic type.
     * @param A First array of size n.
     * @param B Second array of size n.
     * @param C Third array of size n.
     * @param x Target sum.
     * @return [[nodiscard]] ThreeSumResult<T> Result structure.
     * 
     * @section complexity
     * - **Time Complexity:** $\mathcal{O}(n^2 \log n)$ worst-case.
     *   Sorting $C$ takes $\mathcal{O}(n \log n)$ time.
     *   Iterating over all $n^2$ pairs $(a, b) \in A \times B$ and performing binary search
     *   takes $n^2 \times \mathcal{O}(\log n) = \mathcal{O}(n^2 \log n)$ time.
     * - **Auxiliary Space:** $\mathcal{O}(n)$ auxiliary memory to store index-value pairs of $C$.
     */
  template <typename T = int>
  [[nodiscard]] static ThreeSumResult<T>
  solve_binary_search(const std::vector<T> &A, const std::vector<T> &B,
                      const std::vector<T> &C, T x) {
    ThreeSumResult<T> result;
    const std::size_t n_a = A.size();
    const std::size_t n_b = B.size();
    const std::size_t n_c = C.size();

    if (n_a == 0 || n_b == 0 || n_c == 0) {
      return result;
    }

    // Store C with original indices: pair<value, original_index>
    std::vector<std::pair<T, std::size_t>> sorted_c;
    sorted_c.reserve(n_c);
    for (std::size_t i = 0; i < n_c; ++i) {
      sorted_c.emplace_back(C[i], i);
    }

    // Sort C in O(n log n)
    std::sort(
        sorted_c.begin(), sorted_c.end(),
        [](const auto &lhs, const auto &rhs) { return lhs.first < rhs.first; });

    // Iterate over all pairs (a, b) in A x B: n^2 pairs
    for (std::size_t i = 0; i < n_a; ++i) {
      const T val_a = A[i];

      for (std::size_t j = 0; j < n_b; ++j) {
        const T val_b = B[j];
        const T target_c = x - val_a - val_b;

        // Binary search for target_c in sorted C: O(log n)
        auto it = std::lower_bound(
            sorted_c.begin(), sorted_c.end(), target_c,
            [](const auto &elem, const T &val) { return elem.first < val; });

        if (it != sorted_c.end() && it->first == target_c) {
          result.found = true;
          result.a = val_a;
          result.b = val_b;
          result.c = target_c;
          result.index_a = i;
          result.index_b = j;
          result.index_c = it->second;
          return result;
        }
      }
    }

    return result;
  }

  /**
     * @brief Optimal O(n^2)-time algorithm using sorting and two pointers.
     * 
     * Algorithm:
     * 1. Sort B and C with original indices in O(n log n) time.
     * 2. For each a in A:
     *    - Use two pointers on sorted B and C to find b + c == x - a in O(n) time.
     * 
     * @tparam T Arithmetic type.
     * @param A First array.
     * @param B Second array.
     * @param C Third array.
     * @param x Target sum.
     * @return [[nodiscard]] ThreeSumResult<T> Result structure.
     * 
     * @section complexity
     * - **Time Complexity:** $\mathcal{O}(n^2)$ worst-case.
     * - **Auxiliary Space:** $\mathcal{O}(n)$ auxiliary memory for index tracking.
     */
  template <typename T = int>
  [[nodiscard]] static ThreeSumResult<T>
  solve_two_pointers(const std::vector<T> &A, const std::vector<T> &B,
                     const std::vector<T> &C, T x) {
    ThreeSumResult<T> result;
    const std::size_t n_a = A.size();
    const std::size_t n_b = B.size();
    const std::size_t n_c = C.size();

    if (n_a == 0 || n_b == 0 || n_c == 0) {
      return result;
    }

    std::vector<std::pair<T, std::size_t>> sorted_b;
    sorted_b.reserve(n_b);
    for (std::size_t j = 0; j < n_b; ++j) {
      sorted_b.emplace_back(B[j], j);
    }
    std::sort(
        sorted_b.begin(), sorted_b.end(),
        [](const auto &lhs, const auto &rhs) { return lhs.first < rhs.first; });

    std::vector<std::pair<T, std::size_t>> sorted_c;
    sorted_c.reserve(n_c);
    for (std::size_t k = 0; k < n_c; ++k) {
      sorted_c.emplace_back(C[k], k);
    }
    std::sort(
        sorted_c.begin(), sorted_c.end(),
        [](const auto &lhs, const auto &rhs) { return lhs.first < rhs.first; });

    for (std::size_t i = 0; i < n_a; ++i) {
      const T val_a = A[i];
      const T target_bc = x - val_a;

      std::size_t left = 0;
      std::size_t right = n_c;

      while (left < n_b && right > 0) {
        const T sum_bc = sorted_b[left].first + sorted_c[right - 1].first;

        if (sum_bc == target_bc) {
          result.found = true;
          result.a = val_a;
          result.b = sorted_b[left].first;
          result.c = sorted_c[right - 1].first;
          result.index_a = i;
          result.index_b = sorted_b[left].second;
          result.index_c = sorted_c[right - 1].second;
          return result;
        } else if (sum_bc < target_bc) {
          ++left;
        } else {
          --right;
        }
      }
    }

    return result;
  }

  /**
     * @brief Finds all unique valid triplets (a, b, c) across arrays A, B, and C such that a + b + c = x.
     * 
     * @tparam T Arithmetic type.
     * @param A First array.
     * @param B Second array.
     * @param C Third array.
     * @param x Target sum.
     * @return [[nodiscard]] std::vector<ThreeSumResult<T>> List of all matching triplets.
     */
  template <typename T = int>
  [[nodiscard]] static std::vector<ThreeSumResult<T>>
  find_all_triplets(const std::vector<T> &A, const std::vector<T> &B,
                    const std::vector<T> &C, T x) {
    std::vector<ThreeSumResult<T>> all_results;
    const std::size_t n_a = A.size();
    const std::size_t n_b = B.size();
    const std::size_t n_c = C.size();

    if (n_a == 0 || n_b == 0 || n_c == 0) {
      return all_results;
    }

    std::vector<std::pair<T, std::size_t>> sorted_c;
    sorted_c.reserve(n_c);
    for (std::size_t k = 0; k < n_c; ++k) {
      sorted_c.emplace_back(C[k], k);
    }
    std::sort(
        sorted_c.begin(), sorted_c.end(),
        [](const auto &lhs, const auto &rhs) { return lhs.first < rhs.first; });

    for (std::size_t i = 0; i < n_a; ++i) {
      const T val_a = A[i];
      for (std::size_t j = 0; j < n_b; ++j) {
        const T val_b = B[j];
        const T target_c = x - val_a - val_b;

        const std::pair<T, std::size_t> query_target{target_c, 0};
        auto range =
            std::equal_range(sorted_c.begin(), sorted_c.end(), query_target,
                             [](const auto &lhs, const auto &rhs) {
                               return lhs.first < rhs.first;
                             });

        for (auto it = range.first; it != range.second; ++it) {
          ThreeSumResult<T> res;
          res.found = true;
          res.a = val_a;
          res.b = val_b;
          res.c = target_c;
          res.index_a = i;
          res.index_b = j;
          res.index_c = it->second;
          all_results.push_back(res);
        }
      }
    }

    return all_results;
  }
};

/**
 * @brief Convenience function to check whether a triplet exists summing to x using O(n^2 log n) binary search.
 * 
 * @tparam T Arithmetic type.
 * @param A Array A of size n.
 * @param B Array B of size n.
 * @param C Array C of size n.
 * @param x Target sum.
 * @return [[nodiscard]] ThreeSumResult<T> Solution result.
 */
template <typename T = int>
[[nodiscard]] inline ThreeSumResult<T>
three_sum_arrays(const std::vector<T> &A, const std::vector<T> &B,
                 const std::vector<T> &C, T x) {
  return ThreeSumSolver::solve_binary_search(A, B, C, x);
}

} // namespace dsa

#endif // THREE_SUM_ARRAYS_HPP
