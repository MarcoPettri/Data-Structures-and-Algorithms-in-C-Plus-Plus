// File: C-3.10.cpp
/*
    C-3.10 Describe in detail how to swap two nodes x and y (and not just
    their contents) in a singly linked list L given references only to x and
    y. Repeat this exercise for the case when L is a doubly linked list.
    Which algorithm takes more time?

    Answer:
      Swapping nodes means relinking pointers, so every node that points to x
      or y (and everything x and y point to) must be updated. Both lists have
      sentinels, so no null checks are needed at the ends.

      Singly linked list (header sentinel, tail pointer)
          A node has no link to its predecessor, and we only hold x and y, so
          the predecessors px and py must be found by walking from the header.
          1. If x == y, do nothing.
          2. Walk from L.header until both px (px->next == x) and py
             (py->next == y) are found.
          3. If L.tail is x or y, exchange it (positions are being swapped).
          4. If y immediately precedes x, exchange the names x and y (and px
             and py) so that x is the earlier node.
          5. Adjacent case (x->next == y):
                 px->next = y;  x->next = y->next;  y->next = x;
          6. Non-adjacent case:
                 px->next = y;  py->next = x;  swap(x->next, y->next);
          The adjacent case needs its own code: the non-adjacent formulas
          would make x->next point to x itself.
        - Running time : O(n), dominated by finding px and py
        - Space usage  : O(1)

      Doubly linked list (header and trailer sentinels)
          Every node knows its predecessor and successor, so no search is
          needed.
          1. If x == y, do nothing.
          2. If y immediately precedes x, exchange the names x and y.
          3. Adjacent case (x->next == y), with px = x->prev, ny = y->next:
                 px->next = y;  y->prev = px;
                 y->next  = x;  x->prev = y;
                 x->next  = ny; ny->prev = x;
          4. Non-adjacent case, with px, nx, py, ny the neighbors of x and y:
                 px->next = y;  y->prev = px;  y->next = nx;  nx->prev = y;
                 py->next = x;  x->prev = py;  x->next = ny;  ny->prev = x;
        - Running time : O(1), a fixed number of pointer updates
        - Space usage  : O(1)

      Which takes more time? The singly linked version: O(n) versus O(1).
      The extra cost is entirely the search for the predecessors, which a
      doubly linked list stores explicitly. (If a singly linked list were
      also given px and py, its swap would be O(1) as well.)
*/

#include <cassert>
#include <cstddef>
#include <iostream>
#include <utility>
#include <vector>

// =====================================================================
// Singly linked list (header sentinel + tail pointer)
// =====================================================================
template <typename T> struct SNode {
  T data;
  SNode *next;
  explicit SNode(T d = T(), SNode *n = nullptr) : data(d), next(n) {}
};

template <typename T> struct SList {
  SNode<T> *header;
  SNode<T> *tail; // last real node, or header when empty
  std::size_t size;

  SList() : header(new SNode<T>()), tail(header), size(0) {}
  SList(const SList &) = delete;
  SList &operator=(const SList &) = delete;
  ~SList() {
    for (SNode<T> *p = header; p != nullptr;) {
      SNode<T> *next = p->next;
      delete p;
      p = next;
    }
  }

  SNode<T> *pushBack(T v) {
    tail->next = new SNode<T>(v);
    tail = tail->next;
    ++size;
    return tail;
  }
};

// O(n): the walk to find the predecessors dominates.
template <typename T> void swapNodes(SList<T> &L, SNode<T> *x, SNode<T> *y) {
  if (x == y) {
    return;
  }
  SNode<T> *px = nullptr, *py = nullptr;
  for (SNode<T> *p = L.header; p != nullptr && (px == nullptr || py == nullptr);
       p = p->next) {
    if (p->next == x)
      px = p;
    if (p->next == y)
      py = p;
  }
  assert(px != nullptr && py != nullptr); // both nodes must be in L

  if (L.tail == x) {
    L.tail = y;
  } else if (L.tail == y) {
    L.tail = x;
  }

  if (y->next == x) { // make x the earlier node
    std::swap(x, y);
    std::swap(px, py);
  }
  if (x->next == y) { // adjacent: px -> x -> y  becomes  px -> y -> x
    px->next = y;
    x->next = y->next;
    y->next = x;
  } else { // non-adjacent
    px->next = y;
    py->next = x;
    std::swap(x->next, y->next);
  }
}

// =====================================================================
// Doubly linked list (header and trailer sentinels)
// =====================================================================
template <typename T> struct DNode {
  T data;
  DNode *prev;
  DNode *next;
  explicit DNode(T d = T(), DNode *p = nullptr, DNode *n = nullptr)
      : data(d), prev(p), next(n) {}
};

template <typename T> struct DList {
  DNode<T> *header;
  DNode<T> *trailer;
  std::size_t size;

  DList() : header(new DNode<T>()), trailer(new DNode<T>()), size(0) {
    header->next = trailer;
    trailer->prev = header;
  }
  DList(const DList &) = delete;
  DList &operator=(const DList &) = delete;
  ~DList() {
    for (DNode<T> *p = header; p != nullptr;) {
      DNode<T> *next = p->next;
      delete p;
      p = next;
    }
  }

  DNode<T> *pushBack(T v) {
    DNode<T> *last = trailer->prev;
    DNode<T> *n = new DNode<T>(v, last, trailer);
    last->next = n;
    trailer->prev = n;
    ++size;
    return n;
  }
};

// O(1): neighbors are reached directly through prev/next.
template <typename T> void swapNodes(DList<T> &, DNode<T> *x, DNode<T> *y) {
  if (x == y) {
    return;
  }
  if (y->next == x) { // make x the earlier node
    std::swap(x, y);
  }
  DNode<T> *px = x->prev;
  DNode<T> *ny = y->next;
  if (x->next == y) { // adjacent: px <-> x <-> y <-> ny
    px->next = y;
    y->prev = px;
    y->next = x;
    x->prev = y;
    x->next = ny;
    ny->prev = x;
  } else { // non-adjacent
    DNode<T> *nx = x->next;
    DNode<T> *py = y->prev;
    px->next = y;
    y->prev = px;
    y->next = nx;
    nx->prev = y;
    py->next = x;
    x->prev = py;
    x->next = ny;
    ny->prev = x;
  }
}

// =====================================================================
// Helpers
// =====================================================================
namespace {
std::vector<SNode<int> *> nodesForward(const SList<int> &L) {
  std::vector<SNode<int> *> v;
  for (SNode<int> *p = L.header->next; p != nullptr; p = p->next) {
    v.push_back(p);
  }
  return v;
}

std::vector<DNode<int> *> nodesForward(const DList<int> &L) {
  std::vector<DNode<int> *> v;
  for (DNode<int> *p = L.header->next; p != L.trailer; p = p->next) {
    v.push_back(p);
  }
  return v;
}

std::vector<DNode<int> *> nodesBackward(const DList<int> &L) {
  std::vector<DNode<int> *> v;
  for (DNode<int> *p = L.trailer->prev; p != L.header; p = p->prev) {
    v.push_back(p);
  }
  return v;
}

// Builds a list holding 0..n-1, swaps the nodes at positions i and j, and
// verifies the exact node order, that no node's data changed (nodes were
// moved, not their contents), the size, and the structural invariants.
void checkSingly(int n, int i, int j) {
  SList<int> L;
  std::vector<SNode<int> *> ref;
  for (int k = 0; k < n; ++k)
    ref.push_back(L.pushBack(k));

  swapNodes(L, ref[i], ref[j]);
  std::swap(ref[i], ref[j]); // expected order

  assert(nodesForward(L) == ref);
  for (int k = 0; k < n; ++k)
    assert(ref[k]->data == (k == i ? j : k == j ? i : k));
  assert(L.size == static_cast<std::size_t>(n));
  assert(L.tail == (ref.empty() ? L.header : ref.back()));
  assert(L.tail->next == nullptr);

  L.pushBack(100); // the list must remain fully usable
  assert(nodesForward(L).size() == static_cast<std::size_t>(n) + 1);
}

void checkDoubly(int n, int i, int j) {
  DList<int> L;
  std::vector<DNode<int> *> ref;
  for (int k = 0; k < n; ++k)
    ref.push_back(L.pushBack(k));

  swapNodes(L, ref[i], ref[j]);
  std::swap(ref[i], ref[j]); // expected order

  assert(nodesForward(L) == ref);
  assert(nodesBackward(L) ==
         std::vector<DNode<int> *>(ref.rbegin(), ref.rend()));
  for (int k = 0; k < n; ++k)
    assert(ref[k]->data == (k == i ? j : k == j ? i : k));
  assert(L.size == static_cast<std::size_t>(n));

  L.pushBack(100);
  assert(nodesBackward(L).front()->data == 100);
}
} // namespace

// =====================================================================
// Tests & Verification
// =====================================================================
int main() {
  std::cout << "=== C-3.10: Swapping Two Nodes in a Linked List ===\n\n";

  // Test 1: x == y changes nothing
  checkSingly(1, 0, 0);
  checkDoubly(1, 0, 0);
  checkSingly(4, 2, 2);
  checkDoubly(4, 2, 2);
  std::cout << "Test 1 Passed: swapping a node with itself\n";

  // Test 2: Exhaustive. Every ordered pair (i, j) in lists of 1..8 nodes,
  // which covers adjacent (both orders), non-adjacent, first, last, and the
  // tail-pointer update.
  for (int n = 1; n <= 8; ++n) {
    for (int i = 0; i < n; ++i) {
      for (int j = 0; j < n; ++j) {
        checkSingly(n, i, j);
        checkDoubly(n, i, j);
      }
    }
  }
  std::cout << "Test 2 Passed: all (i, j) pairs, lists of 1..8 nodes\n";

  // Test 3: Adjacent nodes in both argument orders
  checkSingly(5, 1, 2);
  checkSingly(5, 2, 1);
  checkDoubly(5, 1, 2);
  checkDoubly(5, 2, 1);
  std::cout << "Test 3 Passed: adjacent nodes, either argument order\n";

  // Test 4: Swapping the first and last nodes of a long list
  checkSingly(1000, 0, 999);
  checkDoubly(1000, 0, 999);
  std::cout << "Test 4 Passed: first <-> last in a 1000-node list\n";

  // Test 5: Swapping twice restores the original order
  {
    SList<int> S;
    std::vector<SNode<int> *> sref;
    for (int k = 0; k < 6; ++k)
      sref.push_back(S.pushBack(k));
    swapNodes(S, sref[1], sref[4]);
    swapNodes(S, sref[1], sref[4]);
    assert(nodesForward(S) == sref);

    DList<int> D;
    std::vector<DNode<int> *> dref;
    for (int k = 0; k < 6; ++k)
      dref.push_back(D.pushBack(k));
    swapNodes(D, dref[1], dref[4]);
    swapNodes(D, dref[1], dref[4]);
    assert(nodesForward(D) == dref);
    std::cout << "Test 5 Passed: swap(x, y) twice restores the list\n";
  }

  std::cout << "\nAll C-3.10 tests passed successfully!\n";
  return 0;
}