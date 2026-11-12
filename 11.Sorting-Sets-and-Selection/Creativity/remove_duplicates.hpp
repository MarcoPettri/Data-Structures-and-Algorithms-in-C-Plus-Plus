/**
 * @file remove_duplicates.hpp
 * @brief Algorithm for removing duplicate entries 
 *        from an arbitrary collection while preserving relative order.
 */

#ifndef REMOVE_DUPLICATES_HPP
#define REMOVE_DUPLICATES_HPP

#include <vector>
#include <unordered_set>
#include <iterator>

namespace dsa {

/**
 * @brief Removes duplicate elements from a sequence while preserving
 *        the relative order of their first occurrences.
 *
 * @tparam InputIt Input iterator type.
 * @tparam Container Output container type.
 * @param first Iterator to the beginning of the input range.
 * @param last Iterator to the end of the input range.
 *
 * @return A container containing only the first occurrence of each element.
 *
 * @note Elements must be hashable and equality-comparable.
 *       The output container must support push_back().
 *
 * @complexity
 * - Time: O(n) expected, O(n²) worst-case due to hash collisions.
 * - Space: O(n), including the hash set and output container.
 */
template <
    typename InputIt,
    typename Container = std::vector<
        typename std::iterator_traits<InputIt>::value_type
    >
>
[[nodiscard]] Container remove_duplicates(InputIt first, InputIt last) {
    using value_type = typename std::iterator_traits<InputIt>::value_type;

    Container result;
    std::unordered_set<value_type> seen;

    for (; first != last; ++first) {
        if (seen.insert(*first).second) {
            result.push_back(*first);
        }
    }

    return result;
}

} // namespace dsa

#endif // REMOVE_DUPLICATES_HPP