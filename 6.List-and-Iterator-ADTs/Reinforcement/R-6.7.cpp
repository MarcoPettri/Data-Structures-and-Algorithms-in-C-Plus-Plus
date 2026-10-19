// File : R-6.7.cpp

/*
    R-6.7 Provide a fully generic version of the class ArrayVector of Code
   Fragment 6.2 using a templated class.
*/

/**
 * @file    R-6.7.cpp
 * @brief   Unit and differential tests for adt::ArrayVector.
 *
 * Covers: construction, assignment, access, capacity, modifiers, iterators,
 * comparisons, move-only / non-default-constructible types, lifetime balance,
 * allocator balance, aliasing safety, exception safety, and a randomised
 * differential test against std::vector.
 */

#include "array_vector.hpp"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// ───────────────────────── mini test framework ─────────────────────────
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

// ───────────────────────── test helper types ─────────────────────────

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
  friend bool operator==(const Tracked &a, const Tracked &b) {
    return a.value == b.value;
  }
};

/// Copy-only type whose copy constructor can be made to throw.
struct ThrowOnCopy {
  static inline int live = 0;
  static inline int countdown = -1; // throw when it reaches 0; -1 = never
  int value;
  explicit ThrowOnCopy(int v) : value(v) { ++live; }
  ThrowOnCopy(const ThrowOnCopy &o) : value(o.value) {
    if (countdown >= 0 && countdown-- == 0)
      throw std::runtime_error("copy failed");
    ++live;
  }
  ThrowOnCopy &operator=(const ThrowOnCopy &) = default;
  ~ThrowOnCopy() { --live; }
};

/// No default constructor.
struct NoDefault {
  int v;
  explicit NoDefault(int x) : v(x) {}
};

/// Allocator that tracks outstanding blocks.
inline std::ptrdiff_t g_live_blocks = 0;
template <typename T> struct CountingAllocator {
  using value_type = T;
  CountingAllocator() = default;
  template <typename U>
  CountingAllocator(const CountingAllocator<U> &) noexcept {}
  T *allocate(std::size_t n) {
    ++g_live_blocks;
    return std::allocator<T>().allocate(n);
  }
  void deallocate(T *p, std::size_t n) noexcept {
    --g_live_blocks;
    std::allocator<T>().deallocate(p, n);
  }
  template <typename U>
  bool operator==(const CountingAllocator<U> &) const noexcept {
    return true;
  }
  template <typename U>
  bool operator!=(const CountingAllocator<U> &) const noexcept {
    return false;
  }
};

template <typename AV, typename SV> static bool same(const AV &a, const SV &b) {
  return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

// ───────────────────────── tests ─────────────────────────

static void test_default_and_size() {
  adt::ArrayVector<int> v;
  CHECK(v.empty());
  CHECK(v.size() == 0);
  CHECK(v.capacity() == 0);
  CHECK(v.begin() == v.end());
}

static void test_constructors() {
  adt::ArrayVector<int> a(3);
  CHECK(a.size() == 3 && a[0] == 0 && a[2] == 0);

  adt::ArrayVector<int> b(
      4, 7); // must pick (count, value), not the iterator range overload
  CHECK(b.size() == 4 && b[3] == 7);

  adt::ArrayVector<int> c{1, 2, 3};
  CHECK(c.size() == 3 && c[1] == 2);

  std::list<int> l{9, 8, 7};
  adt::ArrayVector<int> d(l.begin(), l.end()); // forward iterators
  CHECK(d.size() == 3 && d[0] == 9);

  std::istringstream in("1 2 3 4");
  adt::ArrayVector<int> e((std::istream_iterator<int>(in)),
                          std::istream_iterator<int>()); // input iterators
  CHECK(e.size() == 4 && e[3] == 4);

  adt::ArrayVector<std::string> s(2, "hi");
  CHECK(s[0] == "hi" && s[1] == "hi");
}

static void test_copy_move_assign() {
  adt::ArrayVector<std::string> a{"x", "y", "z"};
  adt::ArrayVector<std::string> b(a);
  CHECK(a == b);
  b[0] = "changed";
  CHECK(a[0] == "x");

  adt::ArrayVector<std::string> c(std::move(a));
  CHECK(c.size() == 3 && a.empty() && a.capacity() == 0);

  adt::ArrayVector<std::string> d;
  d = c;
  CHECK(d == c);
  d = std::move(c);
  CHECK(d.size() == 3 && c.empty());
  d = {"p", "q"};
  CHECK(d.size() == 2 && d[1] == "q");
  d = d; // self-assign must be harmless (silence -Wself-assign by going through
         // a reference)
  auto &r = d;
  d = r;
  CHECK(d.size() == 2);
}

static void test_access() {
  adt::ArrayVector<int> v{10, 20, 30};
  CHECK(v.front() == 10 && v.back() == 30);
  CHECK(v.at(1) == 20);
  CHECK_THROWS_AS(v.at(3), std::out_of_range);
  const auto &cv = v;
  CHECK(cv.at(2) == 30);
  CHECK_THROWS_AS(cv.at(100), std::out_of_range);
  CHECK(v.data() == &v[0]);
}

static void test_push_pop_growth() {
  adt::ArrayVector<int> v;
  for (int i = 0; i < 1000; ++i)
    v.push_back(i);
  CHECK(v.size() == 1000 && v.capacity() >= 1000);
  bool ok = true;
  for (int i = 0; i < 1000; ++i)
    ok &= (v[i] == i);
  CHECK(ok);
  v.pop_back();
  CHECK(v.size() == 999 && v.back() == 998);
  int &r = v.emplace_back(5);
  CHECK(&r == &v.back() && r == 5);
}

static void test_aliasing() {
  const std::string A(60,
                      'A'); // long enough to defeat small-string optimisation
  adt::ArrayVector<std::string> v;
  v.reserve(4);
  v.push_back(A);
  while (v.size() < v.capacity())
    v.push_back("x"); // [A,x,x,x], exactly full

  v.push_back(v[0]); // push_back of own element, forces reallocation
  CHECK(v.size() == 5 && v.back() == A);

  v.reserve(v.capacity()); // no-op; keeps intent explicit
  while (v.size() < v.capacity())
    v.push_back("x"); // full again
  const auto cap_before = v.capacity();
  v.insert(v.begin() + 1, v[0]);    // insert of own element while full
  CHECK(v.capacity() > cap_before); // it really did reallocate
  CHECK(v[0] == A && v[1] == A && v[2] == "x");

  while (v.size() < v.capacity())
    v.push_back("x");               // full again
  v.insert(v.begin(), 2, v.back()); // fill-insert of own element while full
  CHECK(v[0] == "x" && v[1] == "x" && v[2] == A);
}

static void test_insert_erase() {
  adt::ArrayVector<int> v{1, 2, 3};
  auto it = v.insert(v.begin(), 0);
  CHECK(*it == 0 && v.size() == 4 && v[1] == 1);
  it = v.insert(v.end(), 4);
  CHECK(*it == 4 && v.back() == 4);
  it = v.insert(v.begin() + 2, 3, 9);
  CHECK(v.size() == 8 && v[2] == 9 && v[4] == 9 && v[5] == 1 + 1);
  CHECK(it == v.begin() + 2);

  std::vector<int> src{100, 200};
  v.insert(v.begin() + 1, src.begin(), src.end());
  CHECK(v[1] == 100 && v[2] == 200 && v[3] == 1 && v[4] == 9);
  v.insert(v.end(), {7, 8});
  CHECK(v.back() == 8);

  it = v.erase(v.begin());
  CHECK(it == v.begin() && v[0] == 100);
  it = v.erase(v.begin() + 1, v.begin() + 4);
  CHECK(it == v.begin() + 1);
  it = v.erase(v.end(), v.end()); // empty range: no-op
  CHECK(it == v.end());
  v.clear();
  CHECK(v.empty());
}

static void test_resize_reserve_shrink() {
  adt::ArrayVector<int> v{1, 2, 3};
  v.resize(5);
  CHECK(v.size() == 5 && v[3] == 0 && v[4] == 0);
  v.resize(7, 42);
  CHECK(v[6] == 42);
  v.resize(2);
  CHECK(v.size() == 2 && v.back() == 2);

  v.reserve(100);
  CHECK(v.capacity() == 100 && v.size() == 2);
  v.shrink_to_fit();
  CHECK(v.capacity() == 2 && v[1] == 2);
  v.clear();
  v.shrink_to_fit();
  CHECK(v.capacity() == 0 && v.data() == nullptr);

  CHECK_THROWS_AS(v.reserve(v.max_size() + 1), std::length_error);
}

static void test_iterators_and_algorithms() {
  adt::ArrayVector<int> v{5, 3, 1, 4, 2};
  std::sort(v.begin(), v.end());
  CHECK(same(v, std::vector<int>{1, 2, 3, 4, 5}));
  CHECK(*v.rbegin() == 5 && *v.crbegin() == 5 && *(v.rend() - 1) == 1);
  int sum = 0;
  for (int x : v)
    sum += x;
  CHECK(sum == 15);
  static_assert(
      std::is_same_v<std::iterator_traits<
                         adt::ArrayVector<int>::iterator>::iterator_category,
                     std::random_access_iterator_tag>);
}

static void test_swap_and_compare() {
  adt::ArrayVector<int> a{1, 2, 3}, b{4, 5};
  swap(a, b);
  CHECK(a.size() == 2 && b.size() == 3 && a[0] == 4 && b[0] == 1);
  adt::ArrayVector<int> c{1, 2, 3};
  CHECK(b == c && !(b != c));
  CHECK(a > b && b < a && b <= c && b >= c);
}

static void test_assign() {
  adt::ArrayVector<int> v{1, 2, 3};
  v.assign(2, 8);
  CHECK(same(v, std::vector<int>{8, 8}));
  std::vector<int> src{4, 5, 6, 7};
  v.assign(src.begin(), src.end());
  CHECK(same(v, src));
  v.assign({1});
  CHECK(v.size() == 1 && v[0] == 1);
}

static void test_move_only_and_no_default() {
  adt::ArrayVector<std::unique_ptr<int>> v;
  v.push_back(std::make_unique<int>(1));
  v.emplace_back(new int(2));
  v.insert(v.begin(), std::make_unique<int>(0));
  for (int i = 3; i < 50; ++i)
    v.push_back(std::make_unique<int>(i)); // forces reallocations
  CHECK(*v[0] == 0 && *v[1] == 1 && *v[2] == 2 && *v.back() == 49);
  v.erase(v.begin() + 1);
  CHECK(*v[1] == 2);
  auto w = std::move(v);
  CHECK(v.empty() && w.size() == 49);

  adt::ArrayVector<NoDefault> n;
  n.emplace_back(1);
  n.insert(n.begin(), NoDefault(0));
  n.emplace(n.begin() + 1, 5);
  CHECK(n.size() == 3 && n[0].v == 0 && n[1].v == 5 && n[2].v == 1);
}

static void test_lifetime_balance() {
  Tracked::live = 0;
  {
    adt::ArrayVector<Tracked> v;
    for (int i = 0; i < 100; ++i)
      v.push_back(Tracked(i));
    CHECK(Tracked::live == 100);
    v.insert(v.begin() + 10, 5, Tracked(-5));
    v.erase(v.begin(), v.begin() + 20);
    v.resize(30);
    v.resize(200, Tracked(1));
    v.pop_back();
    adt::ArrayVector<Tracked> w(v);
    adt::ArrayVector<Tracked> x(std::move(v));
    w.shrink_to_fit();
    CHECK(Tracked::live == static_cast<int>(w.size() + x.size()));
  }
  CHECK(Tracked::live == 0);
}

static void test_allocator_balance() {
  g_live_blocks = 0;
  {
    adt::ArrayVector<std::string, CountingAllocator<std::string>> v;
    for (int i = 0; i < 200; ++i)
      v.push_back(std::string(40, 'a') + std::to_string(i));
    auto w = v;
    v.insert(v.begin(), w.begin(), w.end());
    v.shrink_to_fit();
    v.clear();
    v.shrink_to_fit();
    w = std::move(v);
  }
  CHECK(g_live_blocks == 0);
}

static void test_exception_safety() {
  ThrowOnCopy::live = 0;
  ThrowOnCopy::countdown = -1;
  {
    adt::ArrayVector<ThrowOnCopy> v;
    v.reserve(4);
    for (int i = 0; i < 4; ++i)
      v.emplace_back(i); // exactly full
    ThrowOnCopy x(99);

    ThrowOnCopy::countdown =
        2; // copy #0 = new element, #1 = relocate elem 0, #2 throws
    bool threw = false;
    try {
      v.push_back(x);
    } catch (const std::runtime_error &) {
      threw = true;
    }
    ThrowOnCopy::countdown = -1;

    CHECK(threw); // strong guarantee: unchanged
    CHECK(v.size() == 4 && v.capacity() == 4);
    CHECK(v[0].value == 0 && v[3].value == 3);
    CHECK(ThrowOnCopy::live == 5); // 4 in v + x, nothing leaked
  }
  CHECK(ThrowOnCopy::live == 0);
}

/// Random operation sequence cross-checked against std::vector.
template <typename T, typename Make>
static void differential(unsigned seed, Make make) {
  std::mt19937 rng(seed);
  adt::ArrayVector<T> a;
  std::vector<T> b;
  bool ok = true;
  for (int step = 0; step < 4000 && ok; ++step) {
    const auto pos = rng() % (b.size() + 1);
    switch (rng() % 8) {
    case 0: {
      T x = make(rng());
      a.push_back(x);
      b.push_back(x);
      break;
    }
    case 1:
      if (!b.empty()) {
        a.pop_back();
        b.pop_back();
      }
      break;
    case 2: {
      T x = make(rng());
      a.insert(a.begin() + pos, x);
      b.insert(b.begin() + pos, x);
      break;
    }
    case 3: {
      auto n = rng() % 5;
      T x = make(rng());
      a.insert(a.begin() + pos, n, x);
      b.insert(b.begin() + pos, n, x);
      break;
    }
    case 4: {
      std::vector<T> chunk;
      for (auto n = rng() % 6; n > 0; --n)
        chunk.push_back(make(rng()));
      a.insert(a.begin() + pos, chunk.begin(), chunk.end());
      b.insert(b.begin() + pos, chunk.begin(), chunk.end());
      break;
    }
    case 5:
      if (!b.empty()) {
        auto p = rng() % b.size();
        a.erase(a.begin() + p);
        b.erase(b.begin() + p);
      }
      break;
    case 6: {
      auto f = rng() % (b.size() + 1);
      auto l = f + rng() % (b.size() - f + 1);
      a.erase(a.begin() + f, a.begin() + l);
      b.erase(b.begin() + f, b.begin() + l);
      break;
    }
    case 7: {
      auto n = rng() % 60;
      a.resize(n);
      b.resize(n);
      break;
    }
    }
    ok = same(a, b);
  }
  CHECK(ok);
}

static void test_differential() {
  differential<int>(1, [](unsigned r) { return static_cast<int>(r % 1000); });
  differential<std::string>(
      2, [](unsigned r) { return std::string(40, 'z') + std::to_string(r); });
}

// ───────────────────────── main ─────────────────────────
int main() {
  RUN(test_default_and_size);
  RUN(test_constructors);
  RUN(test_copy_move_assign);
  RUN(test_access);
  RUN(test_push_pop_growth);
  RUN(test_aliasing);
  RUN(test_insert_erase);
  RUN(test_resize_reserve_shrink);
  RUN(test_iterators_and_algorithms);
  RUN(test_swap_and_compare);
  RUN(test_assign);
  RUN(test_move_only_and_no_default);
  RUN(test_lifetime_balance);
  RUN(test_allocator_balance);
  RUN(test_exception_safety);
  RUN(test_differential);

  std::cout << "\n" << g_checks << " checks, " << g_failures << " failures\n";
  return g_failures == 0 ? 0 : 1;
}