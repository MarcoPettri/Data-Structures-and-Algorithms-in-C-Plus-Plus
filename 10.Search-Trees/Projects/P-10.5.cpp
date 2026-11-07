// File : P-10.5.cpp

/*
    P-10.5 Write a C++ class that implements all the functions of the ordered
   map ADT  using a (2, 4) tree.
*/

#include "two_four_map_test.cpp"

using namespace TwoFourMap_Test;

int main() {
  RUN(testEmpty);
  RUN(testPutFindOverwrite);
  RUN(testSubscriptAndAt);
  RUN(testSplitsAndHeight);
  RUN(testEraseCases);
  RUN(testEraseOrdersWithValidation);
  RUN(testEraseWithAliasedKey);
  RUN(testOrderedNavigation);
  RUN(testSubMap);
  RUN(testIterators);
  RUN(testEraseByIterator);
  RUN(testCopyMoveSwapClear);
  RUN(testCustomComparatorAndStringKeys);
  RUN(testMoveOnlyAndNonDefaultConstructible);
  RUN(testRandomizedAgainstStdMap);

  std::cout << "\n"
            << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
  return g_failures;
}