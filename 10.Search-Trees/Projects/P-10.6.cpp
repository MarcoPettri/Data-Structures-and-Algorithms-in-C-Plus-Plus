// File : P-10.6.cpp

/*
    P-10.6 Write a C++ class that implements all the functions of the ordered
   map ADT (see Section 9.3) using a red-black tree.
*/

#include "red_black_map_test.cpp"
using namespace RedBlackMap_Test;

int main() {
  RUN(testEmpty);
  RUN(testPutFindOverwrite);
  RUN(testSubscriptAndAt);
  RUN(testInsertFixupCases);
  RUN(testSequentialHeightBound);
  RUN(testEraseCases);
  RUN(testEraseExhaustiveSmall);
  RUN(testEraseOrdersWithValidation);
  RUN(testOrderedNavigation);
  RUN(testSubMap);
  RUN(testIterators);
  RUN(testIteratorStability);
  RUN(testEraseByIterator);
  RUN(testEraseWithAliasedKey);
  RUN(testCopyMoveSwapClear);
  RUN(testCustomComparatorAndStringKeys);
  RUN(testMoveOnlyAndNonDefaultConstructible);
  RUN(testRandomizedAgainstStdMap);

  std::cout << "\n"
            << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
  return g_failures;
}