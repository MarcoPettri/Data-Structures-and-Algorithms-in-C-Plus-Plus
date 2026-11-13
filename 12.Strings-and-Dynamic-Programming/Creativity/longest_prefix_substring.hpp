/**
 * @file longest_prefix_substring.hpp
 * @brief Knuth-Morris-Pratt based algorithm for finding the longest prefix of a pattern P
 *        that is a substring of a text T in O(n + m) time (C-12.9).
 */

#ifndef LONGEST_PREFIX_SUBSTRING_HPP
#define LONGEST_PREFIX_SUBSTRING_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace dsa {

/**
 * @struct LongestPrefixMatch
 * @brief Encapsulates the result of querying the longest prefix of pattern P occurring in text T.
 * 
 * @tparam CharT Character type (defaults to char).
 * @tparam Traits Character traits type (defaults to std::char_traits<CharT>).
 */
template <typename CharT = char, typename Traits = std::char_traits<CharT>>
struct LongestPrefixMatch {
  /// Length of the longest prefix of P that appears as a substring of T.
  std::size_t length{0};

  /// View of the matching prefix of P. Empty if length == 0.
  std::basic_string_view<CharT, Traits> prefix{};

  /// 0-based starting index in T of the first occurrence of this prefix.
  /// std::nullopt if length == 0.
  std::optional<std::size_t> first_occurrence{std::nullopt};

  /// List of all 0-based starting indices in T where this longest prefix occurs.
  std::vector<std::size_t> occurrences{};

  /**
     * @brief Checks whether a non-empty prefix of P was found in T.
     * @return [[nodiscard]] bool True if length > 0, false otherwise.
     */
  [[nodiscard]] constexpr bool has_match() const noexcept { return length > 0; }
};

/**
 * @class LongestPrefixMatcher
 * @brief Precomputes the KMP failure function for a pattern P and efficiently finds
 *        its longest prefix appearing in any given text T in linear time.
 * 
 * @tparam CharT Character type.
 * @tparam Traits Character traits type.
 */
template <typename CharT = char, typename Traits = std::char_traits<CharT>>
class LongestPrefixMatcher {
public:
  using StringView = std::basic_string_view<CharT, Traits>;

private:
  StringView pattern_{};
  std::vector<std::size_t> pi_{};

  /**
     * @brief Computes the KMP failure function (prefix function) for the pattern.
     * 
     * pi[i] is defined as the length of the longest proper prefix of pattern[0...i]
     * that is also a suffix of pattern[0...i].
     * 
     * @section complexity
     * - **Time Complexity:** $\mathcal{O}(m)$, where $m$ is the pattern length.
     * - **Auxiliary Space:** $\mathcal{O}(m)$ to store the failure table.
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
     * @brief Constructs a matcher by binding to a string view and computing its failure function.
     * 
     * @param pattern Pattern string view to find prefixes of.
     * 
     * @section complexity
     * - **Time Complexity:** $\mathcal{O}(m)$, where $m = |P|$.
     * - **Auxiliary Space:** $\mathcal{O}(m)$.
     */
  explicit LongestPrefixMatcher(StringView pattern) : pattern_(pattern) {
    build_failure_function();
  }

  /**
     * @brief Constructs a matcher from a null-terminated C-style character string.
     * 
     * @param pattern Null-terminated character array.
     */
  explicit LongestPrefixMatcher(const CharT *pattern)
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
     * @brief Returns the precomputed KMP failure table.
     * @return [[nodiscard]] const std::vector<std::size_t>& Failure function values.
     */
  [[nodiscard]] const std::vector<std::size_t> &failure_table() const noexcept {
    return pi_;
  }

  /**
     * @brief Finds the longest prefix of the pattern that occurs as a substring in the given text.
     * 
     * Streams the text through the KMP finite automaton. At each step $i$, the state $j$ represents
     * the length of the longest prefix of $P$ that matches a suffix of $T[0 \dots i]$.
     * Tracking the maximum $j$ encountered across all characters of $T$ yields the exact longest
     * prefix of $P$ present anywhere in $T$.
     * 
     * @param text Target text to search.
     * @return [[nodiscard]] LongestPrefixMatch<CharT, Traits> Search result.
     * 
     * @section complexity
     * - **Time Complexity:** $\mathcal{O}(n)$, where $n = |T|$. Amortized $\mathcal{O}(1)$ operations per character.
     * - **Auxiliary Space:** $\mathcal{O}(k)$ where $k$ is the number of occurrences of the longest prefix.
     */
  [[nodiscard]] LongestPrefixMatch<CharT, Traits>
  find_in(StringView text) const {
    LongestPrefixMatch<CharT, Traits> result;
    const std::size_t n = text.size();
    const std::size_t m = pattern_.size();

    if (n == 0 || m == 0) {
      result.prefix = pattern_.substr(0, 0);
      return result;
    }

    std::size_t max_len = 0;
    std::size_t j = 0; // State: matched length of pattern prefix

    for (std::size_t i = 0; i < n; ++i) {
      while (j > 0 && !Traits::eq(text[i], pattern_[j])) {
        j = pi_[j - 1];
      }
      if (Traits::eq(text[i], pattern_[j])) {
        ++j;
      }

      if (j > max_len) {
        max_len = j;
        result.occurrences.clear();
        result.occurrences.push_back(i - j + 1);
      } else if (j == max_len && max_len > 0) {
        result.occurrences.push_back(i - j + 1);
      }

      if (j == m) {
        // Entire pattern matched; backtrack via failure function for subsequent matches
        j = pi_[j - 1];
      }
    }

    result.length = max_len;
    result.prefix = pattern_.substr(0, max_len);
    if (max_len > 0 && !result.occurrences.empty()) {
      result.first_occurrence = result.occurrences.front();
    }

    return result;
  }
};

// Deduction guides for LongestPrefixMatcher
template <typename CharT, typename Traits>
LongestPrefixMatcher(std::basic_string_view<CharT, Traits>)
    -> LongestPrefixMatcher<CharT, Traits>;

template <typename CharT, typename Traits, typename Allocator>
LongestPrefixMatcher(const std::basic_string<CharT, Traits, Allocator> &)
    -> LongestPrefixMatcher<CharT, Traits>;

template <typename CharT>
LongestPrefixMatcher(const CharT *) -> LongestPrefixMatcher<CharT>;

/**
 * @brief Finds the longest prefix of pattern P that occurs as a substring of text T.
 * 
 * Non-template overload for standard `std::string_view` enabling seamless implicit conversion
 * from `std::string` and `const char*`.
 * 
 * @param text The text $T$ of length $n$.
 * @param pattern The pattern $P$ of length $m$.
 * @return [[nodiscard]] LongestPrefixMatch<char> Result containing length, prefix view, and occurrences.
 * 
 * @section complexity
 * - **Time Complexity:** $\mathcal{O}(n + m)$, where $n = |T|$ and $m = |P|$. Precomputes the failure function
 *   in $\mathcal{O}(m)$ time and scans the text in $\mathcal{O}(n)$ time.
 * - **Auxiliary Space:** $\mathcal{O}(m)$ to store the KMP failure table.
 */
[[nodiscard]] inline LongestPrefixMatch<char>
find_longest_prefix_substring(std::string_view text, std::string_view pattern) {
  LongestPrefixMatcher<char> matcher(pattern);
  return matcher.find_in(text);
}

/**
 * @brief Generic templated overload for custom character types (e.g. wchar_t, char16_t, char32_t).
 */
template <typename CharT, typename Traits = std::char_traits<CharT>>
[[nodiscard]] inline LongestPrefixMatch<CharT, Traits>
find_longest_prefix_substring(std::basic_string_view<CharT, Traits> text,
                              std::basic_string_view<CharT, Traits> pattern) {
  LongestPrefixMatcher<CharT, Traits> matcher(pattern);
  return matcher.find_in(text);
}

} // namespace dsa

#endif // LONGEST_PREFIX_SUBSTRING_HPP
