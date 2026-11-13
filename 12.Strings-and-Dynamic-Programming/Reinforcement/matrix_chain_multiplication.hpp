/**
 * @file matrix_chain_multiplication.hpp
 * @brief Solution for the Matrix Chain Multiplication problem.
 * 
 * Provides an optimal algorithm to determine the minimum number of scalar multiplications
 * required to multiply a chain of matrices, and constructs the fully parenthesized expression
 * achieving that minimum.
 */

#ifndef MATRIX_CHAIN_MULTIPLICATION_HPP
#define MATRIX_CHAIN_MULTIPLICATION_HPP

#include <cstddef>
#include <cstdint>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace dsa {

/**
 * @struct MatrixChainResult
 * @brief Encapsulates the results of matrix chain multiplication optimization.
 */
struct MatrixChainResult {
  /// Minimum scalar multiplications required.
  std::uint64_t min_operations{0};

  /// Fully parenthesized expression string (e.g., "((A1 * (A2 * A3)) * ((A4 * A5) * A6))").
  std::string parenthesized_expression;

  /// Dynamic programming cost table where cost_table[i][j] stores the min cost to multiply A_i .. A_j.
  std::vector<std::vector<std::uint64_t>> cost_table;

  /// Split table where split_table[i][j] stores the index k where the optimal split occurs (i <= k < j).
  std::vector<std::vector<std::size_t>> split_table;
};

/**
 * @class MatrixChainOptimizer
 * @brief Computes optimal matrix chain multiplication parenthesization using dynamic programming.
 * 
 * Given a sequence of $n$ matrices $\langle A_0, A_1, \dots, A_{n-1} \rangle$ with dimensions specified
 * by a vector $P = \langle p_0, p_1, \dots, p_n \rangle$ such that matrix $A_i$ has dimension $p_i \times p_{i+1}$,
 * this class calculates the minimum number of scalar multiplications required and constructs
 * the optimal fully parenthesized multiplication expression.
 */
class MatrixChainOptimizer {
public:
  /**
     * @brief Default constructor creating an empty optimizer.
     * 
     * @section complexity
     * - **Time Complexity:** $\mathcal{O}(1)$
     * - **Auxiliary Space:** $\mathcal{O}(1)$
     */
  MatrixChainOptimizer() = default;

  /**
     * @brief Constructs an optimizer with the given matrix chain dimensions and optional custom matrix names.
     * 
     * @param dimensions Vector of dimensions of size $n + 1$ defining $n$ matrices.
     * @param matrix_names Optional vector of $n$ custom string identifiers for the matrices.
     * @throws std::invalid_argument If matrix_names is provided and its size is not equal to $n$.
     * 
     * @section complexity
     * - **Time Complexity:** $\mathcal{O}(n)$ to initialize and validate input containers.
     * - **Auxiliary Space:** $\mathcal{O}(n)$ auxiliary storage for dimensions and names.
     */
  explicit MatrixChainOptimizer(std::vector<std::size_t> dimensions,
                                std::vector<std::string> matrix_names = {})
      : m_dimensions(std::move(dimensions)),
        m_matrix_names(std::move(matrix_names)) {
    validate_and_initialize_names();
  }

  /**
     * @brief Updates the dimensions and matrix names for the optimizer.
     * 
     * @param dimensions Vector of dimensions of size $n + 1$.
     * @param matrix_names Optional vector of $n$ custom string identifiers for the matrices.
     * @throws std::invalid_argument If matrix_names is non-empty and its size != dimensions.size() - 1.
     * 
     * @section complexity
     * - **Time Complexity:** $\mathcal{O}(n)$
     * - **Auxiliary Space:** $\mathcal{O}(n)$
     */
  void set_dimensions(std::vector<std::size_t> dimensions,
                      std::vector<std::string> matrix_names = {}) {
    m_dimensions = std::move(dimensions);
    m_matrix_names = std::move(matrix_names);
    validate_and_initialize_names();
  }

  /**
     * @brief Returns the number of matrices in the chain.
     * 
     * @return [[nodiscard]] std::size_t Number of matrices $n$.
     * 
     * @section complexity
     * - **Time Complexity:** $\mathcal{O}(1)$
     * - **Auxiliary Space:** $\mathcal{O}(1)$
     */
  [[nodiscard]] std::size_t matrix_count() const noexcept {
    return (m_dimensions.size() >= 2) ? (m_dimensions.size() - 1) : 0;
  }

  /**
     * @brief Computes the optimal parenthesization and minimum scalar multiplications.
     * 
     * Uses bottom-up dynamic programming considering sub-chains of increasing lengths $L = 2, \dots, n$.
     * 
     * @return [[nodiscard]] MatrixChainResult Struct containing min operations, parenthesized string,
     *                                          and memoized cost and split tables.
     * 
     * @section complexity
     * - **Time Complexity:** $\mathcal{O}(n^3)$, where $n$ is the number of matrices. There are $\mathcal{O}(n^2)$
     *   subproblems, each evaluating at most $n$ split positions. String reconstruction takes $\mathcal{O}(n)$ time.
     * - **Auxiliary Space:** $\mathcal{O}(n^2)$ space for the $n \times n$ cost and split tables, plus $\mathcal{O}(n)$
     *   recursion depth and string buffer space for the output expression.
     */
  [[nodiscard]] MatrixChainResult solve() const {
    const std::size_t n = matrix_count();
    MatrixChainResult result;

    if (n == 0) {
      result.min_operations = 0;
      result.parenthesized_expression = "";
      return result;
    }

    if (n == 1) {
      result.min_operations = 0;
      result.parenthesized_expression = m_matrix_names.front();
      result.cost_table = {{0}};
      result.split_table = {{0}};
      return result;
    }

    // Initialize cost and split DP tables of size n x n
    result.cost_table.assign(n, std::vector<std::uint64_t>(n, 0));
    result.split_table.assign(n, std::vector<std::size_t>(n, 0));

    // Subchain length L ranges from 2 to n
    for (std::size_t L = 2; L <= n; ++L) {
      for (std::size_t i = 0; i <= n - L; ++i) {
        const std::size_t j = i + L - 1;
        result.cost_table[i][j] = std::numeric_limits<std::uint64_t>::max();

        for (std::size_t k = i; k < j; ++k) {
          const std::uint64_t mult_cost =
              static_cast<std::uint64_t>(m_dimensions[i]) *
              static_cast<std::uint64_t>(m_dimensions[k + 1]) *
              static_cast<std::uint64_t>(m_dimensions[j + 1]);

          const std::uint64_t total_cost =
              result.cost_table[i][k] + result.cost_table[k + 1][j] + mult_cost;

          if (total_cost < result.cost_table[i][j]) {
            result.cost_table[i][j] = total_cost;
            result.split_table[i][j] = k;
          }
        }
      }
    }

    result.min_operations = result.cost_table[0][n - 1];

    // Reconstruct fully parenthesized expression
    std::string expr;
    // Reserve an estimate of bytes: ~7 chars per matrix plus parentheses
    expr.reserve(n * 10);
    build_parenthesized_expression(0, n - 1, result.split_table, expr);
    result.parenthesized_expression = std::move(expr);

    return result;
  }

private:
  std::vector<std::size_t> m_dimensions;
  std::vector<std::string> m_matrix_names;

  /**
     * @brief Validates input sizes and generates default 1-based matrix labels (e.g. A1, A2, ...)
     *        if custom names were not provided.
     */
  void validate_and_initialize_names() {
    const std::size_t n = matrix_count();
    if (m_matrix_names.empty()) {
      m_matrix_names.reserve(n);
      for (std::size_t i = 1; i <= n; ++i) {
        m_matrix_names.push_back("A" + std::to_string(i));
      }
    } else if (m_matrix_names.size() != n) {
      throw std::invalid_argument("MatrixChainOptimizer: matrix_names size (" +
                                  std::to_string(m_matrix_names.size()) +
                                  ") must match number of matrices (" +
                                  std::to_string(n) + ").");
    }
  }

  /**
     * @brief Recursively constructs the fully parenthesized expression from the split table.
     * 
     * @param i Starting index of matrix range (inclusive).
     * @param j Ending index of matrix range (inclusive).
     * @param split Split table storing optimal k values.
     * @param out String accumulator for the parenthesized output.
     * 
     * @section complexity
     * - **Time Complexity:** $\mathcal{O}(j - i + 1)$
     * - **Auxiliary Space:** $\mathcal{O}(j - i + 1)$ recursion stack frames.
     */
  void build_parenthesized_expression(
      std::size_t i, std::size_t j,
      const std::vector<std::vector<std::size_t>> &split,
      std::string &out) const {
    if (i == j) {
      out += m_matrix_names[i];
      return;
    }

    out += '(';
    build_parenthesized_expression(i, split[i][j], split, out);
    out += " * ";
    build_parenthesized_expression(split[i][j] + 1, j, split, out);
    out += ')';
  }
};

/**
 * @brief Convenience free function to solve the matrix chain multiplication problem.
 * 
 * @param dimensions Vector of dimensions of size $n + 1$ defining $n$ matrices.
 * @param matrix_names Optional custom matrix names vector of size $n$.
 * @return [[nodiscard]] MatrixChainResult Containing minimum operations and parenthesized string.
 * 
 * @section complexity
 * - **Time Complexity:** $\mathcal{O}(n^3)$
 * - **Auxiliary Space:** $\mathcal{O}(n^2)$
 */
[[nodiscard]] inline MatrixChainResult
optimize_matrix_chain(const std::vector<std::size_t> &dimensions,
                      const std::vector<std::string> &matrix_names = {}) {
  MatrixChainOptimizer optimizer(dimensions, matrix_names);
  return optimizer.solve();
}

} // namespace dsa

#endif // MATRIX_CHAIN_MULTIPLICATION_HPP
