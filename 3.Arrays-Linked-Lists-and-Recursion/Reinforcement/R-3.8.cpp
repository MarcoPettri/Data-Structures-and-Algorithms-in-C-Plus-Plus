// File: R-3.8.cpp
/*

R-3.8 Give a fully generic implementation of the doubly linked list data
structure of Section 3.3.3 by using a templated class.


*/
#include <iostream>
#include <string>

#include "DoublyLinkedList.hpp"

template <typename T>
void print(const char *label, const DoublyLinkedList<T> &list) {
  std::cout << label << ": ";
  for (const auto &x : list)
    std::cout << x << ' ';
  std::cout << "(size " << list.size() << ")\n";
}

int main() {
  DoublyLinkedList<int> list{1, 2, 3};
  list.push_back(4);
  list.push_front(0);
  print("initial       ", list); // 0 1 2 3 4

  auto it = list.begin();
  ++it;
  ++it; // points to 2
  list.insert(it, 99);
  print("after insert  ", list); // 0 1 99 2 3 4

  it = list.erase(it);           // erases 2, returns iterator to 3
  print("after erase   ", list); // 0 1 99 3 4
  std::cout << "element after erase: " << *it << "\n"; // 3

  list.reverse();
  print("after reverse ", list); // 4 3 99 1 0

  std::cout << "backwards      : ";
  for (auto r = list.rbegin(); r != list.rend(); ++r)
    std::cout << *r << ' ';
  std::cout << "\n"; // 0 1 99 3 4

  // Works with any type: here std::string
  DoublyLinkedList<std::string> words;
  words.emplace_back("hello");
  words.push_back("world");
  auto copy = words; // deep copy
  copy.front() = "HELLO";
  print("words         ", words); // hello world
  print("copy          ", copy);  // HELLO world
  std::cout << std::boolalpha << "equal: " << (words == copy) << "\n"; // false

  words.pop_back();
  print("after pop_back", words); // hello
  return 0;
}