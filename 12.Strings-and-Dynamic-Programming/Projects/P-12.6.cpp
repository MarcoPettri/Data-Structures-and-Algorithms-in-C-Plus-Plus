// File : P-12.6.cpp

/*
    P-12.6 Create a class that implements a standard trie for a set of ASCII strings.
    The class should have a constructor that takes as an argument a list of
    strings, and the class should have a method that tests whether a given
    string is stored in the trie.

    P-12.7 Create a class that implements a compressed trie for a set of ASCII strings.
    The class should have a constructor that takes as an argument a list of
    strings, and the class should have a function that tests whether a given
    string is stored in the trie.

    P-12.8 Create a class that implements a prefix trie for an ASCII string. The class
    should have a constructor that takes as an argument a string and a function
    for pattern matching on the string.
*/

#include "test_tries.hpp"

using namespace test_tries;

int main() {
  testStandardTrieBasics();
  testStandardTrieConstructorsAndEdgeCases();
  testCompressedTrieBasics();
  testCompressedTrieSplitting();
  testRandomizedAgainstStdSet();
  testSuffixTrieBanana();
  testSuffixTrieEdgeCases();
  testSuffixTrieRandomized();
  testSuffixTrieLarger();

  std::cout << (g_checks - g_failures) << '/' << g_checks << " checks passed\n";
  return g_failures == 0 ? 0 : 1;
}
