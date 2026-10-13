// File: Matrix.hpp

#ifndef MATRIX_HPP
#define MATRIX_HPP

#include <cstddef>
#include <initializer_list>
#include <iosfwd>
#include <vector>

/**
 * @class Matrix
 * @brief Represents a mathematical matrix of integers with arbitrary
 * dimensions.
 *
 * Implements fundamental matrix operations including addition, matrix
 * multiplication, element access, and stream serialization with robust bounds
 * and dimension checking.
 */
class Matrix {
private:
  std::size_t numRows{0};
  std::size_t numCols{0};
  std::vector<std::vector<int>> data;

  /**
   * @brief Validates if index pair (row, col) is within matrix boundaries.
   * @throws std::out_of_range if row or col are out of bounds.
   */
  void validateBounds(std::size_t row, std::size_t col) const;

public:
  // =========================================================================
  // Constructors & Rule of Zero
  // =========================================================================

  /**
   * @brief Default constructor creating an empty 0x0 matrix.
   */
  Matrix() = default;

  /**
   * @brief Parameterized constructor initializing a matrix with given
   * dimensions.
   * @param rows Number of rows.
   * @param cols Number of columns.
   * @param initVal Initial value for all matrix elements (defaults to 0).
   */
  Matrix(std::size_t rows, std::size_t cols, int initVal = 0);

  /**
   * @brief Constructs a Matrix from a 2D std::vector of integers.
   * @param grid 2D vector of integers.
   * @throws std::invalid_argument if rows in grid have ragged/unequal column
   * lengths.
   */
  Matrix(std::vector<std::vector<int>> grid);

  /**
   * @brief Constructs a Matrix using nested initializer lists.
   * @example Matrix m = {{1, 2, 3}, {4, 5, 6}};
   * @throws std::invalid_argument if rows have ragged/unequal column lengths.
   */
  Matrix(std::initializer_list<std::initializer_list<int>> list);

  Matrix(const Matrix &) = default;
  Matrix(Matrix &&) noexcept = default;
  Matrix &operator=(const Matrix &) = default;
  Matrix &operator=(Matrix &&) noexcept = default;
  ~Matrix() = default;

  // =========================================================================
  // Dimension Accessors & State Inquiries
  // =========================================================================

  [[nodiscard]] std::size_t rows() const noexcept { return numRows; }
  [[nodiscard]] std::size_t cols() const noexcept { return numCols; }
  [[nodiscard]] bool empty() const noexcept {
    return numRows == 0 || numCols == 0;
  }

  // =========================================================================
  // Element Accessors and Mutators
  // =========================================================================

  /**
   * @brief Safe element getter with bounds checking.
   */
  [[nodiscard]] int get(std::size_t row, std::size_t col) const;

  /**
   * @brief Safe element setter with bounds checking.
   */
  void set(std::size_t row, std::size_t col, int value);

  /**
   * @brief Checked function call operator (round bracket access m(r, c)).
   */
  int &operator()(std::size_t row, std::size_t col);
  const int &operator()(std::size_t row, std::size_t col) const;

  /**
   * @brief Subscript operator returning row vector for m[r][c] syntax.
   */
  std::vector<int> &operator[](std::size_t row);
  const std::vector<int> &operator[](std::size_t row) const;

  // =========================================================================
  // Comparison Operators
  // =========================================================================

  bool operator==(const Matrix &other) const noexcept;
  bool operator!=(const Matrix &other) const noexcept;

  // =========================================================================
  // Arithmetic Operators
  // =========================================================================

  /**
   * @brief In-place matrix addition.
   * @throws std::invalid_argument if dimensions do not match.
   */
  Matrix &operator+=(const Matrix &other);

  /**
   * @brief Binary matrix addition.
   * @throws std::invalid_argument if dimensions do not match.
   */
  friend Matrix operator+(Matrix lhs, const Matrix &rhs);

  /**
   * @brief Matrix multiplication.
   * @throws std::invalid_argument if lhs.cols() != rhs.rows().
   */
  friend Matrix operator*(const Matrix &lhs, const Matrix &rhs);

  // =========================================================================
  // Stream Insertion Operator
  // =========================================================================

  /**
   * @brief Stream insertion operator to print matrices cleanly.
   */
  friend std::ostream &operator<<(std::ostream &os, const Matrix &m);
};

#endif // MATRIX_HPP
