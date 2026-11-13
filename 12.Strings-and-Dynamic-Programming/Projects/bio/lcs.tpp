/**
 * @file lcs.tpp
 * @brief Template implementation of the LCS algorithms declared in lcs.hpp.
 *
 * This file is not meant to be included directly; include lcs.hpp instead.
 */

#ifndef BIO_LCS_TPP
#define BIO_LCS_TPP

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <vector>

namespace bio {
namespace detail {

/**
 * @brief Computes the last row of the LCS dynamic-programming table in O(n) memory.
 *
 * Let A = a[aBeg, aEnd) and B = b[bBeg, bEnd). With @p Reverse == false the
 * returned vector `row` satisfies `row[j] = LCS(A, B[0..j))` (prefixes of B).
 * With @p Reverse == true both ranges are traversed backwards, so
 * `row[j] = LCS(A, last j elements of B)`.
 *
 * @tparam Reverse  Traverse the ranges from the back.
 * @return Vector of size `(bEnd - bBeg) + 1`.
 */
template <bool Reverse, typename SeqA, typename SeqB, typename Eq>
[[nodiscard]] std::vector<std::size_t> lcsRow(const SeqA& a, std::size_t aBeg, std::size_t aEnd,
                                              const SeqB& b, std::size_t bBeg, std::size_t bEnd,
                                              const Eq& eq)
{
    const std::size_t m = aEnd - aBeg;
    const std::size_t n = bEnd - bBeg;

    std::vector<std::size_t> prev(n + 1, 0);
    std::vector<std::size_t> cur(n + 1, 0);

    for (std::size_t i = 0; i < m; ++i) {
        const auto& ai = Reverse ? a[aEnd - 1 - i] : a[aBeg + i];
        cur[0] = 0;
        for (std::size_t j = 1; j <= n; ++j) {
            const auto& bj = Reverse ? b[bEnd - j] : b[bBeg + j - 1];
            cur[j] = eq(ai, bj) ? prev[j - 1] + 1 : std::max(prev[j], cur[j - 1]);
        }
        prev.swap(cur);
    }
    return prev;
}

/**
 * @brief Hirschberg recursion: appends the LCS of a[aBeg,aEnd) and b[bBeg,bEnd) to @p out.
 *
 * The range of @p a is halved at every level, so the recursion depth is
 * O(log m); row buffers are sized by the current range of @p b.
 */
template <typename SeqA, typename SeqB, typename Eq>
void hirschberg(const SeqA& a, std::size_t aBeg, std::size_t aEnd, const SeqB& b, std::size_t bBeg,
                std::size_t bEnd, const Eq& eq, std::vector<Match>& out)
{
    if (aBeg == aEnd || bBeg == bEnd) {
        return;
    }

    if (aEnd - aBeg == 1) {  // Base case: a single element of `a`.
        for (std::size_t j = bBeg; j < bEnd; ++j) {
            if (eq(a[aBeg], b[j])) {
                out.emplace_back(aBeg, j);
                return;
            }
        }
        return;
    }

    const std::size_t mid = aBeg + (aEnd - aBeg) / 2;
    const std::size_t n = bEnd - bBeg;

    const auto left = lcsRow<false>(a, aBeg, mid, b, bBeg, bEnd, eq);
    const auto right = lcsRow<true>(a, mid, aEnd, b, bBeg, bEnd, eq);

    // Choose the split point of b that maximises the combined LCS length.
    std::size_t bestK = 0;
    std::size_t bestVal = left[0] + right[n];
    for (std::size_t k = 1; k <= n; ++k) {
        const std::size_t v = left[k] + right[n - k];
        if (v > bestVal) {
            bestVal = v;
            bestK = k;
        }
    }

    hirschberg(a, aBeg, mid, b, bBeg, bBeg + bestK, eq, out);
    hirschberg(a, mid, aEnd, b, bBeg + bestK, bEnd, eq, out);
}

}  // namespace detail

template <typename SeqA, typename SeqB, typename Eq>
std::size_t lcsLength(const SeqA& a, const SeqB& b, Eq eq)
{
    const std::size_t m = std::size(a);
    const std::size_t n = std::size(b);
    if (m == 0 || n == 0) {
        return 0;
    }

    // Keep the row buffer as short as possible: it is sized by the second sequence.
    if (n > m) {
        const auto flipped = [&eq](const auto& x, const auto& y) { return eq(y, x); };
        return detail::lcsRow<false>(b, 0, n, a, 0, m, flipped).back();
    }
    return detail::lcsRow<false>(a, 0, m, b, 0, n, eq).back();
}

template <typename SeqA, typename SeqB, typename Eq>
std::vector<Match> lcsMatches(const SeqA& a, const SeqB& b, Eq eq)
{
    const std::size_t m = std::size(a);
    const std::size_t n = std::size(b);
    const std::size_t limit = std::min(m, n);

    // Strip the common prefix and suffix: they are always part of some LCS.
    std::size_t prefix = 0;
    while (prefix < limit && eq(a[prefix], b[prefix])) {
        ++prefix;
    }
    std::size_t suffix = 0;
    while (suffix < limit - prefix && eq(a[m - 1 - suffix], b[n - 1 - suffix])) {
        ++suffix;
    }

    std::vector<Match> out;
    out.reserve(limit);
    for (std::size_t i = 0; i < prefix; ++i) {
        out.emplace_back(i, i);
    }

    const std::size_t aEnd = m - suffix;
    const std::size_t bEnd = n - suffix;

    // Recurse on the longer sequence so row buffers are sized by the shorter one.
    if (aEnd - prefix >= bEnd - prefix) {
        detail::hirschberg(a, prefix, aEnd, b, prefix, bEnd, eq, out);
    } else {
        const auto flipped = [&eq](const auto& x, const auto& y) { return eq(y, x); };
        std::vector<Match> swapped;
        detail::hirschberg(b, prefix, bEnd, a, prefix, aEnd, flipped, swapped);
        for (const auto& [j, i] : swapped) {
            out.emplace_back(i, j);
        }
    }

    for (std::size_t k = 0; k < suffix; ++k) {
        out.emplace_back(aEnd + k, bEnd + k);
    }
    return out;
}

template <typename SeqA, typename SeqB, typename Eq>
std::vector<Match> lcsMatchesTable(const SeqA& a, const SeqB& b, Eq eq, std::size_t maxCells)
{
    const std::size_t m = std::size(a);
    const std::size_t n = std::size(b);
    const std::size_t width = n + 1;

    if (m + 1 > maxCells / width) {
        throw std::length_error("lcsMatchesTable: DP table would exceed maxCells");
    }

    std::vector<std::uint32_t> table((m + 1) * width, 0);

    for (std::size_t i = 1; i <= m; ++i) {
        const std::size_t row = i * width;
        const std::size_t above = (i - 1) * width;
        for (std::size_t j = 1; j <= n; ++j) {
            table[row + j] = eq(a[i - 1], b[j - 1])
                                 ? table[above + j - 1] + 1
                                 : std::max(table[above + j], table[row + j - 1]);
        }
    }

    // Trace back from the bottom-right corner.
    std::vector<Match> out;
    out.reserve(table[m * width + n]);
    std::size_t i = m;
    std::size_t j = n;
    while (i > 0 && j > 0) {
        if (eq(a[i - 1], b[j - 1])) {
            out.emplace_back(i - 1, j - 1);
            --i;
            --j;
        } else if (table[(i - 1) * width + j] >= table[i * width + j - 1]) {
            --i;
        } else {
            --j;
        }
    }
    std::reverse(out.begin(), out.end());
    return out;
}

}  // namespace bio

#endif  // BIO_LCS_TPP