// File: P-3.2.cpp
/*
    P-3.2 Write a C++ program for a matrix class that can add and multiply
   arbitrary two-dimensional arrays of integers. Do this by overloading the
   addition ("+") and multiplication ("*") operators.
*/

#include "Matrix.hpp"

#include <iostream>
#include <stdexcept>
#include <vector>

// =============================================================================
// Comprehensive Demonstration in main()
// =============================================================================

int main() {
  std::cout << "========================================\n";
  std::cout << "     MATRIX CLASS DEMONSTRATION         \n";
  std::cout << "========================================\n\n";

  // 1. Instantiation demonstrations
  std::cout << "[1] Instantiating Matrices:\n";

  Matrix m1(2, 3, 5); // 2x3 filled with 5
  std::cout << "Matrix m1 (2x3 filled with 5):\n" << m1 << "\n";

  Matrix m2 = {{1, 2, 3}, {4, 5, 6}};
  std::cout << "Matrix m2 (2x3 from initializer list):\n" << m2 << "\n";

  std::vector<std::vector<int>> vec = {{7, 8, 9}, {10, 11, 12}};
  Matrix m3(vec);
  std::cout << "Matrix m3 (2x3 from std::vector<std::vector<int>>):\n"
            << m3 << "\n";

  // 2. Encapsulation: getters, setters, and operator()
  std::cout << "[2] Encapsulation & Element Access:\n";
  std::cout << "m2 dimensions: " << m2.rows() << " rows x " << m2.cols()
            << " cols\n";
  std::cout << "Original m2.get(0, 1): " << m2.get(0, 1) << "\n";
  m2.set(0, 1, 99);
  std::cout << "Modified m2 after m2.set(0, 1, 99):\n" << m2 << "\n";
  m2(0, 1) = 2; // Restore using operator()
  std::cout << "Restored m2 using m2(0, 1) = 2:\n" << m2 << "\n";

  // 3. Matrix Addition
  std::cout << "[3] Matrix Addition (m2 + m3):\n";
  Matrix sum = m2 + m3;
  std::cout << sum << "\n";

  // 4. Matrix Multiplication
  std::cout << "[4] Matrix Multiplication (A * B):\n";
  Matrix A = {{1, 2, 3}, {4, 5, 6}}; // 2x3

  Matrix B = {{7, 8}, {9, 1}, {2, 3}}; // 3x2

  std::cout << "Matrix A (2x3):\n" << A << "\n";
  std::cout << "Matrix B (3x2):\n" << B << "\n";

  Matrix product = A * B; // Expected: 2x2
  std::cout << "Product A * B (2x2):\n" << product << "\n";
  /*
     Calculation check:
     Row 0, Col 0: 1*7 + 2*9 + 3*2 = 7 + 18 + 6 = 31
     Row 0, Col 1: 1*8 + 2*1 + 3*3 = 8 + 2 + 9 = 19
     Row 1, Col 0: 4*7 + 5*9 + 6*2 = 28 + 45 + 12 = 85
     Row 1, Col 1: 4*8 + 5*1 + 6*3 = 32 + 5 + 18 = 55
  */

  // 5. Exception & Edge-Case Handling
  std::cout << "[5] Edge Case & Exception Handling:\n\n";

  // 5a. Out of bounds element access
  try {
    std::cout << "Attempting out-of-bounds access m2.get(10, 10)...\n";
    [[maybe_unused]] int val = m2.get(10, 10);
  } catch (const std::out_of_range &e) {
    std::cout << " Caught expected std::out_of_range: " << e.what() << "\n\n";
  }

  // 5b. Addition dimension mismatch
  try {
    std::cout
        << "Attempting addition of mismatched dimensions (2x3 + 3x2)...\n";
    Matrix invalidSum = A + B;
  } catch (const std::invalid_argument &e) {
    std::cout << " Caught expected std::invalid_argument: " << e.what()
              << "\n\n";
  }

  // 5c. Multiplication inner-dimension mismatch
  try {
    std::cout << "Attempting multiplication with mismatched inner dimensions "
                 "(2x3 * 2x3)...\n";
    Matrix invalidProd = A * m2; // 2x3 * 2x3
  } catch (const std::invalid_argument &e) {
    std::cout << " Caught expected std::invalid_argument: " << e.what()
              << "\n\n";
  }

  // 5d. Ragged matrix initialization detection
  try {
    std::cout
        << "Attempting to create ragged matrix with uneven row sizes...\n";
    Matrix ragged = {
        {1, 2, 3}, {4, 5} // Missing an element
    };
  } catch (const std::invalid_argument &e) {
    std::cout << " Caught expected std::invalid_argument: " << e.what()
              << "\n\n";
  }

  std::cout
      << "All matrix operations and exception tests completed successfully.\n";
  return 0;
}
