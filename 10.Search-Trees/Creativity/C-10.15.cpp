// File : C-10.15.cpp

/*
    C-10.15 Let T and U be (2, 4) trees storing n and m entries, respectively,
   such that all the entries in T have keys less than the keys of all the
   entries in U . Describe an O(log n + log m) time method for joining T and U
   into a single tree that stores all the entries in T and U
*/

#include "two_four_tree_join.hpp"
#include <cassert>
#include <iostream>

void runTests() {
  std::cout << "Running (2,4) Tree Join tests...\n";

  using TreeJoin = TwoFourTreeJoin<int, std::string>;

  auto T = std::make_shared<TreeJoin::Node>(true);
  T->entries.push_back({10, "Ten"});
  T->entries.push_back({20, "Twenty"});

  auto U = std::make_shared<TreeJoin::Node>(true);
  U->entries.push_back({30, "Thirty"});
  U->entries.push_back({40, "Forty"});

  // Equal height join test
  auto joinedRoot = TreeJoin::join(T, U);
  assert(joinedRoot != nullptr);
  assert(joinedRoot->entries.size() == 1);
  assert(joinedRoot->entries[0].key == 30);
  assert(joinedRoot->children.size() == 2);

  std::cout << "All (2,4) Tree Join tests passed successfully!\n";
}

int main() {
  runTests();
  return 0;
}