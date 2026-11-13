/**
 * @file prefix_suffix_overlap.hpp
 * @brief Algorithm for finding the longest prefix of string X that is a suffix of string Y in O(n + m) time (C-12.13).
 */

#ifndef PREFIX_SUFFIX_OVERLAP_HPP
#define PREFIX_SUFFIX_OVERLAP_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace dsa {

/**
 * @struct PrefixSuffixOverlap
 * @brief Encapsulates the result of finding the longest prefix of X that is a suffix of Y.
 * 
 * @tparam CharT Character type (defaults to char).
 * @tparam Traits Character traits type (defaults to std::char_traits<CharT>).
 */
template <typename CharT = char, typename Traits = std::char_traits<CharT>>
struct PrefixSuffixOverlap {
  /// Length of the longest prefix of X that is a suffix of Y.
  std::size_t length{0};

  /// View of the overlapping prefix of X (which is also the suffix of Y).
  std::basic_string_view<CharT, Traits> matching_slice{};

  /// 0-based starting index in Y where this suffix begins.
  /// std::nullopt if length == 0.
  std::optional<std::size_t> suffix_start_index{std::nullopt};

  /// All valid overlap lengths (prefixes of X that are suffixes of Y), in strictly descending order.
  std::vector<std::size_t> all_overlap_lengths{};

  /**
     * @brief Checks whether a non-empty prefix of X is a suffix of Y.
     * @return [[nodiscard]] bool True if length > 0, false otherwise.
     */
  [[nodiscard]] constexpr bool has_overlap() const noexcept {
    return length > 0;
  }
};

/**
 * @class PrefixSuffixMatcher
 * @brief Binds to string X of length n, precomputes its KMP failure function in O(n) time,
 *        and allows querying any string Y of length m in O(m) time.
 * 
 * @tparam CharT Character type.
 * @tparam Traits Character traits type.
 */
template <typename CharT = char, typename Traits = std::char_traits<CharT>>
class PrefixSuffixMatcher {
public:
  using StringView = std::basic_string_view<CharT, Traits>;

private:
  StringView x_{};
  std::vector<std::size_t> pi_{};

  /**
     * @brief Computes the KMP failure table for string X.
     * 
     * @section complexity
     * - **Time Complexity:** $\mathcal{O}(n)$, where $n = |X|$.
     * - **Auxiliary Space:** $\mathcal{O}(n)$.
     */
  void build_failure_function() {
    const std::size_t n = x_.size();
    if (n == 0) {
      pi_.clear();
      return;
    }

    pi_.assign(n, 0);
    std::size_t j = 0;

    for (std::size_t i = 1; i < n; ++i) {
      while (j > 0 && !Traits::eq(x_[i], x_[j])) {
        j = pi_[j - 1];
      }
      if (Traits::eq(x_[i], x_[j])) {
        ++j;
      }
      pi_[i] = j;
    }
  }

public:
  /**
     * @brief Constructs a matcher bound to string X.
     * 
     * @param x String X whose prefixes are searched.
     * 
     * @section complexity
     * - **Time Complexity:** $\mathcal{O}(n)$, where $n = |X|$.
     * - **Auxiliary Space:** $\mathcal{O}(n)$.
     */
  explicit PrefixSuffixMatcher(StringView x) : x_(x) {
    build_failure_function();
  }

  /**
     * @brief Constructs a matcher from a null-terminated C-style string.
     * 
     * @param x Null-terminated character string.
     */
  explicit PrefixSuffixMatcher(const CharT *x)
      : x_(x ? StringView(x) : StringView{}) {
    build_failure_function();
  }

  /**
     * @brief Returns the underlying view of X.
     * @return [[nodiscard]] StringView View of X.
     */
  [[nodiscard]] constexpr StringView string_x() const noexcept { return x_; }

  /**
     * @brief Returns the precomputed KMP failure table of X.
     * @return [[nodiscard]] const std::vector<std::size_t>& Failure table.
     */
  [[nodiscard]] const std::vector<std::size_t> &failure_table() const noexcept {
    return pi_;
  }

  /**
     * @brief Finds the longest prefix of X that is a suffix of Y in O(m) time.
     * 
     * Feeds the characters of Y through the KMP automaton of X. After processing all m
     * characters of Y, the automaton state j is exactly the length of the longest prefix
     * of X that matches a suffix of Y.
     * 
     * @param y String Y whose suffix is matched.
     * @return [[nodiscard]] PrefixSuffixOverlap<CharT, Traits> Overlap result.
     * 
     * @section complexity
     * - **Time Complexity:** $\mathcal{O}(m)$, where $m = |Y|$.
     * - **Auxiliary Space:** $\mathcal{O}(1)$ beyond the result struct.
     */
  [[nodiscard]] PrefixSuffixOverlap<CharT, Traits>
  find_overlap_with(StringView y) const {
    PrefixSuffixOverlap<CharT, Traits> result;
    const std::size_t n = x_.size();
    const std::size_t m = y.size();

    if (n == 0 || m == 0) {
      result.matching_slice = x_.substr(0, 0);
      return result;
    }

    std::size_t j = 0; // State in X

    for (std::size_t i = 0; i < m; ++i) {
      while (j > 0 && !Traits::eq(y[i], x_[j])) {
        j = pi_[j - 1];
      }
      if (Traits::eq(y[i], x_[j])) {
        ++j;
      }

      if (j == n && i + 1 < m) {
        // Completely matched X inside Y; continue matching for longer suffix
        j = pi_[n - 1];
      }
    }

    result.length = j;
    result.matching_slice = x_.substr(0, j);
    if (j > 0) {
      result.suffix_start_index = m - j;

      // Collect all valid prefix-suffix overlap lengths via failure links
      std::size_t k = j;
      while (k > 0) {
        result.all_overlap_lengths.push_back(k);
        k = pi_[k - 1];
      }
    }

    return result;
  }
};

// Deduction guides
template <typename CharT, typename Traits>
PrefixSuffixMatcher(std::basic_string_view<CharT, Traits>)
    -> PrefixSuffixMatcher<CharT, Traits>;

template <typename CharT, typename Traits, typename Allocator>
PrefixSuffixMatcher(const std::basic_string<CharT, Traits, Allocator> &)
    -> PrefixSuffixMatcher<CharT, Traits>;

template <typename CharT>
PrefixSuffixMatcher(const CharT *) -> PrefixSuffixMatcher<CharT>;

/**
 * @brief Finds the longest prefix of string X that is a suffix of string Y in O(n + m) time.
 * 
 * Non-template overload for std::string_view for convenient implicit conversions from
 * const char* and std::string.
 * 
 * @param x String X of length n (prefix source).
 * @param y String Y of length m (suffix source).
 * @return [[nodiscard]] PrefixSuffixOverlap<char> Result containing length and string views.
 * 
 * @section complexity
 * - **Time Complexity:** $\mathcal{O}(n + m)$ worst-case.
 * - **Auxiliary Space:** $\mathcal{O}(n)$ auxiliary space to store the KMP failure table of X.
 */
[[nodiscard]] inline PrefixSuffixOverlap<char>
find_longest_prefix_suffix_overlap(std::string_view x, std::string_view y) {
  PrefixSuffixMatcher<char> matcher(x);
  return matcher.find_overlap_with(y);
}

/**
 * @brief Templated overload for custom character types.
 */
template <typename CharT, typename Traits = std::char_traits<CharT>>
[[nodiscard]] inline PrefixSuffixOverlap<CharT, Traits>
find_longest_prefix_suffix_overlap(std::basic_string_view<CharT, Traits> x,
                                   std::basic_string_view<CharT, Traits> y) {
  PrefixSuffixMatcher<CharT, Traits> matcher(x);
  return matcher.find_overlap_with(y);
}

} // namespace dsa

#endif // PREFIX_SUFFIX_OVERLAP_HPP
