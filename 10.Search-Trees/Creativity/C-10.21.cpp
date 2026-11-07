// File : C-10.21.cpp

/*
    C-10.21 The mergeable heap ADT consists of operations insert(k, x),
   removeMin(), unionWith(h), and min(), where the unionWith(h) operation
   performs a union of the mergeable heap h with the present one, destroying the
   old versions of both. Describe a concrete implementation of the mergeable
    heap ADT that achieves O(log n) performance for all its operations.
*/

#include "leftist_heap.hpp"
#include <cassert>
#include <iostream>
#include <string>

void runTests() {
  std::cout << "Running Leftist Heap tests...\n";

  LeftistHeap<int, std::string> h1;
  assert(h1.empty());
  assert(h1.size() == 0);

  // Test 1: Insertions and Min
  h1.insert(20, "Twenty");
  h1.insert(10, "Ten");
  h1.insert(30, "Thirty");

  assert(h1.size() == 3);
  assert(h1.min().key == 10 && h1.min().value == "Ten");

  // Test 2: Remove Min
  h1.removeMin();
  assert(h1.size() == 2);
  assert(h1.min().key == 20);

  // Test 3: UnionWith operation
  LeftistHeap<int, std::string> h2;
  h2.insert(5, "Five");
  h2.insert(15, "Fifteen");

  // Union h2 into h1 (h2 gets consumed/destroyed)
  h1.unionWith(std::move(h2));

  assert(h1.size() == 4); // 2 from h1 + 2 from h2
  assert(h1.min().key == 5 && h1.min().value == "Five"); // New minimum is 5

  // Extract all in sorted order
  assert(h1.min().key == 5);
  h1.removeMin();
  assert(h1.min().key == 15);
  h1.removeMin();
  assert(h1.min().key == 20);
  h1.removeMin();
  assert(h1.min().key == 30);
  h1.removeMin();

  assert(h1.empty());

  std::cout << "All Leftist Heap tests passed successfully!\n";
}

int main() {
  runTests();
  return 0;
}