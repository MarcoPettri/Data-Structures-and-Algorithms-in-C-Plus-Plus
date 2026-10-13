// File: C-3.8.cpp
/*
    C-3.8 Describe a good algorithm for concatenating two singly linked lists
    L and M, with header sentinels, into a single list L' that contains all
    the nodes of L followed by all the nodes of M.

    Answer:
      Splice M's nodes onto the end of L by rewiring a single pointer, using a
      tail pointer to reach L's last node without traversing.

      Method: O(1) splice using a tail pointer
          1. Each list stores its header sentinel and a pointer `tail` to its
             last real node (`tail == header` when the list is empty).
          2. If M is empty, there is nothing to do: L is already L'.
          3. Otherwise set L.tail->next = M.header->next. This links the last
             node of L to the first real node of M.
          4. Set L.tail = M.tail and L.size += M.size.
          5. Reset M to a valid empty list (M.header->next = nullptr,
             M.tail = M.header, M.size = 0). M's header sentinel is not part
             of L', so it simply stays with M.
        - Running time : O(1), independent of the lengths of L and M
        - Space usage  : O(1), no nodes allocated, copied or traversed

      Without a tail pointer, one must walk L from its header to the last node
      first, which costs O(n) for |L| = n. Keeping the tail pointer removes
      that walk, so this is optimal: the constant-time splice is the best
      possible, since the result must at least change one link.
*/

#include <cassert>
#include <cstddef>
#include <iostream>
#include <vector>

// =====================================================================
// List definition (header sentinel + tail pointer)
// =====================================================================
template <typename T> struct Node {
  T data;
  Node *next;
  explicit Node(T d = T(), Node *n = nullptr) : data(d), next(n) {}
};

template <typename T> struct SList {
  Node<T> *header; // sentinel: holds no data, header->next is the first node
  Node<T> *tail;   // last real node, or header when the list is empty
  std::size_t size;

  SList() : header(new Node<T>()), tail(header), size(0) {}
  SList(const SList &) = delete;
  SList &operator=(const SList &) = delete;
  ~SList() {
    for (Node<T> *p = header; p != nullptr;) {
      Node<T> *next = p->next;
      delete p;
      p = next;
    }
  }

  void pushBack(T v) {
    tail->next = new Node<T>(v);
    tail = tail->next;
    ++size;
  }
};

// =====================================================================
// The algorithm: O(1) concatenation
// =====================================================================
// After the call, L holds all nodes of L followed by all nodes of M, and M
// is a valid empty list. L and M must be distinct lists.
template <typename T> void concatenate(SList<T> &L, SList<T> &M) {
  assert(&L != &M);
  if (M.header->next == nullptr) {
    return; // M is empty: L is already L'
  }
  L.tail->next = M.header->next; // last node of L -> first node of M
  L.tail = M.tail;               // L' ends where M ended
  L.size += M.size;

  M.header->next = nullptr; // M keeps only its own sentinel
  M.tail = M.header;
  M.size = 0;
}

// =====================================================================
// Helpers
// =====================================================================
namespace {
std::vector<int> toVector(const SList<int> &L) {
  std::vector<int> v;
  for (const Node<int> *p = L.header->next; p != nullptr; p = p->next) {
    v.push_back(p->data);
  }
  return v;
}

std::vector<Node<int> *> nodes(const SList<int> &L) {
  std::vector<Node<int> *> v;
  for (Node<int> *p = L.header->next; p != nullptr; p = p->next) {
    v.push_back(p);
  }
  return v;
}

void fill(SList<int> &L, const std::vector<int> &v) {
  for (int x : v) {
    L.pushBack(x);
  }
}

// Concatenates M onto L and verifies contents, node identity (no copying),
// size, tail pointer, and that M is left as a valid empty list.
void check(const std::vector<int> &a, const std::vector<int> &b) {
  SList<int> L, M;
  fill(L, a);
  fill(M, b);

  std::vector<Node<int> *> expectedNodes = nodes(L);
  const std::vector<Node<int> *> mNodes = nodes(M);
  expectedNodes.insert(expectedNodes.end(), mNodes.begin(), mNodes.end());

  std::vector<int> expected = a;
  expected.insert(expected.end(), b.begin(), b.end());

  concatenate(L, M);

  assert(toVector(L) == expected);
  assert(nodes(L) == expectedNodes); // same node objects, none copied
  assert(L.size == expected.size());
  assert(L.tail->next == nullptr);
  assert(L.tail == (expectedNodes.empty() ? L.header : expectedNodes.back()));

  assert(M.header->next == nullptr && M.tail == M.header && M.size == 0);

  // Both lists must remain fully usable afterwards.
  L.pushBack(-7);
  assert(toVector(L).back() == -7 && L.size == expected.size() + 1);
  M.pushBack(99);
  assert(toVector(M) == std::vector<int>{99} && M.size == 1);
}
} // namespace

// =====================================================================
// Tests & Verification
// =====================================================================
int main() {
  std::cout << "=== C-3.8: Concatenating Two Singly Linked Lists ===\n\n";

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

  // Test 7: Chained concatenation relies on the updated tail pointer
  {
    SList<int> L, M, N;
    fill(L, {1, 2});
    fill(M, {3});
    fill(N, {4, 5});
    concatenate(L, M);
    concatenate(L, N);
    assert((toVector(L) == std::vector<int>{1, 2, 3, 4, 5}));
    assert(L.size == 5 && L.tail->data == 5);
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

  std::cout << "\nAll C-3.8 tests passed successfully!\n";
  return 0;
}