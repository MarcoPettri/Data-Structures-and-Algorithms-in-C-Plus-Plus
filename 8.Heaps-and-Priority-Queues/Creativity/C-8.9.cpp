// File : C-8.9.cpp

/*
C-8.9 Assuming the input to the sorting problem is given in an array A, describe
how to implement the heap-sort algorithm using only the array A and, at
most, six additional (base-type) variables
*/

#include "heap_sort.hpp"
#include <cassert>
#include <iostream>
#include <vector>

void runTests() {
  std::cout << "Running heap sort tests...\n";

  // Test 1: Empty container
  {
    std::vector<int> vec;
    heapSort(vec);
    assert(vec.empty());
  }

  // Test 2: Single element container
  {
    std::vector<int> vec = {42};
    heapSort(vec);
    assert(vec == std::vector<int>{42});
  }

  // Test 3: Already sorted container
  {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    heapSort(vec);
    assert(vec == (std::vector<int>{1, 2, 3, 4, 5}));
  }

  // Test 4: Reverse sorted container
  {
    std::vector<int> vec = {5, 4, 3, 2, 1};
    heapSort(vec);
    assert(vec == (std::vector<int>{1, 2, 3, 4, 5}));
  }

  // Test 5: Unsorted container with duplicates
  {
    std::vector<int> vec = {12, 11, 13, 5, 6, 7, 11};
    heapSort(vec);
    assert(vec == (std::vector<int>{5, 6, 7, 11, 11, 12, 13}));
  }

  std::cout << "All heap sort tests passed successfully!\n";
}

int main() {
  runTests();
  return 0;
}