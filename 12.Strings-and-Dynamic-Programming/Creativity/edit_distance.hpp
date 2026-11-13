/**
 * @file edit_distance.hpp
 * @brief Dynamic programming algorithm for computing the Levenshtein Edit Distance (C-12.20).
 * 
 * Computes the minimum number of character insertions, deletions, and replacements
 * needed to transform a string X of length n into a string Y of length m in O(nm) time.
 */

#ifndef EDIT_DISTANCE_HPP
#define EDIT_DISTANCE_HPP

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace dsa {

/**
 * @enum EditOperation
 * @brief Represents the atomic edit operation applied to a character.
 */
enum class EditOperation {
  Match,   ///< Character matched without edit (cost 0).
  Insert,  ///< Character inserted into X from Y.
  Delete,  ///< Character deleted from X.
  Replace  ///< Character in X replaced by character from Y.
};

/**
 * @struct EditStep
 * @brief Represents a single edit step in the transformation transcript.
 * 
 * @tparam CharT Character type.
 */
template <typename CharT = char> struct EditStep {
  EditOperation operation{EditOperation::Match};
  std::optional<CharT> source_char{std::nullopt};
  std::optional<CharT> target_char{std::nullopt};
  std::size_t source_index{0};
  std::size_t target_index{0};

  [[nodiscard]] bool operator==(const EditStep &other) const noexcept {
    return operation == other.operation && source_char == other.source_char &&
           target_char == other.target_char &&
           source_index == other.source_index &&
           target_index == other.target_index;
  }
};

/**
 * @struct EditDistanceResult
 * @brief Holds the calculated edit distance and the reconstructed sequence of edit operations.
 * 
 * @tparam CharT Character type.
 */
template <typename CharT = char> struct EditDistanceResult {
  /// Total edit distance (number of edits).
  std::size_t distance{0};

  /// Chronological list of edit operations transforming X into Y.
  std::vector<EditStep<CharT>> script{};

  /// Alignment representation of X with gaps ('-').
  std::basic_string<CharT> aligned_x{};

  /// Alignment representation of Y with gaps ('-').
  std::basic_string<CharT> aligned_y{};

  /**
     * @brief Verifies that applying the edit script to string X produces string Y exactly.
     * 
     * @tparam Traits Character traits type.
     * @param x Original source string.
     * @param y Target string.
     * @return [[nodiscard]] bool True if script accurately transforms X into Y.
     * 
     * @section complexity
     * - **Time Complexity:** $\mathcal{O}(|script|) = \mathcal{O}(n + m)$.
     * - **Auxiliary Space:** $\mathcal{O}(m)$ for the constructed string.
     */
  [[nodiscard]] bool
  verify_transformation(std::basic_string_view<CharT> x,
                        std::basic_string_view<CharT> y) const {
    using Traits = std::char_traits<CharT>;
    std::basic_string<CharT> current;
    std::size_t cost_sum = 0;

    for (const auto &step : script) {
      switch (step.operation) {
      case EditOperation::Match:
        if (!step.source_char.has_value() || !step.target_char.has_value() ||
            !Traits::eq(*step.source_char, *step.target_char)) {
          return false;
        }
        current.push_back(*step.source_char);
        break;
      case EditOperation::Insert:
        if (!step.target_char.has_value())
          return false;
        current.push_back(*step.target_char);
        ++cost_sum;
        break;
      case EditOperation::Delete:
        // Character from X is omitted
        ++cost_sum;
        break;
      case EditOperation::Replace:
        if (!step.target_char.has_value())
          return false;
        current.push_back(*step.target_char);
        ++cost_sum;
        break;
      }
    }

    return cost_sum == distance && current == y;
  }
};

/**
 * @class EditDistance
 * @brief Dynamic programming solver for string edit distance (Levenshtein distance).
 */
class EditDistance {
public:
  /**
     * @brief Computes the edit distance between X and Y using space-optimized O(min(n, m)) memory.
     * 
     * Recurrence Relation:
     * $$DP[i][j] = \begin{cases}
     *   DP[i-1][j-1] & \text{if } X[i-1] = Y[j-1] \\
     *   1 + \min(DP[i-1][j-1], DP[i-1][j], DP[i][j-1]) & \text{if } X[i-1] \ne Y[j-1]
     * \end{cases}$$
     * 
     * @tparam CharT Character type.
     * @tparam Traits Character traits type.
     * @param x Source string of length n.
     * @param y Target string of length m.
     * @return [[nodiscard]] std::size_t Minimum edit distance.
     * 
     * @section complexity
     * - **Time Complexity:** $\mathcal{O}(n \cdot m)$.
     * - **Auxiliary Space:** $\mathcal{O}(\min(n, m))$ using two alternating rows.
     */
  template <typename CharT = char, typename Traits = std::char_traits<CharT>>
  [[nodiscard]] static std::size_t
  compute(std::basic_string_view<CharT, Traits> x,
          std::basic_string_view<CharT, Traits> y) noexcept {
    const std::size_t n = x.size();
    const std::size_t m = y.size();

    if (n == 0)
      return m;
    if (m == 0)
      return n;

    // Ensure y is the shorter string to minimize auxiliary row buffer
    if (n < m) {
      return compute(y, x);
    }

    // DP array of size m + 1
    std::vector<std::size_t> prev(m + 1);
    std::iota(prev.begin(), prev.end(), std::size_t{0});

    std::vector<std::size_t> curr(m + 1, 0);

    for (std::size_t i = 1; i <= n; ++i) {
      curr[0] = i;
      const CharT char_x = x[i - 1];

      for (std::size_t j = 1; j <= m; ++j) {
        const CharT char_y = y[j - 1];

        if (Traits::eq(char_x, char_y)) {
          curr[j] = prev[j - 1]; // Match
        } else {
          const std::size_t replace_cost = prev[j - 1] + 1;
          const std::size_t delete_cost = prev[j] + 1;
          const std::size_t insert_cost = curr[j - 1] + 1;
          curr[j] = std::min({replace_cost, delete_cost, insert_cost});
        }
      }
      std::swap(prev, curr);
    }

    return prev[m];
  }

  /**
     * @brief Computes edit distance and reconstructs the full alignment and edit script in O(nm) time and space.
     * 
     * @tparam CharT Character type.
     * @tparam Traits Character traits type.
     * @param x Source string of length n.
     * @param y Target string of length m.
     * @return [[nodiscard]] EditDistanceResult<CharT> Complete result with distance and transcript.
     * 
     * @section complexity
     * - **Time Complexity:** $\mathcal{O}(n \cdot m)$.
     * - **Auxiliary Space:** $\mathcal{O}(n \cdot m)$ for the full 2D matrix.
     */
  template <typename CharT = char, typename Traits = std::char_traits<CharT>>
  [[nodiscard]] static EditDistanceResult<CharT>
  compute_with_script(std::basic_string_view<CharT, Traits> x,
                      std::basic_string_view<CharT, Traits> y) {
    EditDistanceResult<CharT> result;
    const std::size_t n = x.size();
    const std::size_t m = y.size();

    // 2D DP matrix stored contiguously
    const std::size_t cols = m + 1;
    std::vector<std::size_t> dp((n + 1) * cols, 0);

    for (std::size_t i = 0; i <= n; ++i) {
      dp[i * cols + 0] = i;
    }
    for (std::size_t j = 0; j <= m; ++j) {
      dp[0 * cols + j] = j;
    }

    for (std::size_t i = 1; i <= n; ++i) {
      const CharT char_x = x[i - 1];
      for (std::size_t j = 1; j <= m; ++j) {
        const CharT char_y = y[j - 1];

        if (Traits::eq(char_x, char_y)) {
          dp[i * cols + j] = dp[(i - 1) * cols + (j - 1)];
        } else {
          const std::size_t replace_cost = dp[(i - 1) * cols + (j - 1)] + 1;
          const std::size_t delete_cost = dp[(i - 1) * cols + j] + 1;
          const std::size_t insert_cost = dp[i * cols + (j - 1)] + 1;
          dp[i * cols + j] = std::min({replace_cost, delete_cost, insert_cost});
        }
      }
    }

    result.distance = dp[n * cols + m];

    // Backtrack from (n, m) to (0, 0) to reconstruct optimal alignment and edit script
    std::size_t i = n;
    std::size_t j = m;

    std::vector<EditStep<CharT>> reversed_script;
    std::basic_string<CharT> rev_aligned_x;
    std::basic_string<CharT> rev_aligned_y;

    while (i > 0 || j > 0) {
      if (i > 0 && j > 0 && Traits::eq(x[i - 1], y[j - 1]) &&
          dp[i * cols + j] == dp[(i - 1) * cols + (j - 1)]) {
        // Match
        EditStep<CharT> step;
        step.operation = EditOperation::Match;
        step.source_char = x[i - 1];
        step.target_char = y[j - 1];
        step.source_index = i - 1;
        step.target_index = j - 1;
        reversed_script.push_back(step);

        rev_aligned_x.push_back(x[i - 1]);
        rev_aligned_y.push_back(y[j - 1]);
        --i;
        --j;
      } else if (i > 0 && j > 0 &&
                 dp[i * cols + j] == dp[(i - 1) * cols + (j - 1)] + 1) {
        // Replace
        EditStep<CharT> step;
        step.operation = EditOperation::Replace;
        step.source_char = x[i - 1];
        step.target_char = y[j - 1];
        step.source_index = i - 1;
        step.target_index = j - 1;
        reversed_script.push_back(step);

        rev_aligned_x.push_back(x[i - 1]);
        rev_aligned_y.push_back(y[j - 1]);
        --i;
        --j;
      } else if (i > 0 && dp[i * cols + j] == dp[(i - 1) * cols + j] + 1) {
        // Delete from X
        EditStep<CharT> step;
        step.operation = EditOperation::Delete;
        step.source_char = x[i - 1];
        step.target_char = std::nullopt;
        step.source_index = i - 1;
        step.target_index = j;
        reversed_script.push_back(step);

        rev_aligned_x.push_back(x[i - 1]);
        rev_aligned_y.push_back(static_cast<CharT>('-'));
        --i;
      } else {
        // Insert into X from Y
        EditStep<CharT> step;
        step.operation = EditOperation::Insert;
        step.source_char = std::nullopt;
        step.target_char = y[j - 1];
        step.source_index = i;
        step.target_index = j - 1;
        reversed_script.push_back(step);

        rev_aligned_x.push_back(static_cast<CharT>('-'));
        rev_aligned_y.push_back(y[j - 1]);
        --j;
      }
    }

    std::reverse(reversed_script.begin(), reversed_script.end());
    std::reverse(rev_aligned_x.begin(), rev_aligned_x.end());
    std::reverse(rev_aligned_y.begin(), rev_aligned_y.end());

    result.script = std::move(reversed_script);
    result.aligned_x = std::move(rev_aligned_x);
    result.aligned_y = std::move(rev_aligned_y);

    return result;
  }
};

/**
 * @brief Computes the edit distance between two strings in O(nm) time and O(min(n, m)) auxiliary space.
 * 
 * Non-template overload for std::string_view for convenient implicit conversions.
 * 
 * @param x Source string of length n.
 * @param y Target string of length m.
 * @return [[nodiscard]] std::size_t Minimum edit distance.
 */
[[nodiscard]] inline std::size_t
compute_edit_distance(std::string_view x, std::string_view y) noexcept {
  return EditDistance::compute(x, y);
}

/**
 * @brief Computes edit distance and detailed alignment/script in O(nm) time and space.
 * 
 * @param x Source string.
 * @param y Target string.
 * @return [[nodiscard]] EditDistanceResult<char> Result containing distance, script, and alignment.
 */
[[nodiscard]] inline EditDistanceResult<char>
compute_edit_distance_with_script(std::string_view x, std::string_view y) {
  return EditDistance::compute_with_script(x, y);
}

} // namespace dsa

#endif // EDIT_DISTANCE_HPP
