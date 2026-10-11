// File: R-3.7.cpp
/*

R-3.7 Give an algorithm for finding the penultimate (second to last) node in a
singly linked list where the last element is indicated by a null next link.

*/

#include <iostream>
#include <memory>

struct Node {
  int data;
  std::unique_ptr<Node> next;
  explicit Node(int d) : data(d), next(nullptr) {}
};

// Returns a non-owning pointer to the penultimate node, or nullptr if none
// exists.
Node *penultimate(const std::unique_ptr<Node> &head) {
  if (!head || !head->next)
    return nullptr;

  Node *current = head.get();
  while (current->next->next) // stop when next is the last node
    current = current->next.get();

  return current;
}

int main() {
  // Build 3 -> 7 -> 9
  auto head = std::make_unique<Node>(3);
  head->next = std::make_unique<Node>(7);
  head->next->next = std::make_unique<Node>(9);

  if (Node *p = penultimate(head))
    std::cout << "Penultimate: " << p->data << "\n"; // 7
  else
    std::cout << "No penultimate node\n";
}