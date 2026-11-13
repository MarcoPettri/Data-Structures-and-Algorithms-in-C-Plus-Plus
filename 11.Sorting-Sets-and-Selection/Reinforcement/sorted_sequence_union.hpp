
/**
 * @file sorted_sequence_union.hpp
 * @brief  algorithm for computing the set union of 
 *        two sorted sequences containing duplicate entries in linear time.
 */

#ifndef SORTED_SEQUENCE_UNION_HPP
#define SORTED_SEQUENCE_UNION_HPP

#include <iterator>
#include <type_traits>
#include <vector>

namespace dsa {

/**
 * @brief Computes the set union $A \cup B$ (with no duplicates) from two sorted input sequences.
 * 
 * @tparam InputIt1 Iterator type for sequence A.
 * @tparam InputIt2 Iterator type for sequence B.
 * @tparam Container Output container type (defaults to std::vector of element type).
 * @param first1 Iterator to the beginning of sequence A.
 * @param last1 Iterator to the end of sequence A.
 * @param first2 Iterator to the beginning of sequence B.
 * @param last2 Iterator to the end of sequence B.
 * @return [[nodiscard]] Container storing the sorted unique union of A and B.
 * 
 * @section complexity
 * - **Time Complexity:** $\mathcal{O}(n + m)$, where $n = \text{distance}(first1, last1)$ and $m = \text{distance}(first2, last2)$. Each element from both sequences is inspected and advanced at most once, and duplicate elimination takes $\mathcal{O}(1)$ amortized time per insertion.
 * - **Space Complexity:** $\mathcal{O}(n + m)$ auxiliary space to store the resulting unique union sequence in the output container.
 */
template <typename InputIt1, typename InputIt2,
          typename Container =
              std::vector<typename std::iterator_traits<InputIt1>::value_type>>
[[nodiscard]] Container
compute_sorted_sequence_union(InputIt1 first1, InputIt1 last1, InputIt2 first2,
                              InputIt2 last2) {
  using value_type = typename std::iterator_traits<InputIt1>::value_type;
  static_assert(
      std::is_same_v<value_type,
                     typename std::iterator_traits<InputIt2>::value_type>,
      "Sequence element types must match.");

  Container result;

  // Helper lambda to append unique elements and suppress duplicates
  auto append_unique = [&result](const value_type &val) {
    if (result.empty() || result.back() != val) {
      result.push_back(val);
    }
  };

  // Two-pointer linear scan technique
  while (first1 != last1 && first2 != last2) {
    if (*first1 < *first2) {
      append_unique(*first1);
      ++first1;
    } else if (*first2 < *first1) {
      append_unique(*first2);
      ++first2;
    } else {
      // *first1 == *first2
      append_unique(*first1);
      ++first1;
      ++first2;
    }
  }

  // Process remaining elements in sequence A, if any
  while (first1 != last1) {
    append_unique(*first1);
    ++first1;
  }

  // Process remaining elements in sequence B, if any
  while (first2 != last2) {
    append_unique(*first2);
    ++first2;
  }

  return result;
}

} // namespace dsa

#endif // SORTED_SEQUENCE_UNION_HPP