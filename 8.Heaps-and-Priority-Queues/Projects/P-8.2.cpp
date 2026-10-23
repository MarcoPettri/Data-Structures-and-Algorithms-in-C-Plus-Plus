// File : P-8.2.cpp

/*
    P-8.2 Give a C++ implementation of a priority queue based on an unsorted
   list.
*/

#include <cstddef>
#include <cstdlib>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <random>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

#include "unsorted_priority_queue.hpp"

namespace {

using ds::UnsortedPriorityQueue;

// ---------------------------------------------------------------------------
// Minimal test harness
// ---------------------------------------------------------------------------

int g_checks = 0;   ///< Number of CHECKs evaluated.
int g_failures = 0; ///< Number of CHECKs that failed.

/// Record a failed check with its location.
void reportFailure(const char *file, int line, const char *what) {
  ++g_failures;
  std::cerr << file << ':' << line << ": check failed: " << what << '\n';
}

#define CHECK(cond)                                                            \
  do {                                                                         \
    ++g_checks;                                                                \
    if (!(cond))                                                               \
      reportFailure(__FILE__, __LINE__, #cond);                                \
  } while (false)

#define CHECK_THROWS(expr, ExType)                                             \
  do {                                                                         \
    ++g_checks;                                                                \
    bool thrown_ = false;                                                      \
    try {                                                                      \
      (void)(expr);                                                            \
    } catch (const ExType &) {                                                 \
      thrown_ = true;                                                          \
    } catch (...) {                                                            \
    }                                                                          \
    if (!thrown_)                                                              \
      reportFailure(__FILE__, __LINE__, #expr " did not throw " #ExType);      \
  } while (false)

struct TestCase {
  const char *name;
  void (*fn)();
};

std::vector<TestCase> &registry() {
  static std::vector<TestCase> tests;
  return tests;
}

struct Registrar {
  Registrar(const char *name, void (*fn)()) {
    registry().push_back({name, fn});
  }
};

#define TEST(name)                                                             \
  void test_##name();                                                          \
  const Registrar registrar_##name{#name, &test_##name};                       \
  void test_##name()

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Remove every element, returning them in the order the queue yields them.
template <typename Queue>
std::vector<typename Queue::value_type> drain(Queue &q) {
  std::vector<typename Queue::value_type> out;
  while (!q.empty()) {
    out.push_back(q.min());
    q.removeMin();
  }
  return out;
}

/// Orders (key, tag) pairs by key only, so tags reveal tie-breaking behaviour.
struct ByFirst {
  bool operator()(const std::pair<int, char> &a,
                  const std::pair<int, char> &b) const {
    return a.first < b.first;
  }
};

/// Integer "less" that counts how many times it is called.
struct CountingLess {
  std::size_t *calls;
  bool operator()(int a, int b) const {
    ++*calls;
    return a < b;
  }
};

/// Orders unique_ptr<int> by pointee.
struct ByPointee {
  bool operator()(const std::unique_ptr<int> &a,
                  const std::unique_ptr<int> &b) const {
    return *a < *b;
  }
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST(empty_queue_behaviour) {
  UnsortedPriorityQueue<int> q;
  CHECK(q.empty());
  CHECK(q.size() == 0);
  CHECK_THROWS(q.min(), std::out_of_range);
  CHECK_THROWS(q.removeMin(), std::out_of_range);
  CHECK(q.empty()); // a failed call leaves the queue unchanged
}

TEST(yields_ascending_order_by_default) {
  UnsortedPriorityQueue<int> q;
  for (int v : {5, 3, 8, 1, 9, 2})
    q.insert(v);
  CHECK(q.size() == 6);
  CHECK(!q.empty());
  CHECK(drain(q) == std::vector<int>({1, 2, 3, 5, 8, 9}));
  CHECK(q.empty());
}

TEST(greater_comparator_gives_a_max_queue) {
  UnsortedPriorityQueue<int, std::greater<int>> q;
  for (int v : {5, 3, 8, 1, 9, 2})
    q.insert(v);
  CHECK(drain(q) == std::vector<int>({9, 8, 5, 3, 2, 1}));
}

TEST(duplicates_are_all_kept) {
  UnsortedPriorityQueue<int> q;
  for (int v : {3, 1, 1, 2, 3, 1})
    q.insert(v);
  CHECK(drain(q) == std::vector<int>({1, 1, 1, 2, 3, 3}));
}

TEST(min_does_not_remove) {
  UnsortedPriorityQueue<int> q{4, 2, 7};
  CHECK(q.min() == 2);
  CHECK(q.min() == 2);
  CHECK(q.size() == 3);
  q.removeMin();
  CHECK(q.min() == 4);
  CHECK(q.size() == 2);
}

TEST(interleaved_inserts_and_removals) {
  UnsortedPriorityQueue<int> q;
  q.insert(10);
  q.insert(5);
  CHECK(q.min() == 5);
  q.insert(1);
  CHECK(q.min() == 1);
  q.removeMin();
  CHECK(q.min() == 5);
  q.insert(7);
  q.removeMin();
  CHECK(q.min() == 7);
  q.removeMin();
  CHECK(q.min() == 10);
  q.removeMin();
  CHECK(q.empty());
}

TEST(equal_priorities_are_served_first_in_first_out) {
  UnsortedPriorityQueue<std::pair<int, char>, ByFirst> q;
  q.insert({1, 'a'});
  q.insert({0, 'x'});
  q.insert({1, 'b'});
  q.insert({1, 'c'});
  q.emplace(0, 'y');
  const std::vector<std::pair<int, char>> expected{
      {0, 'x'}, {0, 'y'}, {1, 'a'}, {1, 'b'}, {1, 'c'}};
  CHECK(drain(q) == expected);
}

TEST(insert_is_comparison_free_and_min_scans_all_elements) {
  std::size_t calls = 0;
  UnsortedPriorityQueue<int, CountingLess> q(CountingLess{&calls});
  for (int i = 100; i > 0; --i)
    q.insert(i);
  CHECK(calls == 0); // O(1) insertion: no ordering work at all

  (void)q.min();
  CHECK(calls == 99); // exactly n - 1 comparisons: the O(n) scan

  calls = 0;
  q.removeMin();
  CHECK(calls == 99); // finding the victim costs n - 1; unlinking costs none
  CHECK(q.size() == 99);

  calls = 0;
  (void)q.min();
  CHECK(calls == 98);
}

TEST(size_tracking_and_clear) {
  UnsortedPriorityQueue<int> q;
  for (int i = 0; i < 10; ++i) {
    q.insert(i);
    CHECK(q.size() == static_cast<std::size_t>(i) + 1);
  }
  q.removeMin();
  CHECK(q.size() == 9);
  q.clear();
  CHECK(q.empty());
  CHECK(q.size() == 0);
  q.insert(42); // still usable after clear()
  CHECK(q.min() == 42);
}

TEST(constructors) {
  const std::vector<int> source{4, 2, 7};

  UnsortedPriorityQueue<int> fromRange(source.begin(), source.end());
  CHECK(drain(fromRange) == std::vector<int>({2, 4, 7}));

  UnsortedPriorityQueue<int> fromList{9, 1, 5};
  CHECK(drain(fromList) == std::vector<int>({1, 5, 9}));

  UnsortedPriorityQueue<int, std::greater<int>> maxFromList{3, 6, 4};
  CHECK(drain(maxFromList) == std::vector<int>({6, 4, 3}));

  UnsortedPriorityQueue<int, std::greater<int>> withComparator(
      std::greater<int>{});
  CHECK(withComparator.empty());

  UnsortedPriorityQueue<int, std::greater<int>> maxFromRange(
      source.begin(), source.end(), std::greater<int>{});
  CHECK(drain(maxFromRange) == std::vector<int>({7, 4, 2}));
}

TEST(move_only_elements) {
  UnsortedPriorityQueue<std::unique_ptr<int>, ByPointee> q;
  q.insert(std::make_unique<int>(3));
  q.emplace(new int(1));
  q.insert(std::make_unique<int>(2));

  CHECK(*q.min() == 1);
  q.removeMin();
  CHECK(*q.min() == 2);
  q.removeMin();
  CHECK(*q.min() == 3);
  q.removeMin();
  CHECK(q.empty());
}

TEST(copies_are_independent) {
  UnsortedPriorityQueue<int> a{3, 1, 2};
  UnsortedPriorityQueue<int> b = a; // copy construction
  a.removeMin();
  CHECK(a.size() == 2 && b.size() == 3);
  CHECK(b.min() == 1);

  UnsortedPriorityQueue<int> c;
  c = b; // copy assignment
  b.clear();
  CHECK(c.size() == 3 && c.min() == 1);

  UnsortedPriorityQueue<int> d = std::move(c); // move construction
  CHECK(d.size() == 3 && d.min() == 1);
}

TEST(randomized_against_std_multiset) {
  std::mt19937 rng(12345);
  UnsortedPriorityQueue<int> q;
  std::multiset<int> reference;

  for (int step = 0; step < 10000; ++step) {
    if (reference.empty() || rng() % 5 < 3) {
      const int value =
          static_cast<int>(rng() % 100); // small range => many ties
      q.insert(value);
      reference.insert(value);
    } else {
      CHECK(q.min() == *reference.begin());
      q.removeMin();
      reference.erase(reference.begin());
    }
    CHECK(q.size() == reference.size());
    CHECK(q.empty() == reference.empty());
  }
  while (!reference.empty()) { // finish by draining both and comparing
    CHECK(q.min() == *reference.begin());
    q.removeMin();
    reference.erase(reference.begin());
  }
  CHECK(q.empty());
}

} // namespace

int main() {
  int failedTests = 0;
  for (const TestCase &test : registry()) {
    const int before = g_failures;
    try {
      test.fn();
    } catch (const std::exception &e) {
      ++g_failures;
      std::cerr << test.name << ": unexpected exception: " << e.what() << '\n';
    } catch (...) {
      ++g_failures;
      std::cerr << test.name << ": unexpected non-standard exception\n";
    }
    const bool ok = g_failures == before;
    failedTests += ok ? 0 : 1;
    std::cout << (ok ? "[ PASS ] " : "[ FAIL ] ") << test.name << '\n';
  }
  std::cout << registry().size() - static_cast<std::size_t>(failedTests) << '/'
            << registry().size() << " tests passed, " << g_checks << " checks, "
            << g_failures << " failures\n";
  return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}