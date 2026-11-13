/**
 * @file test_tries.cpp
 * @brief Self-contained unit tests (no external framework) for the trie classes.
 *
 * Link with standard_trie.cpp, compressed_trie.cpp and suffix_trie.cpp.
 * The exit code is 0 when every check passes.
 */

#include <algorithm>
#include <iostream>
#include <list>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "ds_tries/compressed_trie.hpp"
#include "ds_tries/standard_trie.hpp"
#include "ds_tries/suffix_trie.hpp"

namespace test_tries {

inline int g_checks = 0;
inline int g_failures = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    ++g_checks;                                                                \
    if (!(cond)) {                                                             \
      ++g_failures;                                                            \
      std::cerr << __FILE__ << ':' << __LINE__                                 \
                << ": CHECK failed: " #cond "\n";                              \
    }                                                                          \
  } while (false)

#define CHECK_THROWS(expr, ExType)                                             \
  do {                                                                         \
    ++g_checks;                                                                \
    bool caught = false;                                                       \
    try {                                                                      \
      (void)(expr);                                                            \
    } catch (const ExType &) {                                                 \
      caught = true;                                                           \
    } catch (...) {                                                            \
    }                                                                          \
    if (!caught) {                                                             \
      ++g_failures;                                                            \
      std::cerr << __FILE__ << ':' << __LINE__ << ": expected " #ExType "\n";  \
    }                                                                          \
  } while (false)

const std::vector<std::string> kTextbookWords{"bear", "bell", "bid",   "bull",
                                              "buy",  "sell", "stock", "stop"};

inline std::string randomString(std::mt19937 &rng, std::size_t maxLen,
                                int alphabet) {
  std::uniform_int_distribution<std::size_t> len(0, maxLen);
  std::uniform_int_distribution<int> ch(0, alphabet - 1);
  std::string s(len(rng), 'a');
  for (auto &c : s) {
    c = static_cast<char>('a' + ch(rng));
  }
  return s;
}

inline std::vector<std::size_t> naiveFindAll(const std::string &text,
                                             const std::string &pat) {
  std::vector<std::size_t> out;
  for (std::size_t i = 0; i + pat.size() <= text.size(); ++i) {
    if (text.compare(i, pat.size(), pat) == 0) {
      out.push_back(i);
    }
  }
  return out;
}

// ---------------------------------------------------------------- StandardTrie

inline void testStandardTrieBasics() {
  const ds::StandardTrie trie(kTextbookWords);
  for (const auto &w : kTextbookWords) {
    CHECK(trie.contains(w));
  }
  CHECK(!trie.contains("be"));      // prefix of a word, not a word
  CHECK(!trie.contains("bears"));   // extension of a word
  CHECK(!trie.contains("xyz"));
  CHECK(!trie.contains(""));
  CHECK(trie.hasPrefix("be"));
  CHECK(trie.hasPrefix("sto"));
  CHECK(!trie.hasPrefix("sx"));
  CHECK(trie.size() == 8);
  CHECK(trie.nodeCount() == 22);
}

inline void testStandardTrieConstructorsAndEdgeCases() {
  ds::StandardTrie empty;
  CHECK(empty.empty());
  CHECK(!empty.contains(""));
  CHECK(!empty.hasPrefix(""));

  ds::StandardTrie init{"a", "ab", "a"};  // duplicate ignored
  CHECK(init.size() == 2);
  CHECK(init.contains("a") && init.contains("ab"));

  const std::list<std::string> words{"x", "xy", "", "xyz"};
  const ds::StandardTrie fromRange(words.begin(), words.end());
  CHECK(fromRange.size() == 4);
  CHECK(fromRange.contains(""));  // empty string stored at the root
  CHECK(fromRange.contains("xyz"));

  ds::StandardTrie t;
  CHECK_THROWS(t.insert("caf\xC3\xA9"), std::invalid_argument);
  CHECK(t.size() == 0 &&
        t.nodeCount() == 1);  // unchanged after the failed insert
  CHECK_THROWS(ds::StandardTrie(std::vector<std::string>{"ok", "\x80"}),
               std::invalid_argument);
  CHECK(!t.contains("\xC3\xA9"));  // lookups with non-ASCII simply miss
}

// -------------------------------------------------------------- CompressedTrie

inline void testCompressedTrieBasics() {
  const ds::CompressedTrie trie(kTextbookWords);
  for (const auto &w : kTextbookWords) {
    CHECK(trie.contains(w));
  }
  CHECK(!trie.contains("be"));
  CHECK(!trie.contains("sto"));
  CHECK(!trie.contains("stocks"));
  CHECK(!trie.contains("bel"));
  CHECK(trie.hasPrefix("be"));
  CHECK(trie.hasPrefix("stoc"));  // ends in the middle of the "ck" edge
  CHECK(!trie.hasPrefix("stx"));
  CHECK(trie.size() == 8);
  CHECK(trie.nodeCount() == 14);  // matches the textbook figure
}

inline void testCompressedTrieSplitting() {
  ds::CompressedTrie t;
  t.insert("abcdef");
  CHECK(t.nodeCount() == 2);
  t.insert("abc");  // word ends in the middle of an edge
  CHECK(t.nodeCount() == 3);
  CHECK(t.contains("abc") && t.contains("abcdef"));
  t.insert("abxyz");  // diverges in the middle of an edge
  CHECK(t.contains("abxyz"));
  CHECK(!t.contains("ab"));
  t.insert("ab");
  CHECK(t.contains("ab"));
  t.insert("");
  CHECK(t.contains(""));
  t.insert("abc");  // duplicate
  CHECK(t.size() == 5);

  CHECK_THROWS(t.insert("\xFF"), std::invalid_argument);
  CHECK(t.size() == 5);

  ds::CompressedTrie init{"romane", "romanus", "romulus",
                          "rubens", "ruber",   "rubicon"};
  CHECK(init.contains("romulus") && init.contains("rubicon"));
  CHECK(!init.contains("rom"));
  const std::vector<std::string> v{"p", "pq"};
  const ds::CompressedTrie fromRange(v.begin(), v.end());
  CHECK(fromRange.contains("pq") && fromRange.size() == 2);
}

inline void testRandomizedAgainstStdSet() {
  std::mt19937 rng(2026);
  for (int round = 0; round < 200; ++round) {
    std::vector<std::string> words;
    const int count = static_cast<int>(rng() % 25);
    for (int i = 0; i < count; ++i) {
      words.push_back(randomString(rng, 8, 3));
    }
    const std::set<std::string> truth(words.begin(), words.end());

    const ds::StandardTrie st(words);
    const ds::CompressedTrie ct(words);
    CHECK(st.size() == truth.size());
    CHECK(ct.size() == truth.size());
    CHECK(ct.nodeCount() <= st.nodeCount());

    for (int q = 0; q < 60; ++q) {
      const std::string probe = randomString(rng, 9, 3);
      const bool expected = truth.count(probe) != 0;
      CHECK(st.contains(probe) == expected);
      CHECK(ct.contains(probe) == expected);

      const bool prefixExpected =
          std::any_of(truth.begin(), truth.end(), [&](const std::string &w) {
            return w.compare(0, probe.size(), probe) == 0 &&
                   w.size() >= probe.size();
          });
      CHECK(st.hasPrefix(probe) == prefixExpected);
      CHECK(ct.hasPrefix(probe) == prefixExpected);
    }
  }
}

// ------------------------------------------------------------------ SuffixTrie

inline void testSuffixTrieBanana() {
  const ds::SuffixTrie trie("banana");
  CHECK(trie.text() == "banana");
  CHECK(trie.textSize() == 6);
  CHECK(trie.contains("nan"));
  CHECK(trie.contains("banana"));
  CHECK(trie.contains("a"));
  CHECK(!trie.contains("nab"));
  CHECK(!trie.contains("bananas"));
  CHECK((trie.findAll("ana") == std::vector<std::size_t>{1, 3}));
  CHECK((trie.findAll("a") == std::vector<std::size_t>{1, 3, 5}));
  CHECK(trie.count("a") == 3);
  CHECK(trie.findFirst("na") == std::optional<std::size_t>(2));
  CHECK(!trie.findFirst("x").has_value());
  CHECK(trie.findAll("x").empty());
  CHECK(trie.count("") == 7);  // every position 0..n
  CHECK(trie.contains(""));
}

inline void testSuffixTrieEdgeCases() {
  const ds::SuffixTrie empty("");
  CHECK(empty.contains(""));
  CHECK(!empty.contains("a"));
  CHECK(empty.findAll("").size() == 1);

  const ds::SuffixTrie single("a");
  CHECK(single.contains("a"));
  CHECK((single.findAll("a") == std::vector<std::size_t>{0}));

  const ds::SuffixTrie repeat("aaaaaa");
  CHECK((repeat.findAll("aaa") == std::vector<std::size_t>{0, 1, 2, 3}));
  CHECK(repeat.count("aaaaaa") == 1);
  CHECK(!repeat.contains("aaaaaaa"));

  const ds::SuffixTrie nul(std::string("a\0b", 3));
  CHECK(nul.contains(std::string("\0b", 2)));
  CHECK(!nul.contains("\x80"));  // sentinel must never be matchable
  CHECK(!nul.contains("caf\xC3\xA9"));

  CHECK_THROWS(ds::SuffixTrie("na\xC3\xAFve"), std::invalid_argument);

  std::vector<std::size_t> seen;
  ds::SuffixTrie("abab").forEachOccurrence(
      "ab", [&seen](std::size_t i) { seen.push_back(i); });
  std::sort(seen.begin(), seen.end());
  CHECK((seen == std::vector<std::size_t>{0, 2}));
}

inline void testSuffixTrieRandomized() {
  std::mt19937 rng(424242);
  for (int round = 0; round < 300; ++round) {
    const std::string text = randomString(rng, 40, round % 2 == 0 ? 2 : 4);
    const ds::SuffixTrie trie(text);
    CHECK(trie.nodeCount() <= 2 * (text.size() + 1) + 1);

    // Every substring must be found at exactly the naive positions.
    for (std::size_t i = 0; i < text.size(); ++i) {
      for (std::size_t len = 1; i + len <= text.size() && len <= 6; ++len) {
        const std::string pat = text.substr(i, len);
        CHECK(trie.findAll(pat) == naiveFindAll(text, pat));
      }
    }
    // Random probes, mostly absent.
    for (int q = 0; q < 30; ++q) {
      const std::string pat = randomString(rng, 6, 4);
      if (pat.empty())
        continue;
      const auto expected = naiveFindAll(text, pat);
      CHECK(trie.contains(pat) == !expected.empty());
      CHECK(trie.count(pat) == expected.size());
      CHECK(trie.findAll(pat) == expected);
      if (!expected.empty()) {
        CHECK(trie.findFirst(pat) ==
              std::optional<std::size_t>(expected.front()));
      }
    }
  }
}

inline void testSuffixTrieLarger() {
  std::mt19937 rng(7);
  std::string text(50000, 'A');
  const char bases[] = {'A', 'C', 'G', 'T'};
  for (auto &c : text)
    c = bases[rng() % 4];
  const ds::SuffixTrie trie(text);
  CHECK(trie.nodeCount() <= 2 * (text.size() + 1) + 1);
  for (int q = 0; q < 200; ++q) {
    const std::size_t at = rng() % (text.size() - 12);
    const std::string pat = text.substr(at, 3 + rng() % 9);
    const auto hits = trie.findAll(pat);
    CHECK(std::binary_search(hits.begin(), hits.end(), at));
    for (const auto h : hits) {
      CHECK(text.compare(h, pat.size(), pat) == 0);
    }
  }
}

}  // namespace test_tries
