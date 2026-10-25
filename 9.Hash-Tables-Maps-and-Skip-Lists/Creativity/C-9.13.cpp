// File : C-9.13.cpp

/*
    C-9.13 Describe an efficient dictionary structure for storing n entries
   whose r < n keys have distinct hash codes. Your structure should perform
   operation findAll in O(1 + s) expected time, where s is the number of entries
   returned, and the remaining operations of the dictionary ADT in O(1) ex-
    pected time.
*/

#include "distinct_hash_multimap.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

void runTests() {
  std::cout << "Running Distinct Hash Multimap tests...\n";

  DistinctHashMultimap<int, std::string> map(4);

  assert(map.empty());
  assert(map.size() == 0);
  assert(map.distinctKeysCount() == 0);

  // Insert entries (multiple entries sharing keys)
  map.insert(100, "Alpha");
  map.insert(200, "Beta");
  map.insert(100, "Gamma"); // Duplicate key 100
  map.insert(300, "Delta");
  map.insert(100, "Epsilon"); // Another duplicate key 100

  assert(map.size() == 5);
  assert(map.distinctKeysCount() == 3);

  // Test findAll (O(1 + s) expected time)
  auto results100 = map.findAll(100);
  assert(results100.size() == 3);

  std::sort(results100.begin(), results100.end());
  assert(results100[0] == "Alpha");
  assert(results100[1] == "Epsilon");
  assert(results100[2] == "Gamma");

  // Test findAll for non-existent key
  auto results999 = map.findAll(999);
  assert(results999.empty());

  // Test remove
  size_t removed = map.remove(100);
  assert(removed == 3);
  assert(map.size() == 2);
  assert(map.distinctKeysCount() == 2);
  assert(map.findAll(100).empty());

  std::cout << "All Distinct Hash Multimap tests passed successfully!\n";
}

int main() {
  runTests();
  return 0;
}