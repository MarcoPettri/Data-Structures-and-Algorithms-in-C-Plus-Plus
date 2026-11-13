/**
 * @file alignment.hpp
 * @brief Turns an LCS into a human-readable pairwise alignment of two DNA strings.
 *
 * The alignment is built from the LCS "anchors": identical columns are the LCS
 * characters; between two anchors the leftover characters of both strings are
 * paired up as mismatches and any remainder is aligned against gaps ('-').
 *
 * @note This is an LCS-based alignment (matches maximised, no scoring matrix).
 *       It is not a replacement for Needleman-Wunsch / Smith-Waterman with an
 *       affine gap model.
 */

#ifndef BIO_ALIGNMENT_HPP
#define BIO_ALIGNMENT_HPP

#include <cstddef>
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

#include "lcs.hpp"

namespace bio {

/**
 * @brief Result of a pairwise alignment.
 *
 * `top`, `bottom` and `midline` always have identical lengths. In `midline`,
 * '|' marks an identical column and ' ' a mismatch or a gap.
 */
struct Alignment {
  std::string top;          ///< First sequence with gaps ('-') inserted.
  std::string bottom;       ///< Second sequence with gaps ('-') inserted.
  std::string midline;      ///< Match markers.
  std::size_t matches = 0;  ///< Identical columns (equals the LCS length).
  std::size_t mismatches = 0;  ///< Columns with two different residues.
  std::size_t gaps = 0;     ///< Columns containing a gap.

    /// @return Number of alignment columns.
  [[nodiscard]] std::size_t length() const noexcept { return top.size(); }

  /// @return Identical columns divided by alignment length, in [0, 1] (0 if empty).
  [[nodiscard]] double identity() const noexcept;
};

/**
 * @brief Builds an alignment from a precomputed LCS.
 *
 * @param a          First sequence.
 * @param b          Second sequence.
 * @param lcsPairs   Strictly increasing matched index pairs, e.g. from bio::lcsMatches().
 * @return The gapped alignment.
 *
 * @throws std::invalid_argument if @p lcsPairs is out of range, not strictly
 *         increasing, or pairs two different characters.
 *
 * @complexity O(m + n).
 */
[[nodiscard]] Alignment buildAlignment(std::string_view a, std::string_view b,
                                       const std::vector<Match> &lcsPairs);

/**
 * @brief Convenience wrapper: computes the LCS of two strings and aligns them.
 *
 * @param a  First sequence.
 * @param b  Second sequence.
 * @return The gapped alignment.
 *
 * @complexity Time O(m*n); memory O(m + n).
 */
[[nodiscard]] Alignment alignByLcs(std::string_view a, std::string_view b);

/**
 * @brief Extracts the LCS characters themselves.
 *
 * @param a         First sequence.
 * @param lcsPairs  Matched index pairs (only the first index is used).
 * @return The subsequence of @p a selected by @p lcsPairs.
 */
[[nodiscard]] std::string lcsString(std::string_view a,
                                    const std::vector<Match> &lcsPairs);

/**
 * @brief Writes a BLAST-like block view of an alignment.
 *
 * Each block shows one-based start positions of both sequences.
 *
 * @param os         Destination stream.
 * @param alignment  Alignment to print.
 * @param lineWidth  Columns per block (clamped to at least 1).
 */
void writeAlignment(std::ostream &os, const Alignment &alignment,
                    std::size_t lineWidth = 80);

}  // namespace bio

#endif  // BIO_ALIGNMENT_HPP