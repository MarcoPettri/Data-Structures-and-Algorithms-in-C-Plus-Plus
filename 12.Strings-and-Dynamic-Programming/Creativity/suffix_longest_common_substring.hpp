/**
 * @file suffix_longest_common_substring.hpp
 * @brief Dynamic programming algorithm for computing all B(i, j) values of suffix longest common substrings (C-12.23).
 * 
 * Given strings x of length n and y of length m, computes B(i, j), defined as the length
 * of the longest common substring of the suffix of length i in x and the suffix of length j in y,
 * for all 1 <= i <= n and 1 <= j <= m in O(nm) time.
 */

#ifndef SUFFIX_LONGEST_COMMON_SUBSTRING_HPP
#define SUFFIX_LONGEST_COMMON_SUBSTRING_HPP

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace dsa {

/**
 * @class SuffixLCSTable
 * @brief Represents the 2D matrix of B(i, j) values for all suffix lengths 0 <= i <= n and 0 <= j <= m.
 */
class SuffixLCSTable {
private:
  std::size_t n_{0};
  std::size_t m_{0};
  std::size_t cols_{0};
  std::vector<std::size_t> data_{};

public:
  SuffixLCSTable() = default;

  /**
     * @brief Constructs a table with dimensions (n + 1) x (m + 1).
     * @param n Length of string x.
     * @param m Length of string y.
     */
  SuffixLCSTable(std::size_t n, std::size_t m)
      : n_(n), m_(m), cols_(m + 1), data_((n + 1) * (m + 1), 0) {}

  /**
     * @brief Accesses B(i, j) using 1-based suffix length indexing without bounds checking.
     * 
     * @param i Suffix length in x (0 <= i <= n).
     * @param j Suffix length in y (0 <= j <= m).
     * @return [[nodiscard]] std::size_t Length of longest common substring.
     */
  [[nodiscard]] std::size_t operator()(std::size_t i,
                                       std::size_t j) const noexcept {
    return data_[i * cols_ + j];
  }

  /**
     * @brief Mutable reference for filling table entries.
     */
  [[nodiscard]] std::size_t &operator()(std::size_t i, std::size_t j) noexcept {
    return data_[i * cols_ + j];
  }

  /**
     * @brief Accesses B(i, j) with strict bounds checking.
     * 
     * @param i Suffix length in x (0 <= i <= n).
     * @param j Suffix length in y (0 <= j <= m).
     * @return [[nodiscard]] std::size_t Length of longest common substring.
     * @throws std::out_of_range If i > n or j > m.
     */
  [[nodiscard]] std::size_t at(std::size_t i, std::size_t j) const {
    if (i > n_ || j > m_) {
      throw std::out_of_range("SuffixLCSTable index out of bounds.");
    }
    return data_[i * cols_ + j];
  }

  /**
     * @brief Length of string x (maximum suffix length i).
     */
  [[nodiscard]] std::size_t n() const noexcept { return n_; }

  /**
     * @brief Length of string y (maximum suffix length j).
     */
  [[nodiscard]] std::size_t m() const noexcept { return m_; }

  /**
     * @brief Returns the overall longest common substring length, B(n, m).
     */
  [[nodiscard]] std::size_t overall_longest() const noexcept {
    return (n_ > 0 && m_ > 0) ? (*this)(n_, m_) : 0;
  }

  /**
     * @brief Returns raw underlying flat contiguous storage.
     */
  [[nodiscard]] const std::vector<std::size_t> &raw_data() const noexcept {
    return data_;
  }
};

/**
 * @class SuffixLCS
 * @brief Dynamic programming engine for computing all B(i, j) values.
 */
class SuffixLCS {
public:
  /**
     * @brief Computes all B(i, j) values for i = 1, ..., n and j = 1, ..., m in O(nm) time.
     * 
     * Definition:
     * - Suffix of length i in x: x^{(i)} = x[n - i .. n - 1].
     * - Suffix of length j in y: y^{(j)} = y[m - j .. m - 1].
     * - B(i, j): length of the longest common substring of x^{(i)} and y^{(j)}.
     * 
     * Recurrence Relation:
     * Let C(i, j) be the length of the longest common prefix of x^{(i)} and y^{(j)}:
     * $$C(i, j) = \begin{cases}
     *   1 + C(i - 1, j - 1) & \text{if } x[n - i] = y[m - j] \\
     *   0 & \text{if } x[n - i] \ne y[m - j]
     * \end{cases}$$
     * 
     * Then B(i, j) satisfies:
     * $$B(i, j) = \max\Big( B(i - 1, j), \; B(i, j - 1), \; C(i, j) \Big)$$
     * with base cases B(0, j) = 0 and B(i, 0) = 0.
     * 
     * @tparam CharT Character type.
     * @tparam Traits Character traits type.
     * @param x Source string x of length n.
     * @param y Target string y of length m.
     * @return [[nodiscard]] SuffixLCSTable Matrix holding all B(i, j) values.
     * 
     * @section complexity
     * - **Time Complexity:** $\mathcal{O}(n \cdot m)$, evaluating each table cell in $\mathcal{O}(1)$ time.
     * - **Auxiliary Space:** $\mathcal{O}(n \cdot m)$ for the output table plus $\mathcal{O}(m)$ auxiliary row memory.
     */
  template <typename CharT = char, typename Traits = std::char_traits<CharT>>
  [[nodiscard]] static SuffixLCSTable
  compute_all(std::basic_string_view<CharT, Traits> x,
              std::basic_string_view<CharT, Traits> y) {
    const std::size_t n = x.size();
    const std::size_t m = y.size();

    SuffixLCSTable table(n, m);
    if (n == 0 || m == 0) {
      return table;
    }

    // Two alternating rows for C(i, j) to achieve O(m) temporary auxiliary space
    std::vector<std::size_t> prev_c(m + 1, 0);
    std::vector<std::size_t> curr_c(m + 1, 0);

    for (std::size_t i = 1; i <= n; ++i) {
      const CharT char_x = x[n - i];

      for (std::size_t j = 1; j <= m; ++j) {
        const CharT char_y = y[m - j];

        // Compute C(i, j): common prefix of suffixes x^{(i)} and y^{(j)}
        if (Traits::eq(char_x, char_y)) {
          curr_c[j] = 1 + prev_c[j - 1];
        } else {
          curr_c[j] = 0;
        }

        // Compute B(i, j) = max(B(i - 1, j), B(i, j - 1), C(i, j))
        const std::size_t val_from_x = table(i - 1, j);
        const std::size_t val_from_y = table(i, j - 1);
        const std::size_t val_from_c = curr_c[j];

        table(i, j) = std::max({val_from_x, val_from_y, val_from_c});
      }

      std::swap(prev_c, curr_c);
      std::fill(curr_c.begin(), curr_c.end(), std::size_t{0});
    }

    return table;
  }
};

/**
 * @brief Computes all B(i, j) values for strings x and y in O(nm) time.
 * 
 * Non-template overload for std::string_view for convenient implicit conversions from
 * const char* and std::string.
 * 
 * @param x String x of length n.
 * @param y String y of length m.
 * @return [[nodiscard]] SuffixLCSTable Complete table of all B(i, j) values.
 */
[[nodiscard]] inline SuffixLCSTable
compute_suffix_lcs_table(std::string_view x, std::string_view y) {
  return SuffixLCS::compute_all(x, y);
}

/**
 * @brief Templated overload for custom character types.
 */
template <typename CharT, typename Traits = std::char_traits<CharT>>
[[nodiscard]] inline SuffixLCSTable
compute_suffix_lcs_table(std::basic_string_view<CharT, Traits> x,
                         std::basic_string_view<CharT, Traits> y) {
  return SuffixLCS::compute_all(x, y);
}

} // namespace dsa

#endif // SUFFIX_LONGEST_COMMON_SUBSTRING_HPP
