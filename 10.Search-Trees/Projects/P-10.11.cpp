// File : P-10.11.cpp

/*
    P-10.11 Implement a binary search tree data structure so that it can support
   the dictionary ADT, where different entries can have equal keys. In addi-
    tion, implement the functions entrySetPreorder(), entrySetInorder(), and
    entrySetPostorder(), which produce an iterable collection of the entries in
    the binary search tree in the same order they would respectively be visited
    in a preorder, inorder, and postorder traversal of the tree.
*/

#include "BSTMultiMap.hpp"

#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <vector>

namespace {

// =============================================================================
// Minimal test harness
// =============================================================================

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

using Map = ds::BSTMultiMap<int, std::string>;

// =============================================================================
// Tests
// =============================================================================

void testEmpty() {
  Map m;
  CHECK(m.empty());
  CHECK(m.size() == 0);
  CHECK(m.height() == 0);
  CHECK(m.begin() == m.end());
  CHECK(m.find(1) == m.end());
  CHECK(!m.contains(1));
  CHECK(!m.erase(1));
  CHECK(m.entrySetPreorder().empty());
  CHECK(m.entrySetInorder().empty());
  CHECK(m.entrySetPostorder().empty());
  CHECK(m.isValid());
}

void testInsertAndFind() {
  Map m;
  auto it1 = m.insert(10, "ten");
  CHECK(it1->first == 10);
  CHECK(it1->second == "ten");
  CHECK(m.size() == 1);
  CHECK(m.contains(10));
  CHECK(!m.contains(5));

  // Insert same key again – must succeed (dictionary with duplicate keys).
  auto it2 = m.insert(10, "TEN");
  CHECK(it2->first == 10);
  CHECK(it2->second == "TEN");
  CHECK(m.size() == 2);

  // find() returns the leftmost (inorder-first) occurrence.
  auto found = m.find(10);
  CHECK(found != m.end() && found->first == 10);
  CHECK(m.isValid());
}

void testMultipleDuplicates() {
  Map m;
  // Insert several duplicates.
  for (int i = 0; i < 5; ++i)
    m.insert(42, std::to_string(i));
  CHECK(m.size() == 5);

  // findAll must return exactly 5 entries.
  auto all = m.findAll(42);
  CHECK(all.size() == 5);
  for (auto *p : all)
    CHECK(p->first == 42);

  CHECK(m.isValid());
}

void testInorderIsSorted() {
  Map m;
  for (int k : {5, 3, 7, 3, 5, 1, 9, 5})
    m.insert(k, "");

  // Inorder traversal via iterator must be non-decreasing.
  int prev = -1;
  for (const auto &kv : m) {
    CHECK(kv.first >= prev);
    prev = kv.first;
  }

  // entrySetInorder must match iteration order.
  auto inorder = m.entrySetInorder();
  CHECK(inorder.size() == m.size());
  std::size_t idx = 0;
  for (const auto &kv : m) {
    CHECK(inorder[idx]->first == kv.first);
    ++idx;
  }
  CHECK(m.isValid());
}

void testEntrySetPreorder() {
  // Build a known tree:
  //       5
  //      / \
  //     3   7
  //    /
  //   1
  Map m;
  m.insert(5, "root");
  m.insert(3, "left");
  m.insert(7, "right");
  m.insert(1, "ll");

  auto pre = m.entrySetPreorder();
  CHECK(pre.size() == 4);
  // Preorder: root(5), left-subtree(3,1), right-subtree(7)
  CHECK(pre[0]->first == 5);
  CHECK(pre[1]->first == 3);
  CHECK(pre[2]->first == 1);
  CHECK(pre[3]->first == 7);
  CHECK(m.isValid());
}

void testEntrySetPostorder() {
  //       5
  //      / \
  //     3   7
  //    /
  //   1
  Map m;
  m.insert(5, "root");
  m.insert(3, "left");
  m.insert(7, "right");
  m.insert(1, "ll");

  auto post = m.entrySetPostorder();
  CHECK(post.size() == 4);
  // Postorder: 1, 3, 7, 5
  CHECK(post[0]->first == 1);
  CHECK(post[1]->first == 3);
  CHECK(post[2]->first == 7);
  CHECK(post[3]->first == 5);
  CHECK(m.isValid());
}

void testEntrySetSizesMatch() {
  Map m;
  for (int k : {4, 2, 6, 2, 4, 1, 3, 5, 7})
    m.insert(k, "");

  const std::size_t n = m.size();
  CHECK(m.entrySetPreorder().size() == n);
  CHECK(m.entrySetInorder().size() == n);
  CHECK(m.entrySetPostorder().size() == n);
  CHECK(m.isValid());
}

void testEraseByKey() {
  Map m;
  m.insert(10, "a");
  m.insert(10, "b");
  m.insert(20, "c");

  // Erase one duplicate of key 10.
  CHECK(m.erase(10));
  CHECK(m.size() == 2);
  CHECK(m.contains(10)); // second duplicate still present

  CHECK(m.erase(10));
  CHECK(m.size() == 1);
  CHECK(!m.contains(10));

  CHECK(!m.erase(10)); // already gone
  CHECK(m.isValid());
}

void testEraseAll() {
  Map m;
  for (int i = 0; i < 4; ++i)
    m.insert(5, "");
  m.insert(3, "");
  m.insert(7, "");

  CHECK(m.eraseAll(5) == 4);
  CHECK(m.size() == 2);
  CHECK(!m.contains(5));
  CHECK(m.contains(3) && m.contains(7));
  CHECK(m.eraseAll(99) == 0); // no-op
  CHECK(m.isValid());
}

void testEraseByIterator() {
  Map m;
  for (int k : {1, 2, 3, 4, 5})
    m.insert(k, "");

  // Erase key 3 via iterator; returned iterator should point to 4.
  auto it = m.find(3);
  it = m.erase(it);
  CHECK(it != m.end() && it->first == 4);
  CHECK(m.size() == 4 && m.isValid());

  // Erase everything via iterator.
  for (auto i = m.begin(); i != m.end();)
    i = m.erase(i);
  CHECK(m.empty() && m.isValid());
}

void testCopyMoveSwap() {
  Map a;
  a.insert(1, "one");
  a.insert(2, "two");
  a.insert(2, "TWO"); // duplicate

  Map b(a); // deep copy
  b.insert(3, "three");
  CHECK(a.size() == 3 && b.size() == 4);
  CHECK(a.isValid() && b.isValid());

  Map c(std::move(b));
  CHECK(b.empty() && b.isValid());
  CHECK(c.size() == 4 && c.isValid());

  Map d;
  d = a;
  CHECK(d.size() == 3 && d.isValid());

  d = std::move(c);
  CHECK(d.size() == 4 && d.isValid());

  a.swap(d);
  CHECK(a.size() == 4 && d.size() == 3);
  swap(a, d);
  CHECK(a.size() == 3 && d.size() == 4);

  a.clear();
  CHECK(a.empty() && a.isValid());
  a.insert(99, "reuse");
  CHECK(a.size() == 1);
}

void testInitializerList() {
  Map m({{1, "a"}, {1, "b"}, {2, "c"}, {1, "d"}});
  CHECK(m.size() == 4);
  CHECK(m.findAll(1).size() == 3);
  CHECK(m.findAll(2).size() == 1);
  CHECK(m.isValid());
}

void testCustomComparator() {
  // Reverse order: keys stored descending.
  ds::BSTMultiMap<int, int, std::greater<int>> desc;
  for (int k : {3, 1, 5, 3})
    desc.insert(k, k);
  CHECK(desc.size() == 4);
  // Inorder (following comparator) should be non-increasing.
  int prev = std::numeric_limits<int>::max();
  for (const auto &kv : desc) {
    CHECK(kv.first <= prev);
    prev = kv.first;
  }
  CHECK(desc.isValid());
}

void testStringKeys() {
  ds::BSTMultiMap<std::string, int> words;
  words.insert("banana", 1);
  words.insert("apple", 2);
  words.insert("apple", 3);
  words.insert("cherry", 4);

  // Inorder: apple, apple, banana, cherry.
  auto io = words.entrySetInorder();
  CHECK(io.size() == 4);
  CHECK(io[0]->first == "apple");
  CHECK(io[1]->first == "apple");
  CHECK(io[2]->first == "banana");
  CHECK(io[3]->first == "cherry");
  CHECK(words.isValid());
}

void testTraversalWithDuplicates() {
  // Insert keys {2, 1, 3, 2} in that order so the second 2 goes right.
  Map m;
  m.insert(2, "first-2");
  m.insert(1, "one");
  m.insert(3, "three");
  m.insert(2, "second-2"); // duplicate of 2 → placed right of existing 2

  // Preorder: root(2,"first-2"), 1, 3, second-2 (right child of 3... actually
  // the second 2 goes to 2's right because 2 >= 2; then 3's left).
  // The exact shape depends on insertion, so we just verify sizes and that
  // inorder is non-decreasing.
  CHECK(m.size() == 4);
  auto pre = m.entrySetPreorder();
  auto in = m.entrySetInorder();
  auto post = m.entrySetPostorder();
  CHECK(pre.size() == 4);
  CHECK(in.size() == 4);
  CHECK(post.size() == 4);

  for (std::size_t i = 1; i < in.size(); ++i)
    CHECK(in[i]->first >= in[i - 1]->first);

  // findAll for key 2 must return exactly 2 entries.
  CHECK(m.findAll(2).size() == 2);
  CHECK(m.isValid());
}

void testRandomizedAgainstStdMultimap() {
  std::mt19937 rng(98765);
  std::uniform_int_distribution<int> keyDist(0, 49);
  std::uniform_int_distribution<int> opDist(0, 99);

  ds::BSTMultiMap<int, int> m;
  std::multimap<int, int> ref;

  for (int step = 0; step < 5000; ++step) {
    const int k = keyDist(rng);
    const int op = opDist(rng);

    if (op < 40) {
      m.insert(k, step);
      ref.insert({k, step});
    } else if (op < 60) {
      // Erase one copy of key k from both containers.
      auto refIt = ref.find(k);
      if (refIt != ref.end())
        ref.erase(refIt);
      m.erase(k);
      // Containment must agree after erase.
      CHECK(m.contains(k) == (ref.count(k) > 0));
    } else {
      // Sizes must match throughout.
    }
    CHECK(m.size() == ref.size());

    if (step % 500 == 0)
      CHECK(m.isValid());
  }

  // Final inorder check: our inorder sequence of keys must equal the
  // multimap's key sequence (non-decreasing, same multiset of keys).
  std::vector<int> ourKeys, refKeys;
  for (const auto &kv : m)
    ourKeys.push_back(kv.first);
  for (const auto &kv : ref)
    refKeys.push_back(kv.first);
  CHECK(ourKeys == refKeys);
  CHECK(m.isValid());
}

} // namespace

int main() {
  RUN(testEmpty);
  RUN(testInsertAndFind);
  RUN(testMultipleDuplicates);
  RUN(testInorderIsSorted);
  RUN(testEntrySetPreorder);
  RUN(testEntrySetPostorder);
  RUN(testEntrySetSizesMatch);
  RUN(testEraseByKey);
  RUN(testEraseAll);
  RUN(testEraseByIterator);
  RUN(testCopyMoveSwap);
  RUN(testInitializerList);
  RUN(testCustomComparator);
  RUN(testStringKeys);
  RUN(testTraversalWithDuplicates);
  RUN(testRandomizedAgainstStdMultimap);

  std::cout << '\n'
            << (g_checks - g_failures) << '/' << g_checks << " checks passed\n";
  return g_failures;
}
