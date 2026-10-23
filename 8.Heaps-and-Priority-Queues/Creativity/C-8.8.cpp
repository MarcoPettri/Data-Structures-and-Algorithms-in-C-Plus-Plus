// File : C-8.8.cpp

/*
    C-8.8 Assuming the input to the sorting problem is given in an array A,
    describe how to implement the insertion-sort algorithm using only the
    array A and, at most, six additional (base-type) variables.
*/

#include "insertion_sort.hpp"
#include <cassert>
#include <iostream>
#include <vector>

void runTests() {
  std::cout << "Running insertion sort tests...\n";

  // Test 1: Empty container
  {
    std::vector<int> vec;
    insertionSort(vec);
    assert(vec.empty());
  }

  // Test 2: Single element container
  {
    std::vector<int> vec = {42};
    insertionSort(vec);
    assert(vec == std::vector<int>{42});
  }

  // Test 3: Already sorted container (Best Case - O(N))
  {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    insertionSort(vec);
    assert(vec == (std::vector<int>{1, 2, 3, 4, 5}));
  }

  // Test 4: Reverse sorted container (Worst Case - O(N^2))
  {
    std::vector<int> vec = {5, 4, 3, 2, 1};
    insertionSort(vec);
    assert(vec == (std::vector<int>{1, 2, 3, 4, 5}));
  }

  // Test 5: Unsorted container with duplicates
  {
    std::vector<int> vec = {12, 11, 13, 5, 6, 11};
    insertionSort(vec);
    assert(vec == (std::vector<int>{5, 6, 11, 11, 12, 13}));
  }

  std::cout << "All insertion sort tests passed successfully!\n";
}

int main() {
  runTests();
  return 0;
}