// File : C-9.3.cpp

/*
    C-9.3 Suppose we are given two ordered dictionaries S and T , each with n
   items, and that S and T are implemented by means of array-based ordered
   sequences. Describe an O(log2 n)-time algorithm for finding the kth small-
   est key in the union of the keys from S and T (assuming no duplicates).
*/

#include "union_kth_finder.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <vector>

void runTests() {
  std::cout << "Running union k-th smallest tests...\n";

  std::vector<int> S = {2, 5, 8, 12};
  std::vector<int> T = {1, 4, 9, 15};
  // Union sorted order: 1, 2, 4, 5, 8, 9, 12, 15

  // Test 1: k = 1 (Minimum element, located in T)
  assert(findKthSmallestInUnion(S, T, 1) == 1);

  // Test 2: k = 2 (Second element, located in S)
  assert(findKthSmallestInUnion(S, T, 2) == 2);

  // Test 3: k = 3 (Third element, located in T)
  assert(findKthSmallestInUnion(S, T, 3) == 4);

  // Test 4: k = 5 (Fifth element, located in S)
  assert(findKthSmallestInUnion(S, T, 5) == 8);

  // Test 5: k = 8 (Maximum element, located in T)
  assert(findKthSmallestInUnion(S, T, 8) == 15);

  // Test 6: Out of range lower bound check
  bool exceptionCaught = false;
  try {
    findKthSmallestInUnion(S, T, 0);
  } catch (const std::out_of_range &) {
    exceptionCaught = true;
  }
  assert(exceptionCaught);

  // Test 7: Out of range upper bound check
  exceptionCaught = false;
  try {
    findKthSmallestInUnion(S, T, 9);
  } catch (const std::out_of_range &) {
    exceptionCaught = true;
  }
  assert(exceptionCaught);

  std::cout << "All union k-th smallest tests passed successfully!\n";
}

int main() {
  runTests();
  return 0;
}