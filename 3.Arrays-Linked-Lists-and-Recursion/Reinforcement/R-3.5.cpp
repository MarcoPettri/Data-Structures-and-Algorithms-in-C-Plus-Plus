// File: R-3.5.cpp
/*
    R-3.5 Give a recursive definition of a singly linked list.


*/
#include <iostream>
#include <memory>

struct Node {
  int data;
  std::unique_ptr<Node> next;
};

void print_list(const std::unique_ptr<Node> &head) {
  if (!head) {
    return;
  }
  std::cout << head->data << " ";
  print_list(head->next);
}

int main() {
  auto head = std::make_unique<Node>();
  head->data = 1;
  head->next = std::make_unique<Node>();
  head->next->data = 2;
  head->next->next = std::make_unique<Node>();
  head->next->next->data = 3;

  print_list(head);
  std::cout.put('\n');

  return 0;
}