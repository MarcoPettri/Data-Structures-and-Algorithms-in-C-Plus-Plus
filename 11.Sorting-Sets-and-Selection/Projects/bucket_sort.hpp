/**
 * @file bucket_sort.hpp
 * @brief Bucket sort for linked lists (std::list) with integer keys in [0, N-1].
 *
 * @details
 * Public interface of the bucket-sort algorithm. The generic implementation
 * lives in @ref bucket_sort.tpp (included at the bottom of this header); the
 * convenience overload for `std::list<int>` is compiled in @ref bucket_sort.cpp.
 *
 * Key properties:
 *  - **Time**:   O(n + N) (assuming the key extractor is O(1)).
 *  - **Space**:  O(N) auxiliary (N bucket headers); *no element is copied,
 *                moved or allocated* — nodes are relinked with
 *                `std::list::splice`, so references, pointers and iterators
 *                to elements stay valid.
 *  - **Stable**: elements with equal keys keep their relative order.
 *
 */

#ifndef ALGO_BUCKET_SORT_HPP
#define ALGO_BUCKET_SORT_HPP

#include <cstddef>
#include <list>

namespace algo {

/**
 * @brief Stable bucket sort of a linked list, using a user-supplied key extractor.
 *
 * Every element is spliced (O(1), no allocation, no copy) into the bucket
 * `key(element)`, then all buckets are spliced back in increasing key order.
 *
 * @tparam T        Element type of the list.
 * @tparam Alloc    Allocator type of the list.
 * @tparam KeyFn    Callable `K(const T&)` where `K` is an integral type
 *                  (not `bool`). Must be cheap (O(1)) to preserve the
 *                  O(n + N) bound.
 *
 * @param[in,out] list  List to be sorted in place (ascending by key).
 * @param[in]     N     Size of the key universe; every key must be in [0, N-1].
 * @param[in]     key   Key extractor.
 *
 * @throws std::invalid_argument  if `N < 2`.
 * @throws std::out_of_range      if some key is negative or `>= N`.
 * @throws std::bad_alloc         if allocating the N bucket headers fails.
 * @throws Anything thrown by `key`.
 *
 * @par Exception safety
 * Basic guarantee. If `N < 2` or the bucket allocation fails the list is
 * untouched. If a key is invalid or `key` throws mid-sort, no element is lost
 * or duplicated and the list stays valid, but its order is unspecified.
 *
 * @note Lists with fewer than two elements are returned immediately, without
 *       invoking `key`.
 *
 * @par Complexity
 * Time O(n + N), auxiliary space O(N).
 *
 * @par Example
 * @code
 * struct Rec { int bucket; std::string name; };
 * std::list<Rec> l = ...;
 * algo::bucket_sort(l, 100, [](const Rec& r) { return r.bucket; });
 * @endcode
 */
template <typename T, typename Alloc, typename KeyFn>
void bucket_sort(std::list<T, Alloc> &list, std::size_t N, KeyFn key);

/**
 * @brief Stable bucket sort of a `std::list<int>` whose values are the keys.
 *
 * @param[in,out] list  List to sort in ascending order.
 * @param[in]     N     Key universe size; every value must be in [0, N-1].
 *
 * @throws std::invalid_argument  if `N < 2`.
 * @throws std::out_of_range      if some value is negative or `>= N`.
 *
 * @see bucket_sort(std::list<T, Alloc>&, std::size_t, KeyFn)
 */
void bucket_sort(std::list<int> &list, std::size_t N);

}  // namespace algo

#include "bucket_sort.tpp"

#endif  // ALGO_BUCKET_SORT_HPP