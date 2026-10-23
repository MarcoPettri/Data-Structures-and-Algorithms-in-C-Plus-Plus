// File : C-8.20.cpp

/*
    C-8.20 Let T be a heap storing n keys. Give an efficient algorithm for
   reporting all the keys in T that are smaller than or equal to a given query
   key x (which is not necessarily in T ). For example, given the heap of
   Figure 8.3 and query key x = 7, the algorithm should report 4, 5, 6, 7. Note
   that the keys do not need to be reported in sorted order. Ideally, your
   algorithm should run in O(k) time, where k is the number of keys reported.
*/

#include "heap_range_search.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

void runTests() {
  std::cout << "Running heap range search tests...\n";

  // Min-heap representation:
  //             2
  //          /     \
  //         5       4
  //        / \     / \
  //       12 10   7   9
  //      /  \
  //     15  20
  std::vector<int> minHeap = {2, 5, 4, 12, 10, 7, 9, 15, 20};

  // Test 1: Threshold x = 6 (Keys <= 6: 2, 5, 4)
  {
    auto res = reportKeysLessThanOrEqual(minHeap, 6);
    assert(res.size() == 3);
    std::sort(res.begin(), res.end());
    assert(res == (std::vector<int>{2, 4, 5}));
  }

  // Test 2: Threshold x = 1 (No keys <= 1)
  {
    auto res = reportKeysLessThanOrEqual(minHeap, 1);
    assert(res.empty());
  }

  // Test 3: Threshold x = 10 (Keys <= 10: 2, 5, 4, 10, 7, 9)
  {
    auto res = reportKeysLessThanOrEqual(minHeap, 10);
    assert(res.size() == 6);
    std::sort(res.begin(), res.end());
    assert(res == (std::vector<int>{2, 4, 5, 7, 9, 10}));
  }

  // Test 4: Threshold x = 25 (All keys reported)
  {
    auto res = reportKeysLessThanOrEqual(minHeap, 25);
    assert(res.size() == minHeap.size());
  }

  std::cout << "All heap range search tests passed successfully!\n";
}

int main() {
  runTests();
  return 0;
}