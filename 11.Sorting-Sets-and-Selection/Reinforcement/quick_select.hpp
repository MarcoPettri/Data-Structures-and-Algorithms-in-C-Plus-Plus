/**
 * @file quick_select.hpp
 * @brief High-performance, modern C++17 in-place QuickSelect algorithm 
 *        with randomized pivoting for expected linear time complexity.
 */

#ifndef QUICK_SELECT_HPP
#define QUICK_SELECT_HPP

#include <iterator>
#include <algorithm>
#include <random>
#include <functional>
#include <type_traits>

namespace dsa {

namespace detail {

/**
 * @brief Partitions a range around a chosen pivot element using Lomuto-style partitioning.
 * 
 * @tparam RandomIt Random access iterator type.
 * @tparam Compare Binary predicate type for element comparison.
 * @param first Iterator to the start of the range.
 * @param last Iterator to the end of the range.
 * @param pivot_idx Iterator pointing to the chosen pivot.
 * @param comp Comparator object.
 * @return RandomIt Iterator pointing to the pivot's final sorted position.
 */
template <typename RandomIt, typename Compare>
[[nodiscard]] RandomIt partition(RandomIt first, RandomIt last, RandomIt pivot_idx, Compare comp) {
    auto pivot_val = *pivot_idx;
    auto last_minus_1 = std::prev(last);
    
    // Move pivot to the end of the range
    std::iter_swap(pivot_idx, last_minus_1);

    RandomIt store_idx = first;
    for (RandomIt it = first; it != last_minus_1; ++it) {
        if (comp(*it, pivot_val)) {
            std::iter_swap(store_idx, it);
            ++store_idx;
        }
    }
    // Restore pivot to its correct sorted position
    std::iter_swap(store_idx, last_minus_1);
    return store_idx;
}

} // namespace detail

/**
 * @brief Rearranges the elements in the range [first, last) such that the element at 
 *        the nth position is the element that would appear in that exact position 
 *        if the range were fully sorted.
 * 
 * @tparam RandomIt Random access iterator type.
 * @tparam Compare Binary predicate type for element comparison (defaults to std::less<>).
 * @param first Iterator to the beginning of the range.
 * @param nth Iterator pointing to the target position to select.
 * @param last Iterator to the end of the range.
 * @param comp Comparator object.
 * @return [[nodiscard]] RandomIt Iterator pointing to the selected nth element.
 * 
 * @section complexity
 * - **Time Complexity:** Expected $\mathcal{O}(n)$, where $n = \text{distance}(first, last)$, 
 *   due to randomized pivoting. Worst-case is $\mathcal{O}(n^2)$ for malicious inputs, 
 *   which has a vanishing probability of occurrence.
 * - **Space Complexity:** $\mathcal{O}(1)$ auxiliary space (iterative implementation avoids 
 *   recursion stack overhead).
 */
template <typename RandomIt, typename Compare = std::less<>>
[[nodiscard]] RandomIt quick_select(RandomIt first, RandomIt nth, RandomIt last, Compare comp = Compare{}) {
    static_assert(
        std::is_base_of_v<
            std::random_access_iterator_tag,
            typename std::iterator_traits<RandomIt>::iterator_category
        >,
        "QuickSelect requires Random Access Iterators."
    );

    if (first == last || nth == last) {
        return last;
    }

    // Thread-safe local random number generator for randomized pivoting
    std::random_device rd;
    std::mt19937 gen(rd());

    auto l = first;
    auto h = std::prev(last);

    while (l < h) {
        // Pick a uniformly random pivot index between l and h inclusive
        auto dist = std::distance(l, h);
        std::uniform_int_distribution<typename std::iterator_traits<RandomIt>::difference_type> dist_rand(0, dist);
        RandomIt pivot_idx = l + dist_rand(gen);

        // Partition range and get pivot's final sorted position
        RandomIt pivot_final = detail::partition(l, h + 1, pivot_idx, comp);

        if (pivot_final == nth) {
            return nth;
        } else if (pivot_final < nth) {
            l = std::next(pivot_final);
        } else {
            h = std::prev(pivot_final);
        }
    }

    return nth;
}

} // namespace dsa

#endif // QUICK_SELECT_HPP