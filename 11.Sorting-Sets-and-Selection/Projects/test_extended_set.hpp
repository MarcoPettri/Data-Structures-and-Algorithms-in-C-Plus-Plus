/**
 * @file test_extended_set.hpp
 * @brief Self-contained unit tests for algo::ExtendedSet (no external framework).

 */

#include "extended_set.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iostream>
#include <iterator>
#include <random>
#include <set>
#include <string>
#include <vector>

namespace test_extended_set {

int g_checks = 0;
int g_failures = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    ++g_checks;                                                                \
    if (!(cond)) {                                                             \
      ++g_failures;                                                            \
      std::cerr << "  FAIL " << __FILE__ << ':' << __LINE__ << "  " << #cond   \
                << '\n';                                                       \
    }                                                                          \
  } while (false)

using IntSet = algo::ExtendedSet<int>;
using Vec = std::vector<int>;

/// Runs one test function and reports it.
void run(const char *name, void (*fn)()) {
  const int before = g_failures;
  fn();
  std::cout << (g_failures == before ? "[ OK ] " : "[FAIL] ") << name << '\n';
}

/// Copies any iterable set-like container into a vector.
template <typename S> std::vector<typename S::value_type> toVec(const S &s) {
  return std::vector<typename S::value_type>(s.begin(), s.end());
}

/// True if strictly increasing (sorted, no duplicates).
template <typename S> bool strictlySorted(const S &s) {
  return std::adjacent_find(s.begin(), s.end(), std::greater_equal<>()) ==
         s.end();
}

// ---------------------------------------------------------------------------
// Basics
// ---------------------------------------------------------------------------

void test_empty_set() {
  IntSet s;
  CHECK(s.empty());
  CHECK(s.size() == 0);
  CHECK(!s.contains(0));
  CHECK(!s.remove(0));
  CHECK(s.begin() == s.end());
}

void test_insert_contains_size() {
  IntSet s;
  CHECK(s.insert(5));
  CHECK(s.insert(1));
  CHECK(s.insert(9));
  CHECK(!s.insert(5));  // duplicate
  CHECK(s.size() == 3);
  CHECK(!s.empty());
  CHECK(s.contains(1) && s.contains(5) && s.contains(9));
  CHECK(!s.contains(2) && !s.contains(0) && !s.contains(10));
  CHECK((toVec(s) == Vec{1, 5, 9}));
}

void test_insert_orders() {
  IntSet asc, desc, mixed;
  for (int i = 0; i < 100; ++i)
    asc.insert(i);          // fast path (append)
  for (int i = 99; i >= 0; --i)
    desc.insert(i);         // always front
  for (int i : {50, 10, 90, 30, 70, 20, 80, 40, 60, 0})
    mixed.insert(i);

  CHECK(asc.size() == 100 && strictlySorted(asc));
  CHECK(desc.size() == 100 && strictlySorted(desc));
  CHECK(asc == desc);
  CHECK((toVec(mixed) == Vec{0, 10, 20, 30, 40, 50, 60, 70, 80, 90}));
}

void test_insert_rvalue_and_lvalue() {
  algo::ExtendedSet<std::string> s;
  std::string lv = "beta";
  CHECK(s.insert(lv));
  CHECK(s.insert(std::string("alpha")));
  CHECK(!s.insert(std::string("beta")));
  CHECK((toVec(s) == std::vector<std::string>{"alpha", "beta"}));
}

void test_remove() {
  IntSet s{1, 2, 3, 4, 5};
  CHECK(s.remove(3));
  CHECK(!s.remove(3));
  CHECK(s.remove(1));  // front
  CHECK(s.remove(5));  // back
  CHECK(!s.remove(42));
  CHECK((toVec(s) == Vec{2, 4}));
  s.remove(2);
  s.remove(4);
  CHECK(s.empty());
}

void test_clear_and_swap() {
  IntSet a{1, 2, 3}, b{9};
  a.swap(b);
  CHECK((toVec(a) == Vec{9}));
  CHECK((toVec(b) == Vec{1, 2, 3}));
  swap(a, b);  // ADL free swap
  CHECK((toVec(a) == Vec{1, 2, 3}));
  a.clear();
  CHECK(a.empty());
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

void test_construction() {
  IntSet fromList{5, 1, 5, 3, 1, 9, 3};
  CHECK((toVec(fromList) == Vec{1, 3, 5, 9}));

  const Vec src{4, 4, 2, 8, 2, 6};
  IntSet fromRange(src.begin(), src.end());
  CHECK((toVec(fromRange) == Vec{2, 4, 6, 8}));

  IntSet empty{};
  CHECK(empty.empty());

  algo::ExtendedSet<int, std::greater<int>> withCmp{std::greater<int>{}};
  CHECK(withCmp.empty());
}

void test_custom_comparator() {
  algo::ExtendedSet<int, std::greater<int>> a{1, 5, 3};
  algo::ExtendedSet<int, std::greater<int>> b{3, 4};
  CHECK((toVec(a) == Vec{5, 3, 1}));  // descending iteration

  auto u = a | b;
  auto i = a & b;
  auto d = a - b;
  CHECK((toVec(u) == Vec{5, 4, 3, 1}));
  CHECK((toVec(i) == Vec{3}));
  CHECK((toVec(d) == Vec{5, 1}));
  CHECK(u.contains(4) && !u.contains(2));
}

// ---------------------------------------------------------------------------
// Set operations: known values
// ---------------------------------------------------------------------------

void test_unite() {
  IntSet a{1, 3, 5, 7};
  a.unite(IntSet{3, 4, 5, 8});
  CHECK((toVec(a) == Vec{1, 3, 4, 5, 7, 8}));

  IntSet empty;
  IntSet x{1, 2};
  x.unite(empty);
  CHECK((toVec(x) == Vec{1, 2}));
  empty.unite(x);  // empty receiver
  CHECK((toVec(empty) == Vec{1, 2}));

  IntSet lo{1, 2, 3}, hi{10, 11};  // append fast path
  lo.unite(hi);
  CHECK((toVec(lo) == Vec{1, 2, 3, 10, 11}));

  IntSet hi2{10, 11}, lo2{1, 2, 3};  // other entirely below
  hi2.unite(lo2);
  CHECK((toVec(hi2) == Vec{1, 2, 3, 10, 11}));

  IntSet same{1, 2, 3};
  same.unite(IntSet{1, 2, 3});
  CHECK((toVec(same) == Vec{1, 2, 3}));
}

void test_intersect() {
  IntSet a{1, 3, 5, 7, 9};
  a.intersect(IntSet{3, 4, 5, 9, 10});
  CHECK((toVec(a) == Vec{3, 5, 9}));

  IntSet b{1, 2, 3};
  b.intersect(IntSet{});
  CHECK(b.empty());

  IntSet c;
  c.intersect(IntSet{1, 2});
  CHECK(c.empty());

  IntSet d{1, 2, 3};
  d.intersect(IntSet{4, 5, 6});  // disjoint
  CHECK(d.empty());

  IntSet e{1, 2, 3};
  e.intersect(IntSet{0, 1, 2, 3, 4});  // subset
  CHECK((toVec(e) == Vec{1, 2, 3}));
}

void test_subtract() {
  IntSet a{1, 3, 5, 7, 9};
  a.subtract(IntSet{3, 4, 5, 10});
  CHECK((toVec(a) == Vec{1, 7, 9}));

  IntSet b{1, 2, 3};
  b.subtract(IntSet{});
  CHECK((toVec(b) == Vec{1, 2, 3}));

  IntSet c;
  c.subtract(IntSet{1});
  CHECK(c.empty());

  IntSet d{1, 2, 3};
  d.subtract(IntSet{1, 2, 3, 4});  // everything removed
  CHECK(d.empty());

  IntSet e{1, 2, 3, 4, 5};
  e.subtract(IntSet{1});  // prefix removed, tail shifts left
  CHECK((toVec(e) == Vec{2, 3, 4, 5}));

  IntSet f{1, 2, 3, 4, 5};
  f.subtract(IntSet{5});  // suffix removed
  CHECK((toVec(f) == Vec{1, 2, 3, 4}));

  IntSet g{1, 2, 3};
  g.subtract(IntSet{7, 8});  // disjoint: unchanged
  CHECK((toVec(g) == Vec{1, 2, 3}));
}

void test_self_aliasing() {
  IntSet a{1, 2, 3};
  a.unite(a);
  CHECK((toVec(a) == Vec{1, 2, 3}));
  a.intersect(a);
  CHECK((toVec(a) == Vec{1, 2, 3}));
  a.subtract(a);
  CHECK(a.empty());
}

void test_equals_and_operators() {
  IntSet a{1, 2, 3}, b{3, 2, 1}, c{1, 2}, d{1, 2, 4};
  CHECK(a.equals(b));
  CHECK(a == b);
  CHECK(a != c);   // different size
  CHECK(a != d);   // same size, different element
  CHECK(a.equals(a));
  CHECK(IntSet{}.equals(IntSet{}));

  IntSet x{1, 2, 3, 4}, y{3, 4, 5};
  CHECK((toVec(x | y) == Vec{1, 2, 3, 4, 5}));
  CHECK((toVec(x & y) == Vec{3, 4}));
  CHECK((toVec(x - y) == Vec{1, 2}));

  IntSet z = x;
  z |= y;
  CHECK((toVec(z) == Vec{1, 2, 3, 4, 5}));
  z &= y;
  CHECK(z == y);
  z -= IntSet{4};
  CHECK((toVec(z) == Vec{3, 5}));

  // operands of the non-mutating operators are untouched
  CHECK((toVec(x) == Vec{1, 2, 3, 4}));
  CHECK((toVec(y) == Vec{3, 4, 5}));
}

void test_algebraic_identities() {
  IntSet a{1, 2, 3, 4, 5}, b{4, 5, 6, 7};
  // |A ∪ B| = |A| + |B| - |A ∩ B|
  CHECK((a | b).size() == a.size() + b.size() - (a & b).size());
  // A \ B and A ∩ B partition A
  CHECK(((a - b) | (a & b)) == a);
  CHECK((a - b & b).empty());
  // commutativity
  CHECK((a | b) == (b | a));
  CHECK((a & b) == (b & a));
}

void test_strings() {
  using SSet = algo::ExtendedSet<std::string>;
  SSet a{"apple", "cherry", "fig"};
  SSet b{"banana", "cherry", "date"};
  CHECK((toVec(a | b) ==
         std::vector<std::string>{"apple", "banana", "cherry", "date", "fig"}));
  CHECK((toVec(a & b) == std::vector<std::string>{"cherry"}));
  CHECK((toVec(a - b) == std::vector<std::string>{"apple", "fig"}));
}

// ---------------------------------------------------------------------------
// Randomised comparison against std::set
// ---------------------------------------------------------------------------

void test_randomised_against_std_set() {
  std::mt19937 gen(2026);

  for (int iter = 0; iter < 1000; ++iter) {
    const int range = 1 + static_cast<int>(gen() % 200);
    const int na = static_cast<int>(gen() % 150);
    const int nb = static_cast<int>(gen() % 150);
    std::uniform_int_distribution<int> dist(0, range - 1);

    IntSet a, b;
    std::set<int> sa, sb;
    for (int i = 0; i < na; ++i) {
      const int v = dist(gen);
      a.insert(v);
      sa.insert(v);
    }
    for (int i = 0; i < nb; ++i) {
      const int v = dist(gen);
      b.insert(v);
      sb.insert(v);
    }

    CHECK(toVec(a) == toVec(sa));
    CHECK(a.size() == sa.size());

    Vec expected;
    std::set_union(sa.begin(), sa.end(), sb.begin(), sb.end(),
                   std::back_inserter(expected));
    IntSet u = a;
    u.unite(b);
    CHECK(toVec(u) == expected);
    CHECK(strictlySorted(u));

    expected.clear();
    std::set_intersection(sa.begin(), sa.end(), sb.begin(), sb.end(),
                          std::back_inserter(expected));
    IntSet i = a;
    i.intersect(b);
    CHECK(toVec(i) == expected);
    CHECK(strictlySorted(i));

    expected.clear();
    std::set_difference(sa.begin(), sa.end(), sb.begin(), sb.end(),
                        std::back_inserter(expected));
    IntSet d = a;
    d.subtract(b);
    CHECK(toVec(d) == expected);
    CHECK(strictlySorted(d));

    CHECK(a.equals(b) == (sa == sb));

    // contains / remove on a few random probes
    for (int k = 0; k < 5; ++k) {
      const int v = dist(gen);
      CHECK(a.contains(v) == (sa.count(v) == 1));
      CHECK(a.remove(v) == (sa.erase(v) == 1));
    }
    CHECK(toVec(a) == toVec(sa));
  }
}

void test_large_sets() {
  IntSet a, b;
  for (int i = 0; i < 200000; i += 2)
    a.insert(i);  // evens
  for (int i = 0; i < 200000; i += 3)
    b.insert(i);  // multiples of 3

  CHECK((a & b).size() == 33334);  // multiples of 6 in [0, 200000)
  CHECK((a | b).size() == a.size() + b.size() - (a & b).size());
  CHECK((a - b).size() == a.size() - (a & b).size());
  CHECK(strictlySorted(a | b));
}

}  // namespace test_extended_set
