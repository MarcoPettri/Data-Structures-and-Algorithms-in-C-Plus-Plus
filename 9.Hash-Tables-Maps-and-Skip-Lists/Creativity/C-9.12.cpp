// File : C-9.12.cpp

/*
    C-9.12 Describe an efficient ordered dictionary structure for storing n
   elements that have an associated set of k < n keys that come from a total
   order. That is, the set of keys is smaller than the number of elements. Your
    structure should perform all the ordered dictionary operations in O(log k +
    s) expected time, where s is the number of elements returned.
*/

#include "distinct_key_multimap.hpp"
#include <cassert>
#include <iostream>
#include <string>

void runTests() {
  std::cout << "Running Distinct Key Multimap tests...\n";

  DistinctKeyMultimap<int, std::string> map;

  assert(map.empty());
  assert(map.size() == 0);
  assert(map.distinctKeysCount() == 0);

  // Insert multiple elements sharing keys (n = 6 elements, k = 3 distinct keys)
  map.insert(20, "Apple");
  map.insert(10, "Banana");
  map.insert(20, "Apricot");
  map.insert(30, "Cherry");
  map.insert(10, "Blueberry");
  map.insert(20, "Avocado");

  assert(map.size() == 6);
  assert(map.distinctKeysCount() == 3);

  // Test findAll (O(log k + s) time)
  auto results20 = map.findAll(20);
  assert(results20.size() == 3);
  assert(results20[0] == "Apple");
  assert(results20[1] == "Apricot");
  assert(results20[2] == "Avocado");

  // Test rangeQuery (O(log k + s) time)
  auto rangeRes = map.rangeQuery(10, 20);
  assert(rangeRes.size() ==
         5); // 2 elements from key 10 + 3 elements from key 20

  // Test remove
  assert(map.remove(10, "Banana"));
  assert(map.size() == 5);
  assert(map.findAll(10).size() == 1);

  // Remove last element of key 10 to check distinct key cleanup
  assert(map.remove(10, "Blueberry"));
  assert(map.distinctKeysCount() == 2); // Key 10 is now completely removed

  std::cout << "All Distinct Key Multimap tests passed successfully!\n";
}

int main() {
  runTests();
  return 0;
}