// File: C-3.9.cpp
/*
    C-3.9 Give a fast algorithm for concatenating two doubly linked lists L
    and M, with header and trailer sentinel nodes, into a single list L'.

    Answer:
      Splice M's real nodes between L's last real node and L's trailer by
      rewiring four pointers. Both sentinels of L are kept, so the same code
      handles empty lists with no special cases.

      Method: O(1) splice
          1. Let lastL  = L.trailer->prev  (equals L.header if L is empty),
                 firstM = M.header->next,
                 lastM  = M.trailer->prev.
          2. If M is empty (firstM == M.trailer), there is nothing to do.
          3. Otherwise rewire four links:
                 lastL->next          = firstM
                 firstM->prev         = lastL
                 lastM->next          = L.trailer
                 L.trailer->prev      = lastM
          4. Set L.size += M.size.
          5. Reset M to a valid empty list (M.header->next = M.trailer,
             M.trailer->prev = M.header, M.size = 0). M's sentinels are not
             part of L', so they stay with M.
        - Running time : O(1), independent of the lengths of L and M
        - Space usage  : O(1), no nodes allocated, copied or traversed

*/

#include <cassert>
#include <cstddef>
#include <iostream>
#include <vector>

// =====================================================================
// List definition (header and trailer sentinels)
// =====================================================================
template <typename T> struct Node {
  T data;
  Node *prev;
  Node *next;
  explicit Node(T d = T(), Node *p = nullptr, Node *n = nullptr)
      : data(d), prev(p), next(n) {}
};

template <typename T> struct DList {
  Node<T> *header;  // sentinel before the first real node
  Node<T> *trailer; // sentinel after the last real node
  std::size_t size;

  DList() : header(new Node<T>()), trailer(new Node<T>()), size(0) {
    header->next = trailer;
    trailer->prev = header;
  }
  DList(const DList &) = delete;
  DList &operator=(const DList &) = delete;
  ~DList() {
    for (Node<T> *p = header; p != nullptr;) {
      Node<T> *next = p->next;
      delete p;
      p = next;
    }
  }

  void pushBack(T v) {
    Node<T> *last = trailer->prev;
    Node<T> *n = new Node<T>(v, last, trailer);
    last->next = n;
    trailer->prev = n;
    ++size;
  }
};

// =====================================================================
// The algorithm: O(1) concatenation
// =====================================================================
// After the call, L holds all nodes of L followed by all nodes of M, and M
// is a valid empty list. L and M must be distinct lists.
template <typename T> void concatenate(DList<T> &L, DList<T> &M) {
  assert(&L != &M);
  Node<T> *firstM = M.header->next;
  if (firstM == M.trailer) {
    return; // M is empty: L is already L'
  }
  Node<T> *lastL = L.trailer->prev; // L.header when L is empty
  Node<T> *lastM = M.trailer->prev;

  lastL->next = firstM;
  firstM->prev = lastL;
  lastM->next = L.trailer;
  L.trailer->prev = lastM;
  L.size += M.size;

  M.header->next = M.trailer; // M keeps only its own sentinels
  M.trailer->prev = M.header;
  M.size = 0;
}

// =====================================================================
// Helpers
// =====================================================================
namespace {
std::vector<int> forward(const DList<int> &L) {
  std::vector<int> v;
  for (const Node<int> *p = L.header->next; p != L.trailer; p = p->next) {
    v.push_back(p->data);
  }
  return v;
}

std::vector<int> backward(const DList<int> &L) {
  std::vector<int> v;
  for (const Node<int> *p = L.trailer->prev; p != L.header; p = p->prev) {
    v.push_back(p->data);
  }
  return v;
}

std::vector<Node<int> *> nodes(const DList<int> &L) {
  std::vector<Node<int> *> v;
  for (Node<int> *p = L.header->next; p != L.trailer; p = p->next) {
    v.push_back(p);
  }
  return v;
}

void fill(DList<int> &L, const std::vector<int> &v) {
  for (int x : v) {
    L.pushBack(x);
  }
}

bool isEmpty(const DList<int> &L) {
  return L.header->next == L.trailer && L.trailer->prev == L.header &&
         L.size == 0;
}

// Concatenates M onto L and verifies contents in both directions, node
// identity (no copying), size, and that M is left as a valid empty list.
void check(const std::vector<int> &a, const std::vector<int> &b) {
  DList<int> L, M;
  fill(L, a);
  fill(M, b);

  std::vector<Node<int> *> expectedNodes = nodes(L);
  const std::vector<Node<int> *> mNodes = nodes(M);
  expectedNodes.insert(expectedNodes.end(), mNodes.begin(), mNodes.end());

  std::vector<int> expected = a;
  expected.insert(expected.end(), b.begin(), b.end());
  const std::vector<int> expectedBack(expected.rbegin(), expected.rend());

  concatenate(L, M);

  assert(forward(L) == expected);
  assert(backward(L) == expectedBack); // prev links are all consistent
  assert(nodes(L) == expectedNodes);   // same node objects, none copied
  assert(L.size == expected.size());
  assert(isEmpty(M));

  // Both lists must remain fully usable afterwards.
  L.pushBack(-7);
  assert(forward(L).back() == -7 && backward(L).front() == -7);
  assert(L.size == expected.size() + 1);
  M.pushBack(99);
  assert(forward(M) == std::vector<int>{99} && backward(M) == forward(M));
  assert(M.size == 1);
}
} // namespace

// =====================================================================
// Tests & Verification
// =====================================================================
int main() {
  std::cout << "=== C-3.9: Concatenating Two Doubly Linked Lists ===\n\n";

  // Test 1: Both lists empty
  check({}, {});
  std::cout << "Test 1 Passed: empty + empty = empty\n";

  // Test 2: L empty
  check({}, {1, 2, 3});
  std::cout << "Test 2 Passed: empty + [1,2,3] = [1,2,3]\n";

  // Test 3: M empty
  check({1, 2, 3}, {});
  std::cout << "Test 3 Passed: [1,2,3] + empty = [1,2,3]\n";

  // Test 4: Single nodes
  check({1}, {2});
  std::cout << "Test 4 Passed: [1] + [2] = [1,2]\n";

  // Test 5: Standard case
  check({1, 2, 3}, {4, 5, 6, 7});
  std::cout << "Test 5 Passed: [1,2,3] + [4,5,6,7] = [1..7]\n";

  // Test 6: Duplicates and negatives
  check({5, 5, -1}, {-1, 5, 0});
  std::cout << "Test 6 Passed: duplicates and negatives\n";

  // Test 7: Chained concatenation
  {
    DList<int> L, M, N;
    fill(L, {1, 2});
    fill(M, {3});
    fill(N, {4, 5});
    concatenate(L, M);
    concatenate(L, N);
    assert((forward(L) == std::vector<int>{1, 2, 3, 4, 5}));
    assert((backward(L) == std::vector<int>{5, 4, 3, 2, 1}));
    assert(L.size == 5 && isEmpty(M) && isEmpty(N));
    std::cout << "Test 7 Passed: L + M, then + N, gives [1..5]\n";
  }

  // Test 8: Large lists (no recursion, so depth is not a concern)
  {
    std::vector<int> a(200000), b(300000);
    for (int i = 0; i < 200000; ++i)
      a[i] = i;
    for (int i = 0; i < 300000; ++i)
      b[i] = 200000 + i;
    check(a, b);
    std::cout << "Test 8 Passed: 200,000 + 300,000 nodes\n";
  }

  std::cout << "\nAll C-3.9 tests passed successfully!\n";
  return 0;
}