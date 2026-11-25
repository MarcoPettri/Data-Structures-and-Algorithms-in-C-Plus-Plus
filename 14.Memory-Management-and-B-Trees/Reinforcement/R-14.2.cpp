// File : R-14.2.cpp

/*
    Implement, in detail, add and remove algorithms for an (a, b) tree.
*/

#include "ab_tree_test.cpp"

using namespace ABTree_Test;

int main() {
  RUN(testEmpty);
  RUN(testSingleInsertAndDuplicate);
  RUN(testInsertNoSplit);
  RUN(testFirstRootSplit);
  RUN(testSequentialAscendingInsert);
  RUN(testSequentialDescendingInsert);
  RUN(testEraseFromSingleElementTree);
  RUN(testEraseAbsentKey);
  RUN(testEraseCausingMerge);
  RUN(testEraseInternalNodeKey);
  RUN(testEraseAllSequential);
  RUN(testEraseAllReverse);
  RUN(testEraseShuffled);
  RUN(testTree35Correctness);
  RUN(testCopyAndSwap);
  RUN(testHeightBound);
  RUN(testRandomizedAgainstStdSet);
  RUN(testPerformanceSanity);

  std::cout << '\n'
            << (g_checks - g_failures) << '/' << g_checks << " checks passed\n";
  return g_failures;
}
