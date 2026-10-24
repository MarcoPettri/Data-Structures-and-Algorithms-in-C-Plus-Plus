// File : R-9.5.cpp

/*
    R-9.5 Describe how an ordered list implemented as a doubly linked list could
   be used to implement the map ADT.
*/

#include "OrderedListMap.hpp"
#include <cassert>
#include <iostream>
#include <string>

void runTests() {
  std::cout << "Running OrderedListMap unit tests...\n";

  OrderedListMap<int, std::string> map;

  // Initial state checks
  assert(map.empty());
  assert(map.size() == 0);
  assert(!map.min().has_value());
  assert(!map.max().has_value());

  // Insert out-of-order keys to verify automatic ordering
  map.insert(30, "Thirty");
  map.insert(10, "Ten");
  map.insert(20, "Twenty");
  map.insert(40, "Forty");

  assert(!map.empty());
  assert(map.size() == 4);

  // Verify min and max O(1) bounds
  assert(map.min()->first == 10 && map.min()->second == "Ten");
  assert(map.max()->first == 40 && map.max()->second == "Forty");

  // Test search / find
  auto found20 = map.find(20);
  assert(found20.has_value());
  assert(*found20 == "Twenty");

  auto foundMissing = map.find(25); // Early termination check
  assert(!foundMissing.has_value());

  // Test update existing key
  map.insert(20, "Twenty-Updated");
  assert(map.find(20) == "Twenty-Updated");
  assert(map.size() == 4); // Size should remain unchanged

  // Test erase
  assert(map.erase(10)); // Erase min element
  assert(map.size() == 3);
  assert(map.min()->first == 20); // New min should be 20
  assert(!map.find(10).has_value());

  assert(!map.erase(99)); // Erase non-existent element should return false

  // Clear map
  map.clear();
  assert(map.empty());
  assert(map.size() == 0);

  std::cout << "All OrderedListMap unit tests passed successfully!\n";
}

int main() {
  runTests();
  return 0;
}