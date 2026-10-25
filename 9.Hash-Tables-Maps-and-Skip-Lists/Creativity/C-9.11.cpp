// File : C-9.11.cpp

/*
    C-9.11 Suppose that each row of an n × n array A consists of 1’s and 0’s
   such that, in any row of A, all the 1’s come before any 0’s in that row.
   Assuming A is already in memory, describe a method running in O(n log n) time
   (not O(n2) time!) for counting the number of 1’s in A.
*/

#include "matrix_ones_counter.hpp"
#include <cassert>
#include <iostream>
#include <vector>

void runTests() {
  std::cout << "Running Matrix Ones Counter tests...\n";
  using Matrix = std::vector<std::vector<int>>;
  // Test 1: Standard 4x4 matrix with 1s followed by 0s per row
  // Row 0: 1 1 0 0 (2 ones)
  // Row 1: 1 1 1 0 (3 ones)
  // Row 2: 1 0 0 0 (1 one)
  // Row 3: 0 0 0 0 (0 ones)
  // Total 1s = 6
  Matrix matrix1 = {{1, 1, 0, 0}, {1, 1, 1, 0}, {1, 0, 0, 0}, {0, 0, 0, 0}};
  assert(countOnesInMatrix(matrix1) == 6);

  // Test 2: All 1s matrix (n x n)
  Matrix matrixAllOnes = {{1, 1, 1}, {1, 1, 1}, {1, 1, 1}};
  assert(countOnesInMatrix(matrixAllOnes) == 9);

  // Test 3: All 0s matrix (n x n)
  Matrix matrixAllZeros = {{0, 0}, {0, 0}};
  assert(countOnesInMatrix(matrixAllZeros) == 0);

  // Test 4: Single element matrix
  Matrix matrixSingle = {{1}};
  assert(countOnesInMatrix(matrixSingle) == 1);

  std::cout << "All Matrix Ones Counter tests passed successfully!\n";
}

int main() {
  runTests();
  return 0;
}