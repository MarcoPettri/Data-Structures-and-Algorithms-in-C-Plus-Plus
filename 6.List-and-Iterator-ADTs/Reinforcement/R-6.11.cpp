// File : R-6.11.cpp

/*
    R-6.11 Provide a fully generic version of the class NodeList of Code
   Fragment 6.9 using a templated class.
*/

/**
 * @file main.cpp
 * @brief Unit tests for the generic NodeList container.
 */
#include "NodeList.hpp"
#include <cassert>
#include <iostream>
#include <string>

void test_basic_operations() {
  std::cout << "Running test_basic_operations...\n";
  datastructures::NodeList<int> list;

  assert(list.empty());
  assert(list.size() == 0);

  list.push_back(10);
  list.push_back(20);
  list.push_front(5);

  assert(!list.empty());
  assert(list.size() == 3);
  assert(list.front() == 5);
  assert(list.back() == 20);

  std::cout << "test_basic_operations passed!\n";
}

void test_pop_operations() {
  std::cout << "Running test_pop_operations...\n";
  datastructures::NodeList<std::string> list;

  list.push_back("Apple");
  list.push_back("Banana");
  list.push_back("Cherry");

  assert(list.size() == 3);

  list.pop_front();
  assert(list.front() == "Banana");
  assert(list.size() == 2);

  list.pop_back();
  assert(list.back() == "Banana");
  assert(list.size() == 1);

  list.pop_back();
  assert(list.empty());

  std::cout << "test_pop_operations passed!\n";
}

void test_iterators() {
  std::cout << "Running test_iterators...\n";
  datastructures::NodeList<int> list;

  for (int i = 1; i <= 5; ++i) {
    list.push_back(i * 10);
  }

  int expected = 10;
  for (auto it = list.begin(); it != list.end(); ++it) {
    assert(*it == expected);
    expected += 10;
  }

  std::cout << "test_iterators passed!\n";
}

int main() {
  try {
    test_basic_operations();
    test_pop_operations();
    test_iterators();

    std::cout << "\nAll NodeList tests completed successfully!\n";
  } catch (const std::exception &e) {
    std::cerr << "Test failed with exception: " << e.what() << "\n";
    return 1;
  }
  return 0;
}