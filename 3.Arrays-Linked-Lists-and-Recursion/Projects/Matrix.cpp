// File: Matrix.cpp

#include "Matrix.hpp"

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

// =============================================================================
// Helper Functions
// =============================================================================

void Matrix::validateBounds(std::size_t row, std::size_t col) const {
  if (row >= numRows || col >= numCols) {
    throw std::out_of_range("Matrix index out of bounds: (" +
                            std::to_string(row) + ", " + std::to_string(col) +
                            ") for matrix of size " + std::to_string(numRows) +
                            "x" + std::to_string(numCols));
  }
}

// =============================================================================
// Constructors
// =============================================================================

Matrix::Matrix(std::size_t rows, std::size_t cols, int initVal)
    : numRows(rows), numCols(cols),
      data(rows, std::vector<int>(cols, initVal)) {}

Matrix::Matrix(std::vector<std::vector<int>> grid) {
  if (grid.empty()) {
    numRows = 0;
    numCols = 0;
    data.clear();
    return;
  }

  numRows = grid.size();
  numCols = grid[0].size();

  for (std::size_t i = 1; i < numRows; ++i) {
    if (grid[i].size() != numCols) {
      throw std::invalid_argument("Invalid 2D vector for Matrix: row " +
                                  std::to_string(i) + " has size " +
                                  std::to_string(grid[i].size()) +
                                  ", expected " + std::to_string(numCols));
    }
  }

  data = std::move(grid);
}

Matrix::Matrix(std::initializer_list<std::initializer_list<int>> list) {
  if (list.size() == 0) {
    numRows = 0;
    numCols = 0;
    return;
  }

  numRows = list.size();
  numCols = list.begin()->size();

  data.reserve(numRows);
  std::size_t rowIdx = 0;
  for (const auto &rowList : list) {
    if (rowList.size() != numCols) {
      throw std::invalid_argument("Invalid initializer_list for Matrix: row " +
                                  std::to_string(rowIdx) + " has size " +
                                  std::to_string(rowList.size()) +
                                  ", expected " + std::to_string(numCols));
    }
    data.emplace_back(rowList.begin(), rowList.end());
    ++rowIdx;
  }
}

// =============================================================================
// Element Accessors and Mutators
// =============================================================================

int Matrix::get(std::size_t row, std::size_t col) const {
  validateBounds(row, col);
  return data[row][col];
}

void Matrix::set(std::size_t row, std::size_t col, int value) {
  validateBounds(row, col);
  data[row][col] = value;
}

int &Matrix::operator()(std::size_t row, std::size_t col) {
  validateBounds(row, col);
  return data[row][col];
}

const int &Matrix::operator()(std::size_t row, std::size_t col) const {
  validateBounds(row, col);
  return data[row][col];
}

std::vector<int> &Matrix::operator[](std::size_t row) { return data[row]; }

const std::vector<int> &Matrix::operator[](std::size_t row) const {
  return data[row];
}

// =============================================================================
// Comparison Operators
// =============================================================================

bool Matrix::operator==(const Matrix &other) const noexcept {
  if (numRows != other.numRows || numCols != other.numCols) {
    return false;
  }
  return data == other.data;
}

bool Matrix::operator!=(const Matrix &other) const noexcept {
  return !(*this == other);
}

// =============================================================================
// Arithmetic Operators
// =============================================================================

Matrix &Matrix::operator+=(const Matrix &other) {
  if (numRows != other.numRows || numCols != other.numCols) {
    throw std::invalid_argument(
        "Matrix addition dimension mismatch: cannot add (" +
        std::to_string(numRows) + "x" + std::to_string(numCols) + ") and (" +
        std::to_string(other.numRows) + "x" + std::to_string(other.numCols) +
        ")");
  }

  for (std::size_t i = 0; i < numRows; ++i) {
    for (std::size_t j = 0; j < numCols; ++j) {
      data[i][j] += other.data[i][j];
    }
  }
  return *this;
}

Matrix operator+(Matrix lhs, const Matrix &rhs) {
  lhs += rhs;
  return lhs;
}

Matrix operator*(const Matrix &lhs, const Matrix &rhs) {
  if (lhs.numCols != rhs.numRows) {
    throw std::invalid_argument("Matrix multiplication dimension mismatch: "
                                "inner dimensions must match. (" +
                                std::to_string(lhs.numRows) + "x" +
                                std::to_string(lhs.numCols) + ") * (" +
                                std::to_string(rhs.numRows) + "x" +
                                std::to_string(rhs.numCols) + ")");
  }

  if (lhs.empty() || rhs.empty()) {
    return Matrix(lhs.numRows, rhs.numCols, 0);
  }

  Matrix result(lhs.numRows, rhs.numCols, 0);

  // Cache-friendly loop order (i -> k -> j) for spatial locality
  for (std::size_t i = 0; i < lhs.numRows; ++i) {
    for (std::size_t k = 0; k < lhs.numCols; ++k) {
      const int lhsVal = lhs.data[i][k];
      for (std::size_t j = 0; j < rhs.numCols; ++j) {
        result.data[i][j] += lhsVal * rhs.data[k][j];
      }
    }
  }

  return result;
}

// =============================================================================
// Stream Insertion Operator
// =============================================================================

std::ostream &operator<<(std::ostream &os, const Matrix &m) {
  if (m.empty()) {
    os << "[] (empty matrix " << m.numRows << "x" << m.numCols << ")\n";
    return os;
  }

  for (std::size_t i = 0; i < m.numRows; ++i) {
    os << "[ ";
    for (std::size_t j = 0; j < m.numCols; ++j) {
      os << std::setw(5) << m.data[i][j] << " ";
    }
    os << "]\n";
  }
  return os;
}
