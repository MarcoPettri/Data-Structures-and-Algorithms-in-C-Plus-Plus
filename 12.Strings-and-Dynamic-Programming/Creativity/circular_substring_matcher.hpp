/**
 * @file circular_substring_matcher.hpp
 * @brief Linear-time algorithm for determining whether a pattern P is a circular substring of a text T (C-12.10).
 * 
 * A pattern P of length m is a circular substring of text T of length n if P is a standard
 * contiguous substring of T, or if P equals the concatenation of a suffix of T and a prefix of T.
 */

#ifndef CIRCULAR_SUBSTRING_MATCHER_HPP
#define CIRCULAR_SUBSTRING_MATCHER_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace dsa {

/**
 * @enum CircularMatchMode
 * @brief Specifies the circular interpretation policy.
 */
enum class CircularMatchMode {
  /// Strict textbook definition: P is a normal substring or single suffix + single prefix (m <= 2n).
  SuffixPrefixOnly,
  /// Generalized circular matching: P can wrap around T an arbitrary number of times (subsequence of T^inf).
  AllowArbitraryWrap
};

/**
 * @struct CircularOccurrence
 * @brief Detailed metadata for a circular occurrence of pattern P in text T.
 */
struct CircularOccurrence {
  /// 0-based starting index in T (0 <= start_index < n).
  std::size_t start_index{0};

  /// True if the match wraps across the boundary of T (from index n - 1 to 0).
  bool is_wrapped{false};

  /// Length of the suffix slice T[start_index .. n - 1]. Equals m if not wrapped.
  std::size_t suffix_length{0};

  /// Length of the prefix slice T[0 .. prefix_length - 1]. Equals 0 if not wrapped.
  std::size_t prefix_length{0};

  /// The index i from the problem formula: P = T[n - m + i .. n - 1] + T[0 .. i - 1].
  std::size_t split_index_i{0};

  [[nodiscard]] bool
  operator==(const CircularOccurrence &other) const noexcept {
    return start_index == other.start_index && is_wrapped == other.is_wrapped &&
           suffix_length == other.suffix_length &&
           prefix_length == other.prefix_length &&
           split_index_i == other.split_index_i;
  }
};

/**
 * @struct CircularMatchResult
 * @brief Encapsulates the complete result of circular pattern matching.
 */
struct CircularMatchResult {
  /// True if pattern P is a circular substring of text T.
  bool is_circular_substring{false};

  /// Details of the first matching occurrence, if found.
  std::optional<CircularOccurrence> first_occurrence{std::nullopt};

  /// List of all starting occurrences of P as a circular substring of T.
  std::vector<CircularOccurrence> occurrences{};

  [[nodiscard]] constexpr explicit operator bool() const noexcept {
    return is_circular_substring;
  }
};

/**
 * @class CircularSubstringMatcher
 * @brief Streaming circular pattern matcher using the KMP failure automaton.
 * 
 * Precomputes the KMP failure table for pattern P in O(m) time and streams the circular text
 * in O(n) time using modular index projection, without duplicating or allocating text strings.
 * 
 * @tparam CharT Character type (defaults to char).
 * @tparam Traits Character traits type (defaults to std::char_traits<CharT>).
 */
template <typename CharT = char, typename Traits = std::char_traits<CharT>>
class CircularSubstringMatcher {
public:
  using StringView = std::basic_string_view<CharT, Traits>;

private:
  StringView pattern_{};
  std::vector<std::size_t> pi_{};

  /**
     * @brief Computes the KMP failure function (prefix function) for the pattern.
     * 
     * @section complexity
     * - **Time Complexity:** $\mathcal{O}(m)$, where $m = |P|$.
     * - **Auxiliary Space:** $\mathcal{O}(m)$.
     */
  void build_failure_function() {
    const std::size_t m = pattern_.size();
    if (m == 0) {
      pi_.clear();
      return;
    }

    pi_.assign(m, 0);
    std::size_t j = 0;

    for (std::size_t i = 1; i < m; ++i) {
      while (j > 0 && !Traits::eq(pattern_[i], pattern_[j])) {
        j = pi_[j - 1];
      }
      if (Traits::eq(pattern_[i], pattern_[j])) {
        ++j;
      }
      pi_[i] = j;
    }
  }

public:
  /**
     * @brief Constructs a circular matcher bound to a pattern.
     * 
     * @param pattern Pattern view.
     * 
     * @section complexity
     * - **Time Complexity:** $\mathcal{O}(m)$.
     * - **Auxiliary Space:** $\mathcal{O}(m)$.
     */
  explicit CircularSubstringMatcher(StringView pattern) : pattern_(pattern) {
    build_failure_function();
  }

  /**
     * @brief Constructs a circular matcher from a C-style string.
     * 
     * @param pattern Null-terminated character pointer.
     */
  explicit CircularSubstringMatcher(const CharT *pattern)
      : pattern_(pattern ? StringView(pattern) : StringView{}) {
    build_failure_function();
  }

  /**
     * @brief Returns the underlying pattern view.
     * @return [[nodiscard]] StringView Pattern view.
     */
  [[nodiscard]] constexpr StringView pattern() const noexcept {
    return pattern_;
  }

  /**
     * @brief Determines whether the pattern is a circular substring of the given text T.
     * 
     * Implements a zero-allocation streaming KMP automaton over the circular sequence
     * $T[k \bmod n]$ for $k = 0, \dots, n + m - 2$.
     * 
     * @param text Text string view.
     * @param mode Matching policy (defaults to SuffixPrefixOnly).
     * @return [[nodiscard]] CircularMatchResult Complete match status and occurrence data.
     * 
     * @section complexity
     * - **Time Complexity:** $\mathcal{O}(n + m)$ worst-case.
     * - **Auxiliary Space:** $\mathcal{O}(1)$ beyond the precomputed failure table and returned occurrences.
     */
  [[nodiscard]] CircularMatchResult search_in(
      StringView text,
      CircularMatchMode mode = CircularMatchMode::SuffixPrefixOnly) const {
    CircularMatchResult result;
    const std::size_t n = text.size();
    const std::size_t m = pattern_.size();

    // Edge case: Empty pattern is trivially a circular substring of any text
    if (m == 0) {
      result.is_circular_substring = true;
      if (n > 0) {
        CircularOccurrence occ;
        occ.start_index = 0;
        occ.is_wrapped = false;
        occ.suffix_length = 0;
        occ.prefix_length = 0;
        occ.split_index_i = 0;
        result.first_occurrence = occ;
        result.occurrences.push_back(occ);
      }
      return result;
    }

    // Edge case: Non-empty pattern in empty text is impossible
    if (n == 0) {
      return result;
    }

    // Handle arbitrary wrap when m > n
    if (mode == CircularMatchMode::AllowArbitraryWrap && m > n) {
      // Pattern must have period n
      for (std::size_t k = n; k < m; ++k) {
        if (!Traits::eq(pattern_[k], pattern_[k - n])) {
          return result; // Periodicity violation
        }
      }

      // Check if pattern[0..n-1] is a circular shift of text
      CircularSubstringMatcher<CharT, Traits> prefix_matcher(
          pattern_.substr(0, n));
      auto sub_res =
          prefix_matcher.search_in(text, CircularMatchMode::SuffixPrefixOnly);
      if (sub_res.is_circular_substring) {
        result.is_circular_substring = true;
        result.first_occurrence = sub_res.first_occurrence;
        result.occurrences = std::move(sub_res.occurrences);
      }
      return result;
    }

    // In SuffixPrefixOnly mode, a single suffix + single prefix cannot exceed 2n
    if (mode == CircularMatchMode::SuffixPrefixOnly && m > 2 * n) {
      return result;
    }

    // Under SuffixPrefixOnly mode:
    // A match can start at any index s in [0, n - 1].
    // To detect all occurrences starting in [0, n - 1], the circular stream needs to cover
    // stream indices k from 0 to n + m - 2.
    const std::size_t stream_limit = n + m - 1;
    std::size_t j = 0; // State in pattern

    for (std::size_t k = 0; k < stream_limit; ++k) {
      const CharT current_char = text[k % n];

      while (j > 0 && !Traits::eq(current_char, pattern_[j])) {
        j = pi_[j - 1];
      }
      if (Traits::eq(current_char, pattern_[j])) {
        ++j;
      }

      if (j == m) {
        // Full pattern matched
        const std::size_t start_stream = k + 1 - m;

        if (start_stream < n) {
          const std::size_t start_idx = start_stream;
          CircularOccurrence occ;
          occ.start_index = start_idx;

          if (start_idx + m <= n) {
            // Linear (contiguous) substring
            occ.is_wrapped = false;
            occ.suffix_length = m;
            occ.prefix_length = 0;
            occ.split_index_i = 0;
          } else {
            // Wrapped: suffix of T + prefix of T
            occ.is_wrapped = true;
            occ.suffix_length = n - start_idx;
            occ.prefix_length = m - occ.suffix_length;
            occ.split_index_i = occ.prefix_length;
          }

          result.is_circular_substring = true;
          if (!result.first_occurrence.has_value()) {
            result.first_occurrence = occ;
          }
          result.occurrences.push_back(occ);
        }

        // Advance state for finding overlapping occurrences
        j = pi_[m - 1];
      }
    }

    return result;
  }

  /**
     * @brief Fast boolean check for whether P is a circular substring of T.
     * 
     * Early-exits as soon as the first circular match is confirmed.
     * 
     * @param text Text string view.
     * @param mode Matching policy.
     * @return [[nodiscard]] bool True if P is a circular substring of T.
     */
  [[nodiscard]] bool is_circular_substring(
      StringView text,
      CircularMatchMode mode = CircularMatchMode::SuffixPrefixOnly) const {
    const std::size_t n = text.size();
    const std::size_t m = pattern_.size();

    if (m == 0)
      return true;
    if (n == 0)
      return false;

    if (mode == CircularMatchMode::AllowArbitraryWrap && m > n) {
      for (std::size_t k = n; k < m; ++k) {
        if (!Traits::eq(pattern_[k], pattern_[k - n])) {
          return false;
        }
      }
      CircularSubstringMatcher<CharT, Traits> prefix_matcher(
          pattern_.substr(0, n));
      return prefix_matcher.is_circular_substring(
          text, CircularMatchMode::SuffixPrefixOnly);
    }

    if (mode == CircularMatchMode::SuffixPrefixOnly && m > 2 * n) {
      return false;
    }

    const std::size_t stream_limit = n + m - 1;
    std::size_t j = 0;

    for (std::size_t k = 0; k < stream_limit; ++k) {
      const CharT current_char = text[k % n];

      while (j > 0 && !Traits::eq(current_char, pattern_[j])) {
        j = pi_[j - 1];
      }
      if (Traits::eq(current_char, pattern_[j])) {
        ++j;
      }

      if (j == m) {
        if (k + 1 - m < n) {
          return true;
        }
        j = pi_[m - 1];
      }
    }

    return false;
  }
};

// Deduction guides
template <typename CharT, typename Traits>
CircularSubstringMatcher(std::basic_string_view<CharT, Traits>)
    -> CircularSubstringMatcher<CharT, Traits>;

template <typename CharT, typename Traits, typename Allocator>
CircularSubstringMatcher(const std::basic_string<CharT, Traits, Allocator> &)
    -> CircularSubstringMatcher<CharT, Traits>;

template <typename CharT>
CircularSubstringMatcher(const CharT *) -> CircularSubstringMatcher<CharT>;

/**
 * @brief Checks whether pattern P is a circular substring of text T in O(n + m) time.
 * 
 * Non-template overload for std::string_view for convenient implicit conversions from
 * const char* and std::string.
 * 
 * @param text The text string T of length n.
 * @param pattern The pattern string P of length m.
 * @param mode Matching policy.
 * @return [[nodiscard]] CircularMatchResult Complete match status and occurrence data.
 * 
 * @section complexity
 * - **Time Complexity:** $\mathcal{O}(n + m)$.
 * - **Auxiliary Space:** $\mathcal{O}(m)$.
 */
[[nodiscard]] inline CircularMatchResult is_circular_substring(
    std::string_view text, std::string_view pattern,
    CircularMatchMode mode = CircularMatchMode::SuffixPrefixOnly) {
  CircularSubstringMatcher<char> matcher(pattern);
  return matcher.search_in(text, mode);
}

/**
 * @brief Templated overload for custom character types.
 */
template <typename CharT, typename Traits = std::char_traits<CharT>>
[[nodiscard]] inline CircularMatchResult is_circular_substring(
    std::basic_string_view<CharT, Traits> text,
    std::basic_string_view<CharT, Traits> pattern,
    CircularMatchMode mode = CircularMatchMode::SuffixPrefixOnly) {
  CircularSubstringMatcher<CharT, Traits> matcher(pattern);
  return matcher.search_in(text, mode);
}

} // namespace dsa

#endif // CIRCULAR_SUBSTRING_MATCHER_HPP
