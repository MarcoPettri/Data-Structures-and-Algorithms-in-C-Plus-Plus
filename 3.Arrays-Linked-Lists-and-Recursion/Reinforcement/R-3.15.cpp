// File: R-3.15.cpp
/*
R-3.15 Give a fully generic implementation of the circularly linked list data
structure by using a templated class.
*/
#include "CircularlyLinkedList.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>
// ── helper: print every element of the list (non-destructive rotate)
// ──────────
template <typename T>
void printList(const std::string &label, CircularlyLinkedList<T> lst) {
  // Received by value so we can rotate without harming the caller's list.
  std::cout << label << " [";
  for (typename CircularlyLinkedList<T>::size_type i = 0; i < lst.size(); ++i) {
    if (i)
      std::cout << ", ";
    std::cout << lst.front();
    lst.rotate();
  }
  std::cout << "]\n";
}

// ── helper: type that counts live instances and can throw on copy ────────────
struct Tracked {
  inline static int live = 0;
  inline static int copies_until_throw = -1; // -1 = never throw

  int v;
  Tracked(int x) : v(x) { ++live; }
  Tracked(const Tracked &o) : v(o.v) {
    if (copies_until_throw == 0)
      throw std::runtime_error("copy failed");
    if (copies_until_throw > 0)
      --copies_until_throw;
    ++live;
  }
  Tracked(Tracked &&o) noexcept : v(o.v) { ++live; }
  ~Tracked() { --live; }
  bool operator==(const Tracked &o) const { return v == o.v; }
};

template <typename F> bool throwsOutOfRange(F f) {
  try {
    f();
  } catch (const std::out_of_range &) {
    return true;
  }
  return false;
}

int main() {
  // ── test 1: default construction ─────────────────────────────────────────
  {
    CircularlyLinkedList<int> lst;
    assert(lst.empty());
    assert(lst.size() == 0);
    std::cout << "Test 1 passed: default construction, empty list.\n";
  }

  // ── test 2: push_back and front/back access ───────────────────────────────
  {
    CircularlyLinkedList<int> lst;
    lst.push_back(1);
    lst.push_back(2);
    lst.push_back(3);
    assert(lst.size() == 3);
    assert(lst.front() == 1);
    assert(lst.back() == 3);
    printList("Test 2 – push_back {1,2,3}:", lst);
    std::cout << "Test 2 passed.\n";
  }

  // ── test 3: push_front ────────────────────────────────────────────────────
  {
    CircularlyLinkedList<int> lst;
    lst.push_front(3);
    lst.push_front(2);
    lst.push_front(1);
    assert(lst.front() == 1);
    assert(lst.back() == 3);
    printList("Test 3 – push_front {1,2,3}:", lst);
    std::cout << "Test 3 passed.\n";
  }

  // ── test 4: pop_front ─────────────────────────────────────────────────────
  {
    CircularlyLinkedList<int> lst{10, 20, 30};
    lst.pop_front();
    assert(lst.front() == 20);
    assert(lst.size() == 2);
    lst.pop_front();
    assert(lst.front() == 30);
    lst.pop_front();
    assert(lst.empty());
    std::cout << "Test 4 passed: pop_front.\n";
  }

  // ── test 5: rotate ────────────────────────────────────────────────────────
  {
    CircularlyLinkedList<int> lst{1, 2, 3, 4};
    lst.rotate(); // {2, 3, 4, 1}
    assert(lst.front() == 2);
    assert(lst.back() == 1);
    lst.rotate(); // {3, 4, 1, 2}
    assert(lst.front() == 3);
    printList("Test 5 – after 2 rotates:", lst);
    std::cout << "Test 5 passed: rotate.\n";
  }

  // ── test 6: initializer-list constructor ──────────────────────────────────
  {
    CircularlyLinkedList<std::string> lst{"alpha", "beta", "gamma"};
    assert(lst.size() == 3);
    assert(lst.front() == "alpha");
    assert(lst.back() == "gamma");
    printList<std::string>("Test 6 – string list:", lst);
    std::cout << "Test 6 passed: initializer_list<string>.\n";
  }

  // ── test 7: copy constructor ──────────────────────────────────────────────
  {
    CircularlyLinkedList<int> a{1, 2, 3};
    CircularlyLinkedList<int> b(a); // copy
    assert(a == b);
    b.pop_front();
    assert(a != b); // modifying copy does not affect original
    assert(a.size() == 3);
    std::cout << "Test 7 passed: copy constructor.\n";
  }

  // ── test 8: move constructor ──────────────────────────────────────────────
  {
    CircularlyLinkedList<int> a{7, 8, 9};
    CircularlyLinkedList<int> b(std::move(a));
    assert(a.empty());
    assert(b.size() == 3);
    assert(b.front() == 7);
    std::cout << "Test 8 passed: move constructor.\n";
  }

  // ── test 9: copy-assignment (copy-and-swap) ───────────────────────────────
  {
    CircularlyLinkedList<int> a{1, 2, 3};
    CircularlyLinkedList<int> b{10, 20};
    b = a;
    assert(b == a);
    std::cout << "Test 9 passed: copy assignment.\n";
  }

  // ── test 10: swap ─────────────────────────────────────────────────────────
  {
    CircularlyLinkedList<int> a{1, 2};
    CircularlyLinkedList<int> b{9, 8, 7};
    swap(a, b);
    assert(a.size() == 3 && a.front() == 9);
    assert(b.size() == 2 && b.front() == 1);
    std::cout << "Test 10 passed: swap.\n";
  }

  // ── test 11: clear ────────────────────────────────────────────────────────
  {
    CircularlyLinkedList<int> lst{1, 2, 3, 4, 5};
    lst.clear();
    assert(lst.empty());
    assert(lst.size() == 0);
    // Re-use after clear
    lst.push_back(42);
    assert(lst.front() == 42 && lst.back() == 42);
    std::cout << "Test 11 passed: clear and re-use.\n";
  }

  // ── test 12: generic with double ──────────────────────────────────────────
  {
    CircularlyLinkedList<double> lst{1.1, 2.2, 3.3};
    assert(lst.size() == 3);
    printList<double>("Test 12 – double list:", lst);
    std::cout << "Test 12 passed: generic with double.\n";
  }

  // ── test 13: single-element edge cases ───────────────────────────────────
  {
    CircularlyLinkedList<int> lst;
    lst.push_back(99);
    assert(lst.front() == 99 && lst.back() == 99);
    lst.rotate(); // rotate on single element is a no-op
    assert(lst.front() == 99);
    lst.pop_front();
    assert(lst.empty());
    std::cout << "Test 13 passed: single-element edge cases.\n";
  }

  // ── test 14: exceptions on empty list ─────────────────────────────────────
  {
    CircularlyLinkedList<int> lst;
    assert(throwsOutOfRange([&] { lst.front(); }));
    assert(throwsOutOfRange([&] { lst.back(); }));
    assert(throwsOutOfRange([&] { lst.pop_front(); }));
    const CircularlyLinkedList<int> &c = lst;
    assert(throwsOutOfRange([&] { c.front(); }));
    assert(throwsOutOfRange([&] { c.back(); }));
    std::cout << "Test 14 passed: out_of_range on empty list.\n";
  }

  // ── test 15: rotate on empty list is a safe no-op ────────────────────────
  {
    CircularlyLinkedList<int> lst;
    lst.rotate();
    assert(lst.empty());
    std::cout << "Test 15 passed: rotate on empty list.\n";
  }

  // ── test 16: self-assignment and move-assignment ─────────────────────────
  {
    CircularlyLinkedList<int> a{1, 2, 3};
    CircularlyLinkedList<int> &ref = a;
    a = ref; // self-assignment
    assert(a.size() == 3 && a.front() == 1 && a.back() == 3);

    CircularlyLinkedList<int> b{9};
    b = std::move(a);
    assert(a.empty());
    assert(b.size() == 3 && b.front() == 1 && b.back() == 3);
    std::cout << "Test 16 passed: self- and move-assignment.\n";
  }

  // ── test 17: equality semantics ──────────────────────────────────────────
  {
    CircularlyLinkedList<int> e1, e2;
    assert(e1 == e2); // two empty lists

    CircularlyLinkedList<int> a{1, 2, 3};
    CircularlyLinkedList<int> shorter{1, 2};
    assert(a != shorter);

    CircularlyLinkedList<int> rotated{2, 3, 1};
    assert(a != rotated); // compared head to head
    a.rotate();           // now {2, 3, 1}
    assert(a == rotated);
    std::cout << "Test 17 passed: equality.\n";
  }

  // ── test 18: copy of an empty list ───────────────────────────────────────
  {
    CircularlyLinkedList<int> e;
    CircularlyLinkedList<int> c(e);
    assert(c.empty());
    c.push_back(5);
    assert(c.front() == 5 && e.empty());
    std::cout << "Test 18 passed: copy of empty list.\n";
  }

  // ── test 19: move-only element type ──────────────────────────────────────
  {
    CircularlyLinkedList<std::unique_ptr<int>> lst;
    lst.push_back(std::make_unique<int>(1));
    lst.push_front(std::make_unique<int>(0));
    assert(*lst.front() == 0 && *lst.back() == 1);
    lst.pop_front();
    assert(*lst.front() == 1);
    std::cout << "Test 19 passed: move-only type.\n";
  }

  // ── test 20: no leak when a copy throws mid-construction ─────────────────
  {
    {
      CircularlyLinkedList<Tracked> src{Tracked(1), Tracked(2), Tracked(3)};
      assert(Tracked::live == 3);

      Tracked::copies_until_throw = 2; // the 3rd copy throws
      bool threw = false;
      try {
        CircularlyLinkedList<Tracked> copy(src);
      } catch (const std::runtime_error &) {
        threw = true;
      }
      Tracked::copies_until_throw = -1;
      assert(threw);
      assert(Tracked::live == 3); // nodes of the failed copy were freed
    }
    assert(Tracked::live == 0);
    std::cout << "Test 20 passed: exception safety of copy constructor.\n";
  }

  // ── test 21: no leak when initializer_list construction throws ───────────
  {
    {
      Tracked::copies_until_throw = 1; // the 2nd copy throws
      bool threw = false;
      try {
        CircularlyLinkedList<Tracked> lst{Tracked(1), Tracked(2), Tracked(3)};
      } catch (const std::runtime_error &) {
        threw = true;
      }
      Tracked::copies_until_throw = -1;
      assert(threw);
    }
    assert(Tracked::live == 0);
    std::cout << "Test 21 passed: exception safety of initializer_list ctor.\n";
  }

  // ── test 22: large list (iterative clear, no stack overflow) ─────────────
  {
    CircularlyLinkedList<int> lst;
    for (int i = 0; i < 1000000; ++i)
      lst.push_back(i);
    assert(lst.size() == 1000000);
    assert(lst.front() == 0 && lst.back() == 999999);
    lst.clear();
    assert(lst.empty());
    std::cout << "Test 22 passed: large list.\n";
  }

  // ── test 23: rvalue overloads ────────────────────────────────────────────
  {
    CircularlyLinkedList<std::string> lst;
    std::string s = "hello";
    lst.push_back(std::move(s));
    lst.push_front(std::string("world"));
    assert(lst.front() == "world" && lst.back() == "hello");
    std::cout << "Test 23 passed: rvalue push_front/push_back.\n";
  }
  // ── test 24: range-based for, read and write ─────────────────────────────
  {
    CircularlyLinkedList<int> lst{1, 2, 3, 4};
    std::vector<int> seen;
    for (int x : lst)
      seen.push_back(x);
    assert((seen == std::vector<int>{1, 2, 3, 4}));

    for (auto &x : lst)
      x *= 10;
    assert(lst.front() == 10 && lst.back() == 40);

    lst.rotate(); // {20, 30, 40, 10}
    seen.clear();
    for (int x : lst)
      seen.push_back(x);
    assert((seen == std::vector<int>{20, 30, 40, 10}));
    std::cout << "Test 24 passed: range-based for.\n";
  }

  // ── test 25: empty and single-element iteration ──────────────────────────
  {
    CircularlyLinkedList<int> e;
    assert(e.begin() == e.end());
    int n = 0;
    for ([[maybe_unused]] int x : e)
      ++n;
    assert(n == 0);

    CircularlyLinkedList<int> one{7};
    auto it = one.begin();
    assert(it != one.end() && *it == 7);
    ++it;
    assert(it == one.end()); // terminates, does not loop forever
    std::cout << "Test 25 passed: empty and single-element iteration.\n";
  }

  // ── test 26: const iteration and iterator -> const_iterator ──────────────
  {
    const CircularlyLinkedList<std::string> lst{"a", "b", "c"};
    std::string joined;
    for (const auto &s : lst)
      joined += s;
    assert(joined == "abc");
    assert(lst.begin()->size() == 1); // operator->

    CircularlyLinkedList<int> m{1, 2, 3};
    CircularlyLinkedList<int>::const_iterator ci =
        m.begin(); // implicit conversion
    assert(ci == m.begin());
    assert(*ci == 1);
    assert(m.cbegin() == m.begin() && m.cend() == m.end());
    std::cout << "Test 26 passed: const iteration.\n";
  }

  // ── test 27: STL algorithms ──────────────────────────────────────────────
  {
    CircularlyLinkedList<int> lst{5, 3, 8, 1};
    assert(std::distance(lst.begin(), lst.end()) == 4);
    assert(std::accumulate(lst.begin(), lst.end(), 0) == 17);
    assert(*std::find(lst.begin(), lst.end(), 8) == 8);
    assert(std::find(lst.begin(), lst.end(), 99) == lst.end());
    assert(*std::max_element(lst.begin(), lst.end()) == 8);
    assert(std::count_if(lst.begin(), lst.end(), [](int x) { return x > 2; }) ==
           3);

    std::vector<int> v(lst.begin(), lst.end());
    assert((v == std::vector<int>{5, 3, 8, 1}));
    std::cout << "Test 27 passed: STL algorithms.\n";
  }

  // ── test 28: post-increment and iterator copies ──────────────────────────
  {
    CircularlyLinkedList<int> lst{1, 2, 3};
    auto it = lst.begin();
    auto old = it++;
    assert(*old == 1 && *it == 2);
    auto copy = it; // multipass: the copy is independent
    ++it;
    assert(*copy == 2 && *it == 3);
    std::cout << "Test 28 passed: post-increment, multipass.\n";
  }
  std::cout << "\nAll tests passed.\n";
  return 0;
}