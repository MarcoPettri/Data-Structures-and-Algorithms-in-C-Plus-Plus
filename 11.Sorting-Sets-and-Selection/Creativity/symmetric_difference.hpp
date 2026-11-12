/**
 * @file symmetric_difference.hpp
 * @brief  Algorithm for computing the symmetric difference 
 *        A ⊕ B of two sorted sequences representing sets.
 */

#ifndef SYMMETRIC_DIFFERENCE_HPP
#define SYMMETRIC_DIFFERENCE_HPP

#include <vector>
#include <iterator>
#include <type_traits>
#include <algorithm>

namespace dsa {

/**
 * @brief Computes the symmetric difference A ⊕ B (elements in A or B, but not both) 
 *        from two sorted input sequences.
 * 
 * @tparam InputIt1 Iterator type for sequence A.
 * @tparam InputIt2 Iterator type for sequence B.
 * @tparam Container Output container type (defaults to std::vector of element type).
 * @param first1 Iterator to the beginning of sequence A.
 * @param last1 Iterator to the end of sequence A.
 * @param first2 Iterator to the beginning of sequence B.
 * @param last2 Iterator to the end of sequence B.
 * @return [[nodiscard]] Container storing the sorted symmetric difference of A and B.
 * 
 * @section complexity
 * - **Time Complexity:** $\mathcal{O}(n + m)$, where $n = \text{distance}(first1, last1)$ and $m = \text{distance}(first2, last2)$. Each element from both sequences is inspected and advanced at most once.
 * - **Space Complexity:** $\mathcal{O}(n + m)$ auxiliary space to store the resulting symmetric difference sequence in the output container.
 */
template <
    typename InputIt1, 
    typename InputIt2, 
    typename Container = std::vector<typename std::iterator_traits<InputIt1>::value_type>
>
[[nodiscard]] Container compute_symmetric_difference(InputIt1 first1, InputIt1 last1, 
                                                      InputIt2 first2, InputIt2 last2) {
    using value_type = typename std::iterator_traits<InputIt1>::value_type;
    static_assert(std::is_same_v<value_type, typename std::iterator_traits<InputIt2>::value_type>,
                  "Sequence element types must match.");

    Container result;

    // Two-pointer linear scan technique for symmetric difference
    while (first1 != last1 && first2 != last2) {
        if (*first1 < *first2) {
            result.push_back(*first1);
            ++first1;
        } else if (*first2 < *first1) {
            result.push_back(*first2);
            ++first2;
        } else {
            // *first1 == *first2: Present in both sets, so exclude from symmetric difference
            ++first1;
            ++first2;
        }
    }

    // Append remaining elements from sequence A, if any
    while (first1 != last1) {
        result.push_back(*first1);
        ++first1;
    }

    // Append remaining elements from sequence B, if any
    while (first2 != last2) {
        result.push_back(*first2);
        ++first2;
    }

    return result;
}

} // namespace dsa

#endif // SYMMETRIC_DIFFERENCE_HPP