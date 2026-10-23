// File : C-8.14.cpp

/*
C-8.14 Given a heap T and a key k, give an algorithm to compute all the entries
in T with a key less than or equal to k. For example, given the heap of Fig-
ure 8.12(a) and query k = 7, the algorithm should report the entries with
keys 2, 4, 5, 6, and 7 (but not necessarily in this order). Your algorithm
should run in time proportional to the number of entries returned.
*/

#include "heap_range_search_entries.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

void runTests() {
  std::cout << "Running heap entry range search tests...\n";

  // Constructing a min-heap of entries (key, value)
  //             (2, "A")
  //          /           \
  //      (5, "B")      (4, "C")
  //      /     \       /     \
  //  (12, "D") (10,"E") (7,"F") (9,"G")
  std::vector<HeapEntry<int, std::string>> minHeap = {
      {2, "A"}, {5, "B"}, {4, "C"}, {12, "D"}, {10, "E"}, {7, "F"}, {9, "G"}};

  // Test 1: Query threshold k = 6 (Matching entries: {2, "A"}, {4, "C"}, {5,
  // "B"})
  {
    auto res = findEntriesLessThanOrEqual(minHeap, 6);
    assert(res.size() == 3);

    // Sort results by key for stable comparison since order isn't guaranteed
    // sorted
    std::sort(res.begin(), res.end(),
              [](const auto &a, const auto &b) { return a.key < b.key; });

    assert(res[0].key == 2 && res[0].value == "A");
    assert(res[1].key == 4 && res[1].value == "C");
    assert(res[2].key == 5 && res[2].value == "B");
  }

  // Test 2: Query threshold k = 1 (No entries match)
  {
    auto res = findEntriesLessThanOrEqual(minHeap, 1);
    assert(res.empty());
  }

  // Test 3: Query threshold k = 10 (Matching entries: 2, 4, 5, 7, 9, 10)
  {
    auto res = findEntriesLessThanOrEqual(minHeap, 10);
    assert(res.size() == 6);
  }

  std::cout << "All heap entry range search tests passed successfully!\n";
}

int main() {
  runTests();
  return 0;
}