// File : P-5.1.cpp

/*
    P-5.1 Give an implementation of the deque ADT using an array, so that each
   of the update functions run in O(1) time.
*/

// main.cpp - compile: g++ -std=c++17 -O2 -Wall -Wextra main.cpp -o deque
// (only main.cpp is compiled; the .tpp is pulled in through the .hpp)
#include <cassert>
#include <deque>
#include <iostream>
#include <memory>
#include <random>
#include <stdexcept>

#include "array_deque.hpp"

//--- TEST ---
void testFixedCapacity();
void testCopy();
void testMoveLeavesSourceValid();
void testSwapAndMoveOnlyElements();
void testEmptyErrors();
void testWraparound();
void testGrowable();
void testConstAccess();
void testMoveOnlyType();
void testRandomized();

int main() {
  testFixedCapacity();
  testCopy();
  testMoveLeavesSourceValid();
  testSwapAndMoveOnlyElements();
  testEmptyErrors();
  testWraparound();
  testGrowable();
  testConstAccess();
  testMoveOnlyType();
  testRandomized();
  std::cout << "All tests passed.\n";
  return 0;
}
// -- TEST --
template <typename E, typename F> bool throwsAs(F &&f) {
  try {
    f();
  } catch (const E &) {
    return true;
  } catch (...) {
    return false;
  }
  return false;
}

void testFixedCapacity() {
  ArrayDeque<int> d(3);
  assert(d.empty() && !d.full());
  d.pushBack(2);
  d.pushFront(1);
  d.pushBack(3);
  assert(d.full() && d.size() == 3);
  assert(d.first() == 1 && d.last() == 3);

  // Full: both pushes throw and leave the contents untouched.
  assert(throwsAs<std::length_error>([&] { d.pushBack(4); }));
  assert(throwsAs<std::length_error>([&] { d.pushFront(0); }));
  assert(d.size() == 3 && d.at(0) == 1 && d.at(1) == 2 && d.at(2) == 3);

  d.popFront();
  d.popBack();
  assert(d.size() == 1 && d.first() == 2 && d.last() == 2);
  d.popBack();
  assert(d.empty());
}

void testEmptyErrors() {
  ArrayDeque<int> d(2);
  assert(throwsAs<std::out_of_range>([&] { d.first(); }));
  assert(throwsAs<std::out_of_range>([&] { d.last(); }));
  assert(throwsAs<std::out_of_range>([&] { d.popFront(); }));
  assert(throwsAs<std::out_of_range>([&] { d.popBack(); }));
  assert(throwsAs<std::out_of_range>([&] { d.at(0); }));
  assert(throwsAs<std::invalid_argument>([] { ArrayDeque<int> bad(0); }));
}

void testWraparound() {
  ArrayDeque<int> d(4);
  for (int i = 1; i <= 4; ++i)
    d.pushBack(i); // 1 2 3 4
  d.popFront();
  d.popFront(); // 3 4
  d.pushBack(5);
  d.pushBack(6); // 3 4 5 6 (physically wrapped)
  assert(d.full());
  for (int i = 0; i < 4; ++i)
    assert(d.at(i) == 3 + i);

  d.popBack();    // 3 4 5
  d.pushFront(2); // 2 3 4 5 (front index wraps backward)
  for (int i = 0; i < 4; ++i)
    assert(d.at(i) == 2 + i);
  assert(d.first() == 2 && d.last() == 5);
}

void testGrowable() {
  ArrayDeque<int> d(2, /*growable=*/true);
  d.pushBack(1);
  d.pushBack(2);
  d.popFront();
  d.pushBack(3); // buffer is physically wrapped here
  d.pushBack(4); // triggers grow() while wrapped
  assert(d.capacity() == 4 && d.size() == 3);
  assert(d.at(0) == 2 && d.at(1) == 3 && d.at(2) == 4);

  for (int i = 0; i < 1000; ++i)
    d.pushFront(-i);
  assert(d.size() == 1003 && d.last() == 4);
}

void testConstAccess() {
  ArrayDeque<int> d(3);
  d.pushBack(7);
  d.pushBack(8);
  const ArrayDeque<int> &c = d;
  assert(c.first() == 7 && c.last() == 8 && c.at(1) == 8);
}

void testMoveOnlyType() {
  ArrayDeque<std::unique_ptr<int>> d(2, true);
  d.pushBack(std::make_unique<int>(10));
  d.pushFront(std::make_unique<int>(5));
  d.pushBack(std::make_unique<int>(20)); // forces growth with move-only items
  assert(*d.first() == 5 && *d.last() == 20);
  d.popFront();
  assert(*d.first() == 10);
}

// Compare against std::deque after every operation.
void randomizedAgainstStd(std::size_t initialCap, bool growable,
                          unsigned seed) {
  std::mt19937 rng(seed);
  ArrayDeque<int> d(initialCap, growable);
  std::deque<int> ref;
  for (int step = 0; step < 100000; ++step) {
    int op = static_cast<int>(rng() % 4);
    int v = static_cast<int>(rng() % 1000);
    switch (op) {
    case 0:
      if (growable || !d.full()) {
        d.pushFront(v);
        ref.push_front(v);
      }
      break;
    case 1:
      if (growable || !d.full()) {
        d.pushBack(v);
        ref.push_back(v);
      }
      break;
    case 2:
      if (!d.empty()) {
        d.popFront();
        ref.pop_front();
      }
      break;
    case 3:
      if (!d.empty()) {
        d.popBack();
        ref.pop_back();
      }
      break;
    }
    assert(d.size() == ref.size());
    if (!ref.empty()) {
      assert(d.first() == ref.front());
      assert(d.last() == ref.back());
    }
    if (step % 1000 == 0)
      for (std::size_t i = 0; i < ref.size(); ++i)
        assert(d.at(i) == ref[i]);
  }
}

void testRandomized() {
  randomizedAgainstStd(16, false, 12345);
  randomizedAgainstStd(1, true, 54321);
}

void testCopy() {
  ArrayDeque<int> a(4);
  for (int i = 1; i <= 4; ++i)
    a.pushBack(i);
  a.popFront();
  a.popFront();
  a.pushBack(5);
  a.pushBack(6); // a is physically wrapped: 3 4 5 6

  ArrayDeque<int> b(a); // copy constructor
  for (int i = 0; i < 4; ++i)
    assert(b.at(i) == 3 + i);

  b.popFront(); // independence: changing b leaves a alone
  b.pushBack(99);
  assert(a.size() == 4 && a.first() == 3 && a.last() == 6);
  assert(b.first() == 4 && b.last() == 99);

  ArrayDeque<int> c(2);
  c = a; // copy assignment
  assert(c.capacity() == 4 && c.first() == 3 && c.last() == 6);

  c = c; // self-assignment is safe
  assert(c.size() == 4 && c.first() == 3);
}

void testMoveLeavesSourceValid() {
  ArrayDeque<int> a(3);
  a.pushBack(1);
  a.pushBack(2);
  ArrayDeque<int> b(std::move(a)); // move constructor

  assert(b.size() == 2 && b.first() == 1 && b.last() == 2);
  // Deliberate use-after-move: the moved-from deque is empty with capacity 0.
  assert(a.empty() && a.capacity() == 0 && a.full());
  assert(throwsAs<std::out_of_range>([&] { a.first(); }));
  assert(throwsAs<std::out_of_range>([&] { a.popBack(); }));
  assert(throwsAs<std::length_error>([&] { a.pushBack(7); })); // non-growable

  ArrayDeque<int> g(2, true);
  g.pushBack(1);
  ArrayDeque<int> h(std::move(g));
  g.pushBack(9); // growable: reallocates from capacity 0
  assert(g.size() == 1 && g.capacity() == 1 && g.first() == 9);

  ArrayDeque<int> d(1);
  d = std::move(b); // move assignment
  assert(d.size() == 2 && d.first() == 1 && d.last() == 2);
}

void testSwapAndMoveOnlyElements() {
  ArrayDeque<int> a(2), b(3);
  a.pushBack(1);
  b.pushBack(2);
  b.pushBack(3);
  swap(a, b); // found via ADL (the friend function)
  assert(a.size() == 2 && a.capacity() == 3 && b.size() == 1 && b.first() == 1);

  // Move-only elements still work as long as the deque itself is moved, not
  // copied.
  ArrayDeque<std::unique_ptr<int>> p(2, true);
  p.pushBack(std::make_unique<int>(42));
  ArrayDeque<std::unique_ptr<int>> q(std::move(p));
  assert(*q.first() == 42);
}