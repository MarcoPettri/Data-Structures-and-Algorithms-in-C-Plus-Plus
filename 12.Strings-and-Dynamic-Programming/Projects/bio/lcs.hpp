/**
 * @file lcs.hpp
 * @brief Longest Common Subsequence (LCS) algorithms for arbitrary sequences.
 *
 * Three variants are offered, each with a different time/memory trade-off:
 *
 * | Function            | Time     | Extra memory      | Output             |
 * |---------------------|----------|-------------------|--------------------|
 * | bio::lcsLength      | O(m*n)   | O(min(m,n))       | length only        |
 * | bio::lcsMatches     | O(m*n)   | O(min(m,n))       | matched index pairs|
 * | bio::lcsMatchesTable| O(m*n)   | O(m*n)            | matched index pairs|
 *
 * bio::lcsMatches uses Hirschberg's divide-and-conquer refinement, so it can
 * process whole genomes (tens of thousands of bases) with a few hundred KiB of
 * memory, instead of the gigabytes a full dynamic-programming table needs.
 * Common prefixes and suffixes are stripped first, which makes closely related
 * sequences (the typical case for DNA) considerably faster.
 *
 * All functions are templates over the sequence types; any type offering
 * `std::size(seq)` and `operator[]` works (std::string, std::string_view,
 * std::vector<T>, std::array<T,N>, ...). The two sequences may have different
 * types.
 *
 * @note The template definitions live in lcs.tpp, which is included at the
 *       bottom of this header.
 *
 */

#ifndef BIO_LCS_HPP
#define BIO_LCS_HPP

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

namespace bio {

/**
 * @brief A pair of zero-based indices `(i, j)` meaning `a[i]` is matched with `b[j]`.
 */
using Match = std::pair<std::size_t, std::size_t>;

/**
 * @brief Default upper bound on the number of cells allocated by lcsMatchesTable().
 *
 * 2^26 cells * 4 bytes = 256 MiB.
 */
inline constexpr std::size_t kDefaultMaxTableCells = std::size_t{1} << 26;

/**
 * @brief Computes the length of the LCS of two sequences.
 *
 * Uses the classic dynamic program with two rolling rows.
 *
 * @tparam SeqA  First sequence type (needs `std::size` and `operator[]`).
 * @tparam SeqB  Second sequence type.
 * @tparam Eq    Binary predicate `bool(const A::value_type&, const B::value_type&)`;
 *               must be const-callable. Defaults to `std::equal_to<>`.
 * @param a      First sequence.
 * @param b      Second sequence.
 * @param eq     Equality predicate.
 * @return Length of a longest common subsequence.
 *
 * @complexity Time O(m*n); memory O(min(m,n)).
 */
template <typename SeqA, typename SeqB, typename Eq = std::equal_to<>>
[[nodiscard]] std::size_t lcsLength(const SeqA& a, const SeqB& b, Eq eq = Eq{});

/**
 * @brief Computes one LCS and returns it as strictly increasing index pairs.
 *
 * Implements Hirschberg's linear-space algorithm with prefix/suffix trimming.
 * The returned vector satisfies, for consecutive elements,
 * `first.i < second.i` and `first.j < second.j`, and `eq(a[i], b[j])` holds for
 * every pair. Its size equals lcsLength().
 *
 * @tparam SeqA,SeqB,Eq  See lcsLength().
 * @param a   First sequence.
 * @param b   Second sequence.
 * @param eq  Equality predicate.
 * @return Matched index pairs in increasing order.
 *
 * @complexity Time O(m*n); memory O(min(m,n)) plus the O(LCS) output.
 */
template <typename SeqA, typename SeqB, typename Eq = std::equal_to<>>
[[nodiscard]] std::vector<Match> lcsMatches(const SeqA& a, const SeqB& b, Eq eq = Eq{});

/**
 * @brief Computes one LCS with the textbook full-table dynamic program.
 *
 * Fast for small inputs (simple, cache-friendly loops) and handy as a
 * reference implementation to validate lcsMatches(). Memory grows with
 * `(m+1)*(n+1)`.
 *
 * @tparam SeqA,SeqB,Eq  See lcsLength().
 * @param a         First sequence.
 * @param b         Second sequence.
 * @param eq        Equality predicate.
 * @param maxCells  Maximum number of table cells to allocate.
 * @return Matched index pairs in increasing order.
 *
 * @throws std::length_error if `(m+1)*(n+1)` exceeds @p maxCells.
 *
 * @complexity Time O(m*n); memory O(m*n).
 */
template <typename SeqA, typename SeqB, typename Eq = std::equal_to<>>
[[nodiscard]] std::vector<Match> lcsMatchesTable(const SeqA& a, const SeqB& b, Eq eq = Eq{},
                                                 std::size_t maxCells = kDefaultMaxTableCells);

}  // namespace bio

#include "lcs.tpp"

#endif  // BIO_LCS_HPP