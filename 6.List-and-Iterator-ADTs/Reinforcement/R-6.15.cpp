// File : R-6.15.cpp

/*
    R-6.15 Provide the details of an array implementation of the list ADT.
*/
/**
 * @brief   Compact tests for adt::ArrayList.
 *
 */

#include "array_list.hpp"

#include <algorithm>
#include <iostream>
#include <list>
#include <memory>
#include <stdexcept>
#include <string>

// ───────────────────────── mini framework ─────────────────────────
static int g_checks = 0, g_failures = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    ++g_checks;                                                                \
    if (!(cond)) {                                                             \
      ++g_failures;                                                            \
      std::cerr << __FILE__ << ':' << __LINE__                                 \
                << ": CHECK failed: " #cond "\n";                              \
    }                                                                          \
  } while (0)

#define CHECK_THROWS_AS(expr, Ex)                                              \
  do {                                                                         \
    ++g_checks;                                                                \
    bool caught_ = false;                                                      \
    try {                                                                      \
      (void)(expr);                                                            \
    } catch (const Ex &) {                                                     \
      caught_ = true;                                                          \
    } catch (...) {                                                            \
    }                                                                          \
    if (!caught_) {                                                            \
      ++g_failures;                                                            \
      std::cerr << __FILE__ << ':' << __LINE__                                 \
                << ": expected " #Ex " from " #expr "\n";                      \
    }                                                                          \
  } while (0)

#define RUN(test)                                                              \
  do {                                                                         \
    std::cout << "[ RUN ] " #test "\n";                                        \
    test();                                                                    \
  } while (0)

// ───────────────────────── helpers ─────────────────────────

/// Counts live instances to detect leaks / double destruction.
struct Tracked {
  static inline int live = 0;
  int value;
  Tracked(int v = 0) : value(v) { ++live; }
  Tracked(const Tracked &o) : value(o.value) { ++live; }
  Tracked(Tracked &&o) noexcept : value(o.value) {
    o.value = -1;
    ++live;
  }
  Tracked &operator=(const Tracked &) = default;
  Tracked &operator=(Tracked &&o) noexcept {
    value = o.value;
    o.value = -1;
    return *this;
  }
  ~Tracked() { --live; }
};

/// No default constructor.
struct NoDefault {
  int v;
  explicit NoDefault(int x) : v(x) {}
};

template <typename L>
static bool equals(const L &l,
                   std::initializer_list<typename L::value_type> expected) {
  return l.size() == expected.size() &&
         std::equal(l.begin(), l.end(), expected.begin());
}

// ───────────────────────── tests ─────────────────────────

static void test_basics() {
  adt::ArrayList<int> l;
  CHECK(l.empty() && l.size() == 0);
  l.push_back(1);
  l.push_back(2);
  l.push_back(3);  // [1,2,3]
  l.push_front(0); // [0,1,2,3]
  CHECK(equals(l, {0, 1, 2, 3}));
  CHECK(l.front() == 0 && l.back() == 3);
  l.set(1, 10); // [0,10,2,3]
  CHECK(l.at(1) == 10 && l[2] == 2);

  adt::ArrayList<int> big;
  for (int i = 0; i < 1000; ++i)
    big.push_back(i);
  CHECK(big.size() == 1000 && big[999] == 999 && big.capacity() >= 1000);
  big.shrink_to_fit();
  CHECK(big.capacity() == 1000);
  big.clear();
  big.shrink_to_fit();
  CHECK(big.capacity() == 0 && big.data() == nullptr);
}

static void test_constructors() {
  adt::ArrayList<int> a(3,
                        7); // must pick (count, value), not the range overload
  CHECK(equals(a, {7, 7, 7}));
  std::list<int> src{3, 2, 1};
  adt::ArrayList<int> b(src.begin(), src.end());
  CHECK(equals(b, {3, 2, 1}));
  adt::ArrayList<int> c{4, 5, 6};
  CHECK(equals(c, {4, 5, 6}));
}

static void test_insert_remove_at() {
  adt::ArrayList<int> l{1, 2, 3};
  l.insert(0, 0);  // [0,1,2,3]
  l.insert(4, 4);  // [0,1,2,3,4]
  l.insert(2, 99); // [0,1,99,2,3,4]
  CHECK(equals(l, {0, 1, 99, 2, 3, 4}));
  CHECK(l.remove_at(2) == 99); // [0,1,2,3,4]
  CHECK(l.remove_at(0) == 0);  // [1,2,3,4]
  CHECK(l.remove_at(3) == 4);  // [1,2,3]
  l.pop_front();               // [2,3]
  l.pop_back();                // [2]
  CHECK(equals(l, {2}));
}

static void test_errors() {
  adt::ArrayList<int> l{1, 2, 3};
  CHECK_THROWS_AS(l.at(3), std::out_of_range);
  CHECK_THROWS_AS(l.set(3, 0), std::out_of_range);
  CHECK_THROWS_AS(l.insert(4, 0), std::out_of_range);
  CHECK_THROWS_AS(l.remove_at(3), std::out_of_range);
  CHECK(equals(l, {1, 2, 3})); // failed calls changed nothing
}

static void test_search_remove() {
  adt::ArrayList<std::string> l{"a", "b", "c", "b"};
  CHECK(l.index_of("b") == 1);
  CHECK(l.index_of("z") == adt::ArrayList<std::string>::npos);
  CHECK(l.contains("c") && !l.contains("z"));
  CHECK(l.remove("b")); // [a,c,b]
  CHECK(l.size() == 3 && l[0] == "a" && l[1] == "c" && l[2] == "b");
  CHECK(!l.remove("zzz"));
}

static void test_copy_move() {
  adt::ArrayList<std::string> a{"x", "y", "z"};
  auto b = a;
  b[0] = "changed";
  CHECK(a[0] == "x" && b[0] == "changed");
  auto c = std::move(a);
  CHECK(c.size() == 3 && a.empty());
  adt::ArrayList<std::string> d;
  d = c;
  CHECK(d == c);
  d = std::move(c);
  CHECK(d.size() == 3 && c.empty());
  swap(d, a);
  CHECK(a.size() == 3 && d.empty());
}

static void test_move_only_and_no_default() {
  adt::ArrayList<std::unique_ptr<int>> l;
  for (int i = 0; i < 20; ++i)
    l.push_back(std::make_unique<int>(i));
  l.insert(5, std::make_unique<int>(-1));
  CHECK(l.size() == 21 && *l[5] == -1 && *l[6] == 5);
  auto p = l.remove_at(5);
  CHECK(*p == -1 && *l[5] == 5);
  l.emplace_front(new int(100));
  CHECK(*l.front() == 100 && l.size() == 21 && *l[1] == 0);
  adt::ArrayList<NoDefault> n;
  n.emplace_back(1);
  n.emplace_front(0);
  n.emplace(1, 5);
  CHECK(n.size() == 3 && n[0].v == 0 && n[1].v == 5 && n[2].v == 1);
}

static void test_aliasing() {
  const std::string A(60,
                      'A'); // long enough to defeat small-string optimisation
  adt::ArrayList<std::string> l;
  l.reserve(3);
  l.push_back(A);
  l.push_back("x");
  l.push_back("y");  // [A,x,y], exactly full
  l.insert(1, l[0]); // reallocating path, argument aliases an element
  CHECK(l.size() == 4 && l[0] == A && l[1] == A && l[2] == "x" && l[3] == "y");

  l.reserve(10);     // spare room: in-place path
  l.insert(0, l[3]); // argument aliases an element that gets shifted
  CHECK(l.size() == 5 && l[0] == "y" && l[1] == A && l[2] == A && l[3] == "x" &&
        l[4] == "y");
}

static void test_lifetime_balance() {
  Tracked::live = 0;
  {
    adt::ArrayList<Tracked> l;
    for (int i = 0; i < 50; ++i)
      l.push_back(Tracked(i));
    l.insert(10, Tracked(-1));
    l.remove_at(0);
    l.pop_back();
    l.pop_front();
    adt::ArrayList<Tracked> c(l);
    CHECK(Tracked::live == static_cast<int>(l.size() + c.size()));
    l.shrink_to_fit();
    l.clear();
    CHECK(Tracked::live == static_cast<int>(c.size()));
  }
  CHECK(Tracked::live == 0);
}

// ───────────────────────── main ─────────────────────────
int main() {
  RUN(test_basics);
  RUN(test_constructors);
  RUN(test_insert_remove_at);
  RUN(test_errors);
  RUN(test_search_remove);
  RUN(test_copy_move);
  RUN(test_move_only_and_no_default);
  RUN(test_aliasing);
  RUN(test_lifetime_balance);

  std::cout << "\n" << g_checks << " checks, " << g_failures << " failures\n";
  return g_failures == 0 ? 0 : 1;
}