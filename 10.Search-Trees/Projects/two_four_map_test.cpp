/**
 * @file two_four_map_test.cpp
 * @brief Self-contained unit tests for ds::TwoFourMap (no external framework).

 */

#include "two_four_map.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <vector>

namespace TwoFourMap_Test {

int g_failures = 0;
int g_checks = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    ++g_checks;                                                                \
    if (!(cond)) {                                                             \
      ++g_failures;                                                            \
      std::cerr << __FILE__ << ":" << __LINE__ << " CHECK failed: " #cond      \
                << '\n';                                                       \
    }                                                                          \
  } while (0)

#define RUN(test)                                                              \
  do {                                                                         \
    const int before = g_failures;                                             \
    test();                                                                    \
    std::cout << (g_failures == before ? "[ OK ] " : "[FAIL] ") << #test       \
              << '\n';                                                         \
  } while (0)

using Map = ds::TwoFourMap<int, std::string>;

void testEmpty() {
  Map m;
  CHECK(m.empty());
  CHECK(m.size() == 0);
  CHECK(m.height() == 0);
  CHECK(m.begin() == m.end());
  CHECK(m.find(1) == m.end());
  CHECK(!m.contains(1));
  CHECK(m.firstEntry() == m.end());
  CHECK(m.lastEntry() == m.end());
  CHECK(m.ceilingEntry(0) == m.end());
  CHECK(m.floorEntry(0) == m.end());
  CHECK(m.higherEntry(0) == m.end());
  CHECK(m.lowerEntry(0) == m.end());
  CHECK(!m.erase(1));
  CHECK(m.isValid());
}

void testPutFindOverwrite() {
  Map m;
  auto r1 = m.put(10, "ten");
  CHECK(r1.second);
  CHECK(r1.first->second == "ten");

  auto r2 = m.put(10, "TEN");
  CHECK(!r2.second);
  CHECK(m.size() == 1);
  CHECK(m.find(10)->second == "TEN");

  auto r3 = m.insert(10, "nope"); // insert must not overwrite
  CHECK(!r3.second);
  CHECK(m.at(10) == "TEN");

  m.put(5, "five");
  m.put(20, "twenty");
  CHECK(m.size() == 3);
  CHECK(m.contains(5) && m.contains(20) && !m.contains(7));
  CHECK(m.isValid());
}

void testSubscriptAndAt() {
  Map m;
  m[3] = "three";
  CHECK(m.size() == 1);
  CHECK(m[3] == "three");
  CHECK(m[4].empty()); // default-inserted
  CHECK(m.size() == 2);

  const Map &cm = m;
  CHECK(cm.at(3) == "three");
  bool thrown = false;
  try {
    (void)cm.at(99);
  } catch (const std::out_of_range &) {
    thrown = true;
  }
  CHECK(thrown);
  thrown = false;
  try {
    (void)m.at(99);
  } catch (const std::out_of_range &) {
    thrown = true;
  }
  CHECK(thrown);
}

void testSplitsAndHeight() {
  // Fill a node to 3 entries, then force the root split.
  Map m;
  for (int k : {1, 2, 3})
    m.put(k, "");
  CHECK(m.height() == 1 && m.isValid()); // one 4-node
  m.put(4, "");
  CHECK(m.height() == 2 && m.isValid()); // root split on next insert

  // Ascending insertion with full validation after every step.
  ds::TwoFourMap<int, int> a;
  for (int i = 0; i < 3000; ++i) {
    a.put(i, i);
    if (i < 300 || i % 50 == 0)
      CHECK(a.isValid());
  }
  // Descending insertion.
  ds::TwoFourMap<int, int> d;
  for (int i = 3000; i > 0; --i) {
    d.put(i, i);
    if (i > 2700 || i % 50 == 0)
      CHECK(d.isValid());
  }

  // Height bounds: 2^h - 1 <= n <= 4^h - 1.
  ds::TwoFourMap<int, int> big;
  const int n = 200000;
  for (int i = 0; i < n; ++i)
    big.put(i, i);
  const int h = big.height();
  CHECK(big.size() == static_cast<std::size_t>(n));
  CHECK(((1LL << h) - 1) <= n);
  CHECK(n <= ((1LL << (2 * h)) - 1));
  CHECK(big.isValid());
}

void testEraseCases() {
  Map m;
  for (int k : {50, 30, 70, 20, 40, 60, 80, 35, 45, 65})
    m.put(k, std::to_string(k));
  CHECK(m.isValid());

  CHECK(m.erase(20));
  CHECK(m.isValid());
  CHECK(m.erase(30));
  CHECK(m.isValid());
  CHECK(m.erase(50));
  CHECK(m.isValid());
  CHECK(!m.erase(50)); // already gone
  CHECK(m.size() == 7);

  std::vector<int> keys;
  for (auto kv : m)
    keys.push_back(kv.first);
  CHECK((keys == std::vector<int>{35, 40, 45, 60, 65, 70, 80}));
}

void testEraseOrdersWithValidation() {
  const int n = 2000;

  { // ascending erase
    ds::TwoFourMap<int, int> m;
    for (int i = 0; i < n; ++i)
      m.put(i, i);
    for (int i = 0; i < n; ++i) {
      CHECK(m.erase(i));
      if (i % 37 == 0)
        CHECK(m.isValid());
    }
    CHECK(m.empty() && m.isValid() && m.height() == 0);
  }
  { // descending erase
    ds::TwoFourMap<int, int> m;
    for (int i = 0; i < n; ++i)
      m.put(i, i);
    for (int i = n - 1; i >= 0; --i) {
      CHECK(m.erase(i));
      if (i % 37 == 0)
        CHECK(m.isValid());
    }
    CHECK(m.empty() && m.isValid());
  }
  { // shuffled erase, validating every step for the first few hundred
    std::vector<int> keys(n);
    std::iota(keys.begin(), keys.end(), 0);
    std::mt19937 rng(7);
    ds::TwoFourMap<int, int> m;
    std::shuffle(keys.begin(), keys.end(), rng);
    for (int k : keys)
      m.put(k, k);
    std::shuffle(keys.begin(), keys.end(), rng);
    int step = 0;
    for (int k : keys) {
      CHECK(m.erase(k));
      if (step++ < 400 || step % 50 == 0)
        CHECK(m.isValid());
    }
    CHECK(m.empty() && m.isValid());
  }
}

void testEraseWithAliasedKey() {
  // erase(it->first) passes a reference to a key stored inside the tree.
  Map m;
  for (int k = 1; k <= 50; ++k)
    m.put(k, "");
  for (int k = 1; k <= 50; k += 2) {
    auto it = m.find(k);
    CHECK(m.erase(it->first));
    CHECK(m.isValid());
  }
  CHECK(m.size() == 25);
  CHECK(!m.contains(1) && m.contains(2));
}

void testOrderedNavigation() {
  Map m;
  for (int k : {10, 20, 30, 40, 50})
    m.put(k, std::to_string(k));

  CHECK(m.firstEntry()->first == 10);
  CHECK(m.lastEntry()->first == 50);

  CHECK(m.ceilingEntry(30)->first == 30); // exact
  CHECK(m.ceilingEntry(31)->first == 40);
  CHECK(m.ceilingEntry(5)->first == 10);
  CHECK(m.ceilingEntry(51) == m.end());

  CHECK(m.floorEntry(30)->first == 30); // exact
  CHECK(m.floorEntry(29)->first == 20);
  CHECK(m.floorEntry(55)->first == 50);
  CHECK(m.floorEntry(9) == m.end());

  CHECK(m.higherEntry(30)->first == 40); // strict
  CHECK(m.higherEntry(50) == m.end());
  CHECK(m.higherEntry(1)->first == 10);

  CHECK(m.lowerEntry(30)->first == 20); // strict
  CHECK(m.lowerEntry(10) == m.end());
  CHECK(m.lowerEntry(100)->first == 50);

  const Map &cm = m;
  CHECK(cm.firstEntry()->first == 10);
  CHECK(cm.floorEntry(45)->first == 40);
}

void testSubMap() {
  Map m;
  for (int k = 1; k <= 10; ++k)
    m.put(k, std::to_string(k));

  auto r = m.subMap(3, 7); // [3, 7)
  std::vector<int> keys;
  for (auto it = r.first; it != r.second; ++it)
    keys.push_back(it->first);
  CHECK((keys == std::vector<int>{3, 4, 5, 6}));

  auto empty1 = m.subMap(7, 3);
  CHECK(empty1.first == empty1.second);
  auto empty2 = m.subMap(4, 4);
  CHECK(empty2.first == empty2.second);

  auto tail = m.subMap(9, 100);
  int cnt = 0;
  for (auto it = tail.first; it != tail.second; ++it)
    ++cnt;
  CHECK(cnt == 2);

  const Map &cm = m;
  auto cr = cm.subMap(1, 3);
  CHECK(std::distance(cr.first, cr.second) == 2);
}

void testIterators() {
  Map m;
  for (int k : {5, 1, 9, 3, 7, 2, 4, 6, 8, 10, 11, 12})
    m.put(k, "");

  std::vector<int> fwd;
  for (auto it = m.begin(); it != m.end(); ++it)
    fwd.push_back(it->first);
  std::vector<int> expect(12);
  std::iota(expect.begin(), expect.end(), 1);
  CHECK(fwd == expect);

  std::vector<int> bwd;
  for (auto it = m.end(); it != m.begin();) {
    --it;
    bwd.push_back(it->first);
  }
  std::reverse(expect.begin(), expect.end());
  CHECK(bwd == expect);

  // Mutating values through the proxy reference and structured bindings.
  for (auto kv : m)
    kv.second = std::to_string(kv.first * 2);
  CHECK(m.at(7) == "14");
  for (auto [k, v] : m)
    v += "!";
  CHECK(m.at(7) == "14!");

  // key()/value() accessors.
  auto it7 = m.find(7);
  CHECK(it7.key() == 7 && it7.value() == "14!");
  it7.value() = "seven";
  CHECK(m.at(7) == "seven");

  // iterator -> const_iterator conversion and mixed comparison.
  Map::const_iterator ci = m.begin();
  CHECK(ci == m.begin());
  CHECK(ci != m.end());

  // Post-increment / post-decrement.
  auto it = m.begin();
  auto old = it++;
  CHECK(old->first == 1 && it->first == 2);
  auto old2 = it--;
  CHECK(old2->first == 2 && it->first == 1);

  // std::distance works (bidirectional category).
  CHECK(std::distance(m.begin(), m.end()) == 12);
}

void testEraseByIterator() {
  Map m;
  for (int k = 1; k <= 40; ++k)
    m.put(k, "");

  auto it = m.find(4);
  it = m.erase(it);
  CHECK(it != m.end() && it->first == 5);
  CHECK(m.size() == 39 && m.isValid());

  auto last = m.lastEntry();
  CHECK(m.erase(last) == m.end());
  CHECK(!m.contains(40) && m.isValid());

  // Erase everything via iterator, always taking the returned iterator.
  std::size_t expected = m.size();
  for (auto i = m.begin(); i != m.end();) {
    i = m.erase(i);
    --expected;
    CHECK(m.size() == expected);
  }
  CHECK(m.empty() && m.isValid());
}

void testCopyMoveSwapClear() {
  Map a{{1, "a"}, {2, "b"}, {3, "c"}};
  CHECK(a.size() == 3 && a.isValid());

  Map big;
  for (int i = 0; i < 500; ++i)
    big.put(i, std::to_string(i));
  Map bigCopy(big); // multi-level deep copy
  CHECK(bigCopy.size() == 500 && bigCopy.isValid());
  big.erase(250);
  CHECK(bigCopy.contains(250) && !big.contains(250));

  Map b(a); // deep copy
  b.put(4, "d");
  b.erase(1);
  CHECK(a.size() == 3 && a.contains(1) && !a.contains(4));
  CHECK(b.size() == 3 && !b.contains(1) && b.contains(4));
  CHECK(a.isValid() && b.isValid());

  Map c(std::move(b)); // move
  CHECK(b.empty() && b.isValid());
  CHECK(c.size() == 3 && c.isValid());

  Map d;
  d = a; // copy assignment
  CHECK(d.size() == 3 && d.at(2) == "b");
  d = std::move(c); // move assignment
  CHECK(d.contains(4) && !d.contains(1));

  a.swap(d);
  CHECK(a.contains(4) && d.contains(1));
  swap(a, d);
  CHECK(a.contains(1) && d.contains(4));

  a.clear();
  CHECK(a.empty() && a.begin() == a.end() && a.isValid());
  a.put(1, "again"); // reusable after clear
  CHECK(a.size() == 1);
}

void testCustomComparatorAndStringKeys() {
  ds::TwoFourMap<int, int, std::greater<int>> desc;
  for (int k : {1, 5, 3, 9, 7, 2})
    desc.put(k, k);
  CHECK(desc.begin()->first == 9);
  CHECK(desc.lastEntry()->first == 1);
  CHECK(desc.ceilingEntry(4)->first == 3); // "ceiling" follows comparator order
  CHECK(desc.isValid());

  ds::TwoFourMap<std::string, int> words;
  words["pear"] = 1;
  words["apple"] = 2;
  words["fig"] = 3;
  words["kiwi"] = 4;
  words["banana"] = 5;
  CHECK(words.firstEntry()->first == "apple");
  CHECK(words.lastEntry()->first == "pear");
  CHECK(words.floorEntry("grape")->first == "fig");
  CHECK(words.isValid());
}

struct MoveOnly {
  std::unique_ptr<int> p;
  MoveOnly() = default;
  explicit MoveOnly(int v) : p(std::make_unique<int>(v)) {}
};

struct NoDefault {
  int v;
  explicit NoDefault(int x) : v(x) {}
};

void testMoveOnlyAndNonDefaultConstructible() {
  ds::TwoFourMap<int, MoveOnly> m;
  for (int i = 0; i < 100; ++i)
    m.put(i, MoveOnly(i * 10));
  CHECK(*m.at(2).p == 20);
  m.put(2, MoveOnly(33)); // overwrite by move
  CHECK(*m.at(2).p == 33);
  CHECK(m[1000].p == nullptr); // default-constructed
  for (int i = 0; i < 100; i += 2)
    m.erase(i);
  CHECK(m.isValid() && m.size() == 51);

  ds::TwoFourMap<int, NoDefault> nd; // no default ctor needed
  for (int i = 0; i < 50; ++i)
    nd.put(i, NoDefault(i));
  nd.erase(10);
  CHECK(nd.at(11).v == 11 && nd.isValid());
}

void testRandomizedAgainstStdMap() {
  std::mt19937 rng(12345);
  std::uniform_int_distribution<int> keyDist(0, 499);
  std::uniform_int_distribution<int> opDist(0, 99);

  ds::TwoFourMap<int, int> m;
  std::map<int, int> ref;

  for (int step = 0; step < 40000; ++step) {
    const int k = keyDist(rng);
    const int op = opDist(rng);

    if (op < 45) {
      m.put(k, step);
      ref[k] = step;
    } else if (op < 70) {
      CHECK(m.erase(k) == (ref.erase(k) > 0));
    } else if (op < 75) { // erase through an iterator
      auto it = m.find(k);
      auto rit = ref.find(k);
      CHECK((it == m.end()) == (rit == ref.end()));
      if (rit != ref.end()) {
        auto next = m.erase(it);
        auto rnext = ref.erase(rit);
        CHECK((next == m.end()) == (rnext == ref.end()));
        if (rnext != ref.end())
          CHECK(next->first == rnext->first);
      }
    } else {
      auto it = m.find(k);
      auto rit = ref.find(k);
      CHECK((it == m.end()) == (rit == ref.end()));
      if (rit != ref.end())
        CHECK(it->second == rit->second);

      auto c = m.ceilingEntry(k);
      auto rc = ref.lower_bound(k);
      CHECK((c == m.end()) == (rc == ref.end()));
      if (rc != ref.end())
        CHECK(c->first == rc->first);

      auto h = m.higherEntry(k);
      auto rh = ref.upper_bound(k);
      CHECK((h == m.end()) == (rh == ref.end()));
      if (rh != ref.end())
        CHECK(h->first == rh->first);

      auto f = m.floorEntry(k);
      auto rf = ref.upper_bound(k);
      if (rf == ref.begin()) {
        CHECK(f == m.end());
      } else {
        --rf;
        CHECK(f != m.end() && f->first == rf->first);
      }

      auto l = m.lowerEntry(k);
      auto rl = ref.lower_bound(k);
      if (rl == ref.begin()) {
        CHECK(l == m.end());
      } else {
        --rl;
        CHECK(l != m.end() && l->first == rl->first);
      }
    }

    CHECK(m.size() == ref.size());
    if (step % 100 == 0)
      CHECK(m.isValid());
  }

  CHECK(m.isValid());
  auto rit = ref.begin();
  for (auto kv : m) {
    CHECK(rit != ref.end() && rit->first == kv.first &&
          rit->second == kv.second);
    ++rit;
  }
  CHECK(rit == ref.end());
}

} // namespace TwoFourMap_Test
