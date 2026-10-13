// File: C-3.7.cpp
/*
    C-3.7 Describe a fast recursive algorithm for reversing a singly linked
    list L, so that the ordering of the nodes becomes opposite of what it was
    before.

    Answer:
      Reverse the sublist after the head, then attach the head to the end of
      the reversed sublist.

      Method 1: Reverse the tail, then fix the links
          1. Base case: an empty list or a one-node list is already reversed.
          2. Recursively reverse the list starting at head->next. The call
             returns the new head (the old tail).
          3. The old head->next is now the LAST node of the reversed sublist,
             so set head->next->next = head, then head->next = nullptr.
          4. Return the new head from step 2 unchanged.
        - Running time : O(n), one O(1) rewiring per node
        - Space usage  : O(n) for the call stack (depth n)

      Method 2: Tail-recursive with an accumulator
        Carry the already-reversed prefix in `prev`. At each call, point
        curr->next at prev and recurse on the remainder. This is the
        iterative loop written recursively.
        - Running time : O(n)
        - Space usage  : O(1) with tail-call optimization, otherwise O(n)

      Both methods are in place: no nodes are allocated, only `next` pointers
      are rewired. Time is optimal, since every node's pointer must change.
*/

#include <cassert>
#include <iostream>
#include <vector>

// =====================================================================
// Node definition
// =====================================================================
template <typename T> struct Node {
  T data;
  Node *next;
  explicit Node(T d, Node *n = nullptr) : data(d), next(n) {}
};

// =====================================================================
// Method 1: Reverse the tail, then fix the links
// =====================================================================
template <typename T> Node<T> *reverseRecursive(Node<T> *head) {
  if (head == nullptr || head->next == nullptr) {
    return head; // base case: empty or single node
  }
  Node<T> *newHead = reverseRecursive(head->next); // reverse the rest
  head->next->next = head; // old successor now points back at head
  head->next = nullptr;    // head becomes the tail
  return newHead;          // old tail is the new head
}

// =====================================================================
// Method 2: Tail-recursive with an accumulator
// =====================================================================
template <typename T> Node<T> *reverseTail(Node<T> *curr, Node<T> *prev) {
  if (curr == nullptr) {
    return prev;
  }
  Node<T> *next = curr->next;
  curr->next = prev;
  return reverseTail(next, curr);
}

template <typename T> Node<T> *reverseTail(Node<T> *head) {
  return reverseTail(head, static_cast<Node<T> *>(nullptr));
}

// =====================================================================
// Helpers
// =====================================================================
namespace {
Node<int> *build(const std::vector<int> &v) {
  Node<int> *head = nullptr;
  for (auto it = v.rbegin(); it != v.rend(); ++it) {
    head = new Node<int>(*it, head);
  }
  return head;
}

std::vector<int> toVector(const Node<int> *head) {
  std::vector<int> v;
  for (; head != nullptr; head = head->next) {
    v.push_back(head->data);
  }
  return v;
}

std::vector<Node<int> *> nodes(Node<int> *head) {
  std::vector<Node<int> *> v;
  for (; head != nullptr; head = head->next) {
    v.push_back(head);
  }
  return v;
}

void freeList(Node<int> *head) {
  while (head != nullptr) {
    Node<int> *next = head->next;
    delete head;
    head = next;
  }
}

std::vector<int> reversed(std::vector<int> v) {
  return std::vector<int>(v.rbegin(), v.rend());
}

// Runs both methods on a fresh list and verifies the result and that the
// reversal was done in place (same node objects, opposite order).
void check(const std::vector<int> &input) {
  const std::vector<int> expected = reversed(input);

  Node<int> *a = build(input);
  const std::vector<Node<int> *> before = nodes(a);
  a = reverseRecursive(a);
  assert(toVector(a) == expected);
  std::vector<Node<int> *> after = nodes(a);
  assert(std::vector<Node<int> *>(before.rbegin(), before.rend()) == after);
  freeList(a);

  Node<int> *b = build(input);
  b = reverseTail(b);
  assert(toVector(b) == expected);
  freeList(b);
}
} // namespace

// =====================================================================
// Tests & Verification
// =====================================================================
int main() {
  std::cout << "=== C-3.7: Recursive Reversal of a Singly Linked List ===\n\n";

  // Test 1: Empty list
  check({});
  std::cout << "Test 1 Passed: empty list stays empty\n";

  // Test 2: Single node
  check({42});
  std::cout << "Test 2 Passed: single node unchanged\n";

  // Test 3: Two nodes
  check({1, 2});
  std::cout << "Test 3 Passed: 1 -> 2 becomes 2 -> 1\n";

  // Test 4: Standard case
  check({1, 2, 3});
  check({1, 2, 3, 4, 5, 6, 7});
  std::cout
      << "Test 4 Passed: 1 -> 2 -> 3 becomes 3 -> 2 -> 1, and a 7-node list\n";

  // Test 5: Duplicates and negative values
  check({5, 5, -1, 5, 0, -1});
  std::cout << "Test 5 Passed: duplicates and negatives\n";

  // Test 6: Reversing twice restores the original
  {
    const std::vector<int> input = {9, 8, 7, 6, 5};
    Node<int> *l = build(input);
    l = reverseRecursive(reverseRecursive(l));
    assert(toVector(l) == input);
    freeList(l);
    std::cout << "Test 6 Passed: reverse(reverse(L)) == L\n";
  }

  // Test 7: Larger list (kept modest so recursion depth stays safe)
  {
    std::vector<int> big(10000);
    for (int i = 0; i < 10000; ++i) {
      big[i] = i;
    }
    check(big);
    std::cout << "Test 7 Passed: 10,000-node list\n";
  }

  std::cout << "\nAll C-3.5 tests passed successfully!\n";
  return 0;
}