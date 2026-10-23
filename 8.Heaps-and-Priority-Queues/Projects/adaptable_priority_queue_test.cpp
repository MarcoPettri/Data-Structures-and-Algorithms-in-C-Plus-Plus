/**
 * @file adaptable_priority_queue_test.cpp
 * @brief Self-contained tests for ds::AdaptablePriorityQueue
 */
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "adaptable_priority_queue.hpp"

namespace adaptable_priority_queue_test {

using ds::AdaptablePriorityQueue;

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

using IntQueue = AdaptablePriorityQueue<int, std::string>;

/// Integer "less" that counts how many times it is called.
struct CountingLess {
  std::size_t *calls;
  bool operator()(int a, int b) const {
    ++*calls;
    return a < b;
  }
};

/// Keys of the entries removeMin() yields, until the queue is empty.
template <typename Queue>
std::vector<typename Queue::key_type> drainKeys(Queue &q) {
  std::vector<typename Queue::key_type> keys;
  while (!q.empty())
    keys.push_back(q.removeMin().key());
  return keys;
}

// ---------------------------------------------------------------------------
// Basic behaviour
// ---------------------------------------------------------------------------

TEST(empty_queue_behaviour) {
  IntQueue q;
  CHECK(q.empty());
  CHECK(q.size() == 0);
  CHECK(q.checkInvariants());
  CHECK_THROWS(q.min(), std::out_of_range);
  CHECK_THROWS(q.removeMin(), std::out_of_range);
}

TEST(insert_returns_locator_with_key_and_value) {
  IntQueue q;
  const auto loc = q.insert(5, "five");
  CHECK(loc.valid());
  CHECK(loc.key() == 5);
  CHECK(loc.value() == "five");
  CHECK(q.contains(loc));
  CHECK(q.size() == 1);
  CHECK(!q.empty());
}

TEST(min_returns_the_smallest_key_without_removing) {
  IntQueue q;
  (void)q.insert(8, "eight");
  const auto three = q.insert(3, "three");
  (void)q.insert(6, "six");
  CHECK(q.min() == three);
  CHECK(q.min().key() == 3);
  CHECK(q.min().value() == "three");
  CHECK(q.size() == 3);
}

TEST(removeMin_yields_keys_in_ascending_order) {
  IntQueue q;
  std::vector<int> keys(100);
  std::iota(keys.begin(), keys.end(), 0);
  std::mt19937 rng(1);
  std::shuffle(keys.begin(), keys.end(), rng);
  for (int k : keys)
    (void)q.insert(k, std::to_string(k));
  CHECK(q.checkInvariants());

  for (int expected = 0; expected < 100; ++expected) {
    const auto removed = q.removeMin();
    CHECK(removed.key() == expected);
    CHECK(removed.value() == std::to_string(expected));
  }
  CHECK(q.empty());
}

TEST(removed_locator_is_detached_but_still_readable) {
  IntQueue q;
  const auto loc = q.insert(1, "one");
  const auto removed = q.removeMin();
  CHECK(removed == loc);
  CHECK(!loc.valid());
  CHECK(!q.contains(loc));
  CHECK(loc.key() == 1);
  CHECK(loc.value() == "one");
}

TEST(greater_comparator_gives_a_max_queue) {
  AdaptablePriorityQueue<int, int, std::greater<int>> q;
  for (int k : {4, 9, 1, 7})
    (void)q.insert(k, k);
  CHECK(drainKeys(q) == std::vector<int>({9, 7, 4, 1}));
}

TEST(duplicate_keys_are_all_kept) {
  AdaptablePriorityQueue<int, int> q;
  for (int id = 0; id < 5; ++id)
    (void)q.insert(7, id);
  (void)q.insert(3, 99);

  CHECK(q.removeMin().value() == 99);
  std::vector<int> ids;
  while (!q.empty()) {
    const auto e = q.removeMin();
    CHECK(e.key() == 7);
    ids.push_back(e.value());
  }
  std::sort(ids.begin(), ids.end());
  CHECK(ids == std::vector<int>({0, 1, 2, 3, 4}));
}

// ---------------------------------------------------------------------------
// Locator-based operations
// ---------------------------------------------------------------------------

TEST(remove_by_locator_from_anywhere_in_the_heap) {
  IntQueue q;
  std::vector<IntQueue::Locator> locs;
  for (int k : {50, 20, 70, 10, 30, 60, 80, 40})
    locs.push_back(q.insert(k, std::to_string(k)));

  q.remove(locs[2]); // 70: an interior node
  CHECK(!locs[2].valid());
  CHECK(q.size() == 7);
  CHECK(q.checkInvariants());

  q.remove(locs[3]); // 10: the current minimum
  CHECK(q.min().key() == 20);

  q.remove(locs[7]); // 40: a leaf (the most recently inserted)
  CHECK(q.checkInvariants());

  CHECK(drainKeys(q) == std::vector<int>({20, 30, 50, 60, 80}));
}

TEST(remove_the_only_entry) {
  IntQueue q;
  const auto loc = q.insert(1, "x");
  q.remove(loc);
  CHECK(q.empty());
  CHECK(!loc.valid());
  CHECK(q.checkInvariants());
}

TEST(replaceKey_to_smaller_key_moves_entry_to_the_top) {
  IntQueue q;
  (void)q.insert(10, "a");
  (void)q.insert(20, "b");
  const auto loc = q.insert(30, "c");

  CHECK(q.replaceKey(loc, 5) == 30); // returns the old key
  CHECK(loc.key() == 5);
  CHECK(q.min() == loc);
  CHECK(q.checkInvariants());
}

TEST(replaceKey_to_larger_key_sinks_entry) {
  IntQueue q;
  const auto loc = q.insert(1, "a");
  (void)q.insert(2, "b");
  (void)q.insert(3, "c");
  CHECK(q.min() == loc);

  CHECK(q.replaceKey(loc, 10) == 1);
  CHECK(q.min().key() == 2);
  CHECK(q.checkInvariants());
  CHECK(drainKeys(q) == std::vector<int>({2, 3, 10}));
}

TEST(replaceKey_with_equal_key_changes_nothing) {
  IntQueue q;
  const auto loc = q.insert(4, "a");
  (void)q.insert(6, "b");
  CHECK(q.replaceKey(loc, 4) == 4);
  CHECK(q.min() == loc);
  CHECK(q.checkInvariants());
}

TEST(replaceValue_swaps_value_and_leaves_order_alone) {
  IntQueue q;
  const auto loc = q.insert(2, "old");
  (void)q.insert(9, "other");

  CHECK(q.replaceValue(loc, "new") == "old");
  CHECK(loc.value() == "new");
  CHECK(q.min() == loc);

  loc.value() += "er"; // value() is a mutable reference
  CHECK(q.min().value() == "newer");
}

// ---------------------------------------------------------------------------
// Invalid locators
// ---------------------------------------------------------------------------

TEST(null_stale_and_foreign_locators_are_rejected) {
  IntQueue a;
  IntQueue b;
  const auto inA = a.insert(1, "a");
  (void)b.insert(2, "b");

  const IntQueue::Locator null;
  CHECK(!null.valid());
  CHECK(!a.contains(null));
  CHECK_THROWS(a.remove(null), std::invalid_argument);
  CHECK_THROWS(a.replaceKey(null, 1), std::invalid_argument);
  CHECK_THROWS(a.replaceValue(null, "x"), std::invalid_argument);
  CHECK_THROWS(null.key(), std::invalid_argument);
  CHECK_THROWS(null.value(), std::invalid_argument);

  CHECK(!b.contains(inA)); // foreign
  CHECK_THROWS(b.remove(inA), std::invalid_argument);
  CHECK_THROWS(b.replaceKey(inA, 0), std::invalid_argument);
  CHECK(b.size() == 1 && a.size() == 1);

  a.remove(inA); // stale
  CHECK_THROWS(a.remove(inA), std::invalid_argument);
  CHECK_THROWS(a.replaceKey(inA, 0), std::invalid_argument);
  CHECK_THROWS(a.replaceValue(inA, "x"), std::invalid_argument);
  CHECK(inA.key() == 1); // a stale locator can still be read
}

TEST(clear_and_destruction_detach_locators) {
  IntQueue::Locator survivor;
  {
    IntQueue q;
    const auto first = q.insert(1, "a");
    survivor = q.insert(2, "b");
    q.clear();
    CHECK(q.empty());
    CHECK(!first.valid() && !survivor.valid());

    survivor = q.insert(3, "c");
    CHECK(survivor.valid());
  }
  CHECK(!survivor.valid()); // the queue is gone
  CHECK(survivor.key() == 3 && survivor.value() == "c");
}

TEST(comparing_locators) {
  IntQueue q;
  const auto a = q.insert(1, "a");
  const auto b = q.insert(1, "b");
  const IntQueue::Locator copyOfA = a;
  CHECK(a == copyOfA);
  CHECK(a != b);
  CHECK(IntQueue::Locator() == IntQueue::Locator());
}

// ---------------------------------------------------------------------------
// Moving, move-only values
// ---------------------------------------------------------------------------

TEST(move_construction_keeps_locators_valid) {
  IntQueue src;
  const auto loc = src.insert(7, "seven");
  (void)src.insert(9, "nine");

  IntQueue dst = std::move(src);
  CHECK(src.size() == 0);
  CHECK(!src.contains(loc));
  CHECK(dst.contains(loc));
  CHECK(dst.size() == 2);

  dst.replaceKey(loc, 100);
  CHECK(dst.min().key() == 9);
  CHECK(dst.checkInvariants());
}

TEST(move_assignment_detaches_old_entries_and_keeps_new_ones) {
  IntQueue a;
  const auto inA = a.insert(1, "a");
  IntQueue b;
  const auto inB = b.insert(2, "b");

  a = std::move(b);
  CHECK(!inA.valid());
  CHECK(a.contains(inB));
  CHECK(a.size() == 1);
  CHECK(a.checkInvariants());
}

TEST(move_only_values) {
  AdaptablePriorityQueue<int, std::unique_ptr<std::string>> q;
  const auto loc = q.insert(2, std::make_unique<std::string>("two"));
  (void)q.insert(1, std::make_unique<std::string>("one"));

  CHECK(*q.min().value() == "one");

  auto old = q.replaceValue(loc, std::make_unique<std::string>("TWO"));
  CHECK(*old == "two");
  CHECK(*loc.value() == "TWO");

  auto taken =
      std::move(loc.value()); // take ownership of a value out of an entry
  CHECK(*taken == "TWO");
  CHECK(loc.value() == nullptr);
}

// ---------------------------------------------------------------------------
// Complexity: count comparisons
// ---------------------------------------------------------------------------

TEST(operations_cost_logarithmically_many_comparisons) {
  std::size_t calls = 0;
  AdaptablePriorityQueue<int, int, CountingLess> q{CountingLess{&calls}};

  std::mt19937 rng(2026);
  std::vector<AdaptablePriorityQueue<int, int, CountingLess>::Locator> locs;
  for (int i = 0; i < 1024; ++i) {
    locs.push_back(q.insert(static_cast<int>(rng() % 100000), i));
  }
  // n = 1024: heap height h = 10. A sift costs at most 2h + 2 = 22 comparisons.
  const std::size_t bound = 22;

  calls = 0;
  (void)q.min();
  CHECK(calls == 0); // O(1): just look at the root

  calls = 0;
  (void)q.replaceKey(locs[500], -1);
  CHECK(calls <= bound);
  calls = 0;
  (void)q.replaceKey(locs[200], 1 << 30);
  CHECK(calls <= bound);
  calls = 0;
  q.remove(locs[300]);
  CHECK(calls <= bound);
  calls = 0;
  (void)q.removeMin();
  CHECK(calls <= bound);
  calls = 0;
  (void)q.insert(7, 7);
  CHECK(calls <= 11); // insert only sifts up: at most h comparisons
  CHECK(q.checkInvariants());
}

// ---------------------------------------------------------------------------
// Randomized differential test against a naive model
// ---------------------------------------------------------------------------

TEST(randomized_against_reference_model) {
  using Queue = AdaptablePriorityQueue<int, int>;
  struct Live {
    int key;
    Queue::Locator loc;
  };

  std::mt19937 rng(777);
  Queue q;
  std::map<int, Live>
      live; // value stored in the queue (an id) -> what we expect
  int nextId = 0;

  const auto minKey = [&live] {
    return std::min_element(live.begin(), live.end(),
                            [](const auto &a, const auto &b) {
                              return a.second.key < b.second.key;
                            })
        ->second.key;
  };
  const auto randomLive = [&live, &rng] {
    auto it = live.begin();
    std::advance(it, static_cast<std::ptrdiff_t>(rng() % live.size()));
    return it;
  };

  for (int step = 0; step < 20000; ++step) {
    const unsigned op = static_cast<unsigned>(rng() % 10);
    if (op < 4 || live.empty()) { // insert
      const int key = static_cast<int>(rng() % 200);
      live.emplace(nextId, Live{key, q.insert(key, nextId)});
      ++nextId;
    } else if (op < 5) { // removeMin
      const int expectedKey = minKey();
      const auto removed = q.removeMin();
      CHECK(removed.key() == expectedKey);
      CHECK(!removed.valid());
      const auto it = live.find(removed.value());
      CHECK(it != live.end());
      if (it != live.end()) {
        CHECK(it->second.key ==
              expectedKey); // the entry we removed really had the min key
        live.erase(it);
      }
    } else if (op < 7) { // remove an arbitrary entry
      const auto it = randomLive();
      q.remove(it->second.loc);
      CHECK(!it->second.loc.valid());
      live.erase(it);
    } else if (op < 9) { // replaceKey
      const auto it = randomLive();
      const int newKey = static_cast<int>(rng() % 200);
      CHECK(q.replaceKey(it->second.loc, newKey) == it->second.key);
      it->second.key = newKey;
      CHECK(it->second.loc.key() == newKey);
    } else { // replaceValue (with the same id, so the model stays in sync)
      const auto it = randomLive();
      CHECK(q.replaceValue(it->second.loc, it->first) == it->first);
    }

    CHECK(q.size() == live.size());
    CHECK(q.checkInvariants());
    if (!live.empty())
      CHECK(q.min().key() == minKey());
  }

  int previous = -1; // finally drain: keys must come out non-decreasing
  while (!q.empty()) {
    const int key = q.removeMin().key();
    CHECK(key >= previous);
    previous = key;
  }
}

} // namespace adaptable_priority_queue_test
