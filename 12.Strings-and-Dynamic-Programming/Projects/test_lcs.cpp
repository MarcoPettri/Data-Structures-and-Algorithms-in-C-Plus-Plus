/**
 * @file test_lcs.cpp
 * @brief Self-contained unit tests (no external framework) for the LCS library.
 *
 * Build it as an executable linking alignment.cpp and fasta.cpp; the process
 * exit code is 0 when every check passes.
 */

#include <algorithm>
#include <cctype>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "bio/alignment.hpp"
#include "bio/fasta.hpp"
#include "bio/lcs.hpp"

namespace {

int g_checks = 0;
int g_failures = 0;

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

/// Matches must be strictly increasing in both indices and join equal elements.
template <typename A, typename B>
bool isValidMatching(const A &a, const B &b, const std::vector<bio::Match> &m) {
  for (std::size_t k = 0; k < m.size(); ++k) {
    if (m[k].first >= std::size(a) || m[k].second >= std::size(b))
      return false;
    if (!(a[m[k].first] == b[m[k].second]))
      return false;
    if (k > 0 &&
        (m[k].first <= m[k - 1].first || m[k].second <= m[k - 1].second))
      return false;
  }
  return true;
}

std::string randomDna(std::mt19937 &rng, std::size_t len) {
  static constexpr char kBases[] = {'A', 'C', 'G', 'T'};
  std::uniform_int_distribution<int> dist(0, 3);
  std::string s(len, 'A');
  for (auto &c : s)
    c = kBases[dist(rng)];
  return s;
}

std::string removeGaps(const std::string &s) {
  std::string out;
  std::copy_if(s.begin(), s.end(), std::back_inserter(out),
               [](char c) { return c != '-'; });
  return out;
}

void testClassicExample() {
  const std::string a = "ABCBDAB";
  const std::string b = "BDCABA";
  CHECK(bio::lcsLength(a, b) == 4);
  const auto h = bio::lcsMatches(a, b);
  const auto t = bio::lcsMatchesTable(a, b);
  CHECK(h.size() == 4);
  CHECK(t.size() == 4);
  CHECK(isValidMatching(a, b, h));
  CHECK(isValidMatching(a, b, t));
}

void testEdgeCases() {
  const std::string empty;
  CHECK(bio::lcsLength(empty, empty) == 0);
  CHECK(bio::lcsLength(empty, std::string("ACGT")) == 0);
  CHECK(bio::lcsLength(std::string("ACGT"), empty) == 0);
  CHECK(bio::lcsMatches(empty, std::string("ACGT")).empty());
  CHECK(bio::lcsMatchesTable(std::string("ACGT"), empty).empty());

  const std::string s = "ACGTACGT";
  CHECK(bio::lcsLength(s, s) == s.size());
  CHECK(bio::lcsMatches(s, s).size() == s.size());

  CHECK(bio::lcsLength(std::string("AAAA"), std::string("CCCC")) == 0);
  CHECK(bio::lcsMatches(std::string("AAAA"), std::string("CCCC")).empty());
}

void testRandomCrossCheck() {
  std::mt19937 rng(12345);
  std::uniform_int_distribution<std::size_t> len(0, 60);
  for (int iter = 0; iter < 300; ++iter) {
    const std::string a = randomDna(rng, len(rng));
    const std::string b = randomDna(rng, len(rng));
    const auto fast = bio::lcsMatches(a, b);
    const auto table = bio::lcsMatchesTable(a, b);
    const auto n = bio::lcsLength(a, b);
    CHECK(fast.size() == table.size());
    CHECK(fast.size() == n);
    CHECK(isValidMatching(a, b, fast));
    CHECK(isValidMatching(a, b, table));
  }
}

void testLargerRandom() {
  std::mt19937 rng(777);
  const std::string a = randomDna(rng, 2000);
  const std::string b = randomDna(rng, 1500);
  const auto m = bio::lcsMatches(a, b);
  CHECK(isValidMatching(a, b, m));
  CHECK(m.size() == bio::lcsLength(a, b));
  CHECK(bio::lcsLength(a, b) == bio::lcsLength(b, a));
}

void testSimilarSequencesWithMutations() {
  std::mt19937 rng(99);
  const std::string a = randomDna(rng, 3000);
  std::string b = a;
  b.erase(100, 5);
  b.insert(1500, "TTTTT");
  b[2500] = (b[2500] == 'A') ? 'C' : 'A';
  const auto m = bio::lcsMatches(a, b);
  CHECK(isValidMatching(a, b, m));
  CHECK(m.size() == bio::lcsLength(a, b));
  CHECK(m.size() >= 2985);
}

void testGenericContainersAndPredicate() {
  const std::vector<int> a{1, 2, 3, 4, 5, 6};
  const std::vector<int> b{2, 4, 6, 8};
  CHECK(bio::lcsLength(a, b) == 3);
  CHECK(isValidMatching(a, b, bio::lcsMatches(a, b)));

  // Mixed types with a custom, case-insensitive predicate.
  const std::string up = "GATTACA";
  const std::vector<char> low{'g', 'a', 't', 'a', 'c', 'a'};
  const auto ci = [](char x, char y) {
    return std::tolower(static_cast<unsigned char>(x)) ==
           std::tolower(static_cast<unsigned char>(y));
  };
  CHECK(bio::lcsLength(up, low, ci) == 6);
  CHECK(bio::lcsMatches(up, low, ci).size() == 6);
  CHECK(bio::lcsMatchesTable(up, low, ci).size() == 6);
}

void testTableLimit() {
  CHECK_THROWS(bio::lcsMatchesTable(std::string("ACGTA"), std::string("ACGTA"),
                                    std::equal_to<>{}, 10),
               std::length_error);
}

void testAlignment() {
  {
    const auto aln = bio::alignByLcs("ACGT", "ACGT");
    CHECK(aln.top == "ACGT");
    CHECK(aln.bottom == "ACGT");
    CHECK(aln.midline == "||||");
    CHECK(aln.identity() == 1.0);
  }
  {
    const auto aln = bio::alignByLcs("ACGT", "AGT");
    CHECK(aln.top == "ACGT");
    CHECK(aln.bottom == "A-GT");
    CHECK(aln.matches == 3);
    CHECK(aln.gaps == 1);
    CHECK(aln.mismatches == 0);
  }

  std::mt19937 rng(2024);
  for (int iter = 0; iter < 100; ++iter) {
    const std::string a = randomDna(rng, 1 + iter % 50);
    const std::string b = randomDna(rng, 1 + (iter * 7) % 50);
    const auto aln = bio::alignByLcs(a, b);
    CHECK(aln.top.size() == aln.bottom.size());
    CHECK(aln.top.size() == aln.midline.size());
    CHECK(removeGaps(aln.top) == a);
    CHECK(removeGaps(aln.bottom) == b);
    CHECK(aln.matches + aln.mismatches + aln.gaps == aln.length());
    CHECK(aln.matches == bio::lcsLength(a, b));
  }

  const std::vector<bio::Match> bad{{0, 1}};  // 'A' vs 'C'
  CHECK_THROWS(bio::buildAlignment("AG", "CC", bad), std::invalid_argument);

  const std::string a = "ACGTAC";
  CHECK(bio::lcsString(a, bio::lcsMatches(a, std::string("AGC"))) == "AGC");
}

void testWriteAlignment() {
  const auto aln = bio::alignByLcs("ACGTACGT", "ACGACGT");
  std::ostringstream os;
  bio::writeAlignment(os, aln, 4);
  const std::string text = os.str();
  CHECK(text.find('|') != std::string::npos);
  CHECK(text.find("A ") != std::string::npos);
  CHECK(text.find("B ") != std::string::npos);
}

void testFasta() {
  std::istringstream in(">NC_000001.1 Test virus, complete genome\r\n"
                        "acgt\r\n"
                        "ACGT N\r\n"
                        "\r\n"
                        ">second\n"
                        "TT-AA*\n");
  const auto recs = bio::readFasta(in);
  CHECK(recs.size() == 2);
  CHECK(recs[0].id == "NC_000001.1");
  CHECK(recs[0].description == "Test virus, complete genome");
  CHECK(recs[0].sequence == "ACGTACGTN");
  CHECK(recs[1].id == "second");
  CHECK(recs[1].description.empty());
  CHECK(recs[1].sequence == "TTAA");

  std::istringstream noHeader("ACGT\n");
  CHECK_THROWS(bio::readFasta(noHeader), std::runtime_error);

  std::istringstream badChar(">x\nAC1GT\n");
  CHECK_THROWS(bio::readFasta(badChar), std::runtime_error);

  CHECK_THROWS(bio::readFastaFile("/this/path/does/not/exist.fasta"),
               std::runtime_error);
}

}  // namespace

int main() {
  testClassicExample();
  testEdgeCases();
  testRandomCrossCheck();
  testLargerRandom();
  testSimilarSequencesWithMutations();
  testGenericContainersAndPredicate();
  testTableLimit();
  testAlignment();
  testWriteAlignment();
  testFasta();

  std::cout << (g_checks - g_failures) << '/' << g_checks << " checks passed\n";
  return g_failures == 0 ? 0 : 1;
}