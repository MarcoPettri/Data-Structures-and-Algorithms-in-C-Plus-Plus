// File : C-8.7.cpp

/*
    C-8.7 Describe an in-place version of the selection-sort algorithm that uses
   only O(1) space for member variables in addition to an input array itself.
*/

#include "selection_sort.hpp"
#include <cassert>
#include <iostream>
#include <vector>

void runTests() {
  std::cout << "Running selection sort tests...\n";

  // Test 1: Empty container
  {
    std::vector<int> vec;
    selectionSort(vec);
    assert(vec.empty());
  }

  // Test 2: Single element container
  {
    std::vector<int> vec = {42};
    selectionSort(vec);
    assert(vec == std::vector<int>{42});
  }

  // Test 3: Already sorted container
  {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    selectionSort(vec);
    assert(vec == (std::vector<int>{1, 2, 3, 4, 5}));
  }

  // Test 4: Reverse sorted container
  {
    std::vector<int> vec = {5, 4, 3, 2, 1};
    selectionSort(vec);
    assert(vec == (std::vector<int>{1, 2, 3, 4, 5}));
  }

  // Test 5: Unsorted container with duplicates
  {
    std::vector<int> vec = {3, 1, 4, 1, 5, 9, 2, 6};
    selectionSort(vec);
    assert(vec == (std::vector<int>{1, 1, 2, 3, 4, 5, 6, 9}));
  }

  std::cout << "All selection sort tests passed successfully!\n";
}

int main() {
  runTests();
  return 0;
}