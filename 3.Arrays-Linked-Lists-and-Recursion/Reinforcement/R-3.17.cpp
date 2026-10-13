// File: R-3.17.cpp
/*
    R-3.17 Write a short C++ function to count the number of nodes in a
    circularly linked list.

    Answer:
      Algorithm:
        1. If the list is empty (i.e. the reference/cursor pointer is nullptr),
           return 0.
        2. Otherwise, initialize a counter to 0 and set a traversal pointer
   `curr` to the starting node (`cursor`).
        3. Traverse the list following each node's `next` pointer using a
   do-while loop: do {
                 ++count;
                 curr = curr->next;
             } while (curr != cursor);
        4. Stop as soon as `curr` cycles back to `cursor`, and return `count`.

      Running time : O(n) — visits every node in the circle exactly once.
      Space usage  : O(1) — requires only two pointers and a counter.
*/

#include "CircularlyLinkedList.hpp"

#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>

// =====================================================================
// 1. Raw Node Representation (classic approach)
// =====================================================================
template <typename T> struct CNode {
  T data;
  CNode *next = nullptr;

  explicit CNode(const T &val, CNode *nxt = nullptr)
      : data(val), next(nxt ? nxt : this) {}
};

// Counts the number of nodes in a raw circularly linked list given a pointer
// to any node in the list (`cursor`).
template <typename Node> std::size_t countNodes(const Node *cursor) {
  if (cursor == nullptr) {
    return 0;
  }

  std::size_t count = 0;
  const Node *curr = cursor;
  do {
    ++count;
    curr = curr->next;
  } while (curr != cursor);

  return count;
}

// =====================================================================
// 2. Container Representation (using CircularlyLinkedList<T> from R-3.15)
// =====================================================================
// Counts the nodes by traversing the circular list links (via iterators)
// rather than querying the cached size_ member.
template <typename T>
std::size_t countNodes(const CircularlyLinkedList<T> &list) {
  if (list.empty()) {
    return 0;
  }

  std::size_t count = 0;
  for (auto it = list.begin(); it != list.end(); ++it) {
    ++count;
  }
  return count;
}

// =====================================================================
// Tests & Verification
// =====================================================================
int main() {
  std::cout << "=== R-3.17: Counting nodes in a circularly linked list ===\n\n";

  // -----------------------------------------------------------------
  // Section 1: Raw CNode tests
  // -----------------------------------------------------------------
  std::cout << "-- Testing raw node-based circularly linked list --\n";

  // Test 1.1: Empty list
  {
    const CNode<int> *emptyList = nullptr;
    std::size_t count = countNodes(emptyList);
    assert(count == 0);
    std::cout << "Test 1.1 Passed: Empty raw list -> count = " << count << "\n";
  }

  // Test 1.2: Single-node circular list (node points to itself)
  {
    auto *single = new CNode<int>(42);
    single->next = single;
    std::size_t count = countNodes(single);
    assert(count == 1);
    std::cout << "Test 1.2 Passed: Single node list {42} -> count = " << count
              << "\n";
    delete single;
  }

  // Test 1.3: Two-node circular list
  {
    auto *n1 = new CNode<int>(10);
    auto *n2 = new CNode<int>(20);
    n1->next = n2;
    n2->next = n1;

    std::size_t countFromN1 = countNodes(n1);
    std::size_t countFromN2 = countNodes(n2);
    assert(countFromN1 == 2);
    assert(countFromN2 == 2);
    std::cout << "Test 1.3 Passed: 2-node list {10, 20} -> count = "
              << countFromN1 << "\n";

    delete n1;
    delete n2;
  }

  // Test 1.4: Multi-node circular list (5 nodes) started from different cursors
  {
    constexpr int N = 5;
    CNode<int> *nodes[N];
    for (int i = 0; i < N; ++i) {
      nodes[i] = new CNode<int>(i + 1);
    }
    for (int i = 0; i < N; ++i) {
      nodes[i]->next = nodes[(i + 1) % N];
    }

    // Counting should yield N regardless of which node is passed as cursor
    for (int i = 0; i < N; ++i) {
      assert(countNodes(nodes[i]) == N);
    }
    std::cout << "Test 1.4 Passed: 5-node circular list counted correctly from "
                 "every node.\n";

    for (int i = 0; i < N; ++i) {
      delete nodes[i];
    }
  }

  // -----------------------------------------------------------------
  // Section 2: CircularlyLinkedList<T> container tests
  // -----------------------------------------------------------------
  std::cout << "\n-- Testing CircularlyLinkedList<T> container --\n";

  // Test 2.1: Empty container
  {
    CircularlyLinkedList<int> list;
    std::size_t count = countNodes(list);
    assert(count == 0);
    assert(count == list.size());
    std::cout << "Test 2.1 Passed: Empty CircularlyLinkedList -> count = "
              << count << "\n";
  }

  // Test 2.2: Single element container
  {
    CircularlyLinkedList<std::string> list{"hello"};
    std::size_t count = countNodes(list);
    assert(count == 1);
    assert(count == list.size());
    std::cout << "Test 2.2 Passed: Single element {\"hello\"} -> count = "
              << count << "\n";
  }

  // Test 2.3: Multi-element container {1, 2, 3, 4, 5, 6, 7}
  {
    CircularlyLinkedList<int> list{1, 2, 3, 4, 5, 6, 7};
    std::size_t count = countNodes(list);
    assert(count == 7);
    assert(count == list.size());
    std::cout << "Test 2.3 Passed: 7-element CircularlyLinkedList -> count = "
              << count << "\n";

    // After rotation, node count must remain invariant
    list.rotate();
    assert(countNodes(list) == 7);
    std::cout << "Test 2.3 Passed: After rotate() -> count remains = "
              << countNodes(list) << "\n";

    // After pop_front, node count decreases by 1
    list.pop_front();
    assert(countNodes(list) == 6);
    std::cout << "Test 2.3 Passed: After pop_front() -> count = "
              << countNodes(list) << "\n";
  }

  std::cout << "\nAll R-3.17 tests passed successfully!\n";
  return 0;
}