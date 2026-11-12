/**
 * @file iterative_quicksort.hpp
 * @brief Non-recursive in-place QuickSort algorithm 
 *        using an explicit stack and O(log n) stack space optimization.
 */

#ifndef ITERATIVE_QUICKSORT_HPP
#define ITERATIVE_QUICKSORT_HPP

#include <iterator>
#include <algorithm>
#include <stack>
#include <utility>
#include <type_traits>

namespace dsa {

namespace detail {

/**
 * @brief Partitions a range [first, last) around a pivot element chosen via median-of-three.
 * 
 * @tparam RandomIt Random access iterator type.
 * @tparam Compare Binary predicate type.
 * @param first Iterator to the start of the range.
 * @param last Iterator to the end of the range.
 * @param comp Comparator object.
 * @return RandomIt Iterator pointing to the pivot's final sorted position.
 */
template <typename RandomIt, typename Compare>
[[nodiscard]] RandomIt partition(RandomIt first, RandomIt last, Compare comp) {
    auto last_elem = std::prev(last);
    auto mid = first + std::distance(first, last) / 2;

    // Median-of-three pivot selection to prevent worst-case O(n^2) degradation on sorted data
    if (comp(*mid, *first)) std::iter_swap(first, mid);
    if (comp(*last_elem, *first)) std::iter_swap(first, last_elem);
    if (comp(*mid, *last_elem)) std::iter_swap(mid, last_elem);

    auto pivot_val = *last_elem;
    RandomIt i = first;

    for (RandomIt j = first; j != last_elem; ++j) {
        if (comp(*j, pivot_val)) {
            std::iter_swap(i, j);
            ++i;
        }
    }
    std::iter_swap(i, last_elem);
    return i;
}

} // namespace detail

/**
 * @brief Sorts a random-access range in-place using iterative QuickSort with an explicit stack.
 * 
 * @tparam RandomIt Random access iterator type.
 * @tparam Compare Binary predicate type (defaults to std::less<>).
 * @param first Iterator to the beginning of the range.
 * @param last Iterator to the end of the range.
 * @param comp Comparator object.
 * 
 * @section complexity
 * - **Time Complexity:** Expected $\mathcal{O}(n \log n)$, where $n = \text{distance}(first, last)$. 
 *   Worst-case is $\mathcal{O}(n^2)$ when pivot selection splits unbalanced partitions, 
 *   mitigated here via median-of-three pivoting.
 * - **Space Complexity:** $\mathcal{O}(\log n)$ auxiliary stack space. By always pushing 
 *   the larger subproblem onto the explicit stack first and processing the smaller subproblem 
 *   iteratively, the stack depth is strictly bounded by $\mathcal{O}(\log n)$.
 */
template <typename RandomIt, typename Compare = std::less<>>
void iterative_quicksort(RandomIt first, RandomIt last, Compare comp = Compare{}) {
    static_assert(
        std::is_base_of_v<
            std::random_access_iterator_tag,
            typename std::iterator_traits<RandomIt>::iterator_category
        >,
        "Iterative QuickSort requires Random Access Iterators."
    );

    if (first >= last || std::distance(first, last) <= 1) {
        return;
    }

    // Explicit stack storing subproblem iterator pairs [low, high]
    std::stack<std::pair<RandomIt, RandomIt>> stack;
    stack.push({first, last});

    while (!stack.empty()) {
        auto [low, high] = stack.top();
        stack.pop();

        auto dist = std::distance(low, high);
        if (dist > 1) {
            RandomIt pivot_pos = detail::partition(low, high, comp);

            RandomIt left_low = low;
            RandomIt left_high = pivot_pos;
            RandomIt right_low = std::next(pivot_pos);
            RandomIt right_high = high;

            auto left_size = std::distance(left_low, left_high);
            auto right_size = std::distance(right_low, right_high);

            // Push larger subproblem first to guarantee O(log n) auxiliary stack space
            if (left_size > right_size) {
                if (left_size > 1) {
                    stack.push({left_low, left_high});
                }
                if (right_size > 1) {
                    stack.push({right_low, right_high});
                }
            } else {
                if (right_size > 1) {
                    stack.push({right_low, right_high});
                }
                if (left_size > 1) {
                    stack.push({left_low, left_high});
                }
            }
        }
    }
}

} // namespace dsa

#endif // ITERATIVE_QUICKSORT_HPP