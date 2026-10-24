// File : R-9.4.cpp

/*
    R-9.4 Describe how to use a skip-list map to implement the dictionary ADT,
    allowing the user to insert different entries with equal keys.
*/

#include "skip_list_multimap.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

void runTests() {
  std::cout << "Running Skip-List Multimap tests...\n";

  SkipListMultimap<int, std::string> map;

  assert(map.empty());
  assert(map.size() == 0);

  // Insert entries with unique and duplicate keys
  map.insert(10, "Apple");
  map.insert(20, "Banana");
  map.insert(10, "Apricot"); // Duplicate key 10
  map.insert(30, "Cherry");
  map.insert(10, "Avocado"); // Another duplicate key 10

  assert(map.size() == 5);

  // Test findAll for duplicate keys
  auto values10 = map.findAll(10);
  assert(values10.size() == 3);

  // Ensure all inserted values for key 10 are present
  std::sort(values10.begin(), values10.end());
  assert(values10[0] == "Apple");
  assert(values10[1] == "Apricot");
  assert(values10[2] == "Avocado");

  // Test single find
  std::string val;
  assert(map.find(20, val));
  assert(val == "Banana");

  // Test non-existent key
  assert(!map.find(99, val));
  assert(map.findAll(99).empty());

  // Test remove duplicates
  size_t removed = map.remove(10);
  assert(removed == 3);
  assert(map.size() == 2);
  assert(map.findAll(10).empty());

  std::cout << "All Skip-List Multimap tests passed successfully!\n";
}

int main() {
  runTests();
  return 0;
}