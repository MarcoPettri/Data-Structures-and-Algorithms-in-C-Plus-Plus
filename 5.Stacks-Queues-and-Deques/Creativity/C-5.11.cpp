// File : C-5.11.cpp

/*
    C-5.11 Show how to use a stack S and a queue Q to generate all possible
   subsets of an n-element set T nonrecursively.
*/

#include <iostream>
#include <queue>
#include <stack>
#include <vector>

template <typename T>
std::vector<std::vector<T>> generateSubsets(const std::vector<T> &T_set) {
  std::stack<T> S;
  std::queue<std::vector<T>> Q;

  // 1. Push all elements of the set into Stack S
  for (const auto &item : T_set) {
    S.push(item);
  }

  // 2. Initialize Queue Q with the empty subset
  Q.push({});

  // 3. Process each element from the stack
  while (!S.empty()) {
    T elem = S.top();
    S.pop();

    int queueSize = Q.size();

    // Take every current subset in the queue and branch into two choices:
    //  - Exclude the current element
    //  - Include the current element
    for (int i = 0; i < queueSize; ++i) {
      std::vector<T> curr = Q.front();
      Q.pop();

      // Choice A: Exclude 'elem' (push original subset back)
      Q.push(curr);

      // Choice B: Include 'elem' (create new subset with 'elem' added)
      std::vector<T> newSubset = curr;
      newSubset.push_back(elem);
      Q.push(newSubset);
    }
  }

  // 4. Extract all completed subsets from the queue into a result list
  std::vector<std::vector<T>> allSubsets;
  while (!Q.empty()) {
    allSubsets.push_back(Q.front());
    Q.pop();
  }

  return allSubsets;
}

int main() {
  std::vector<int> T = {1, 2, 3};
  auto subsets = generateSubsets(T);

  std::cout << "All possible subsets (" << subsets.size() << " total):\n";
  for (const auto &subset : subsets) {
    std::cout << "{ ";
    for (int val : subset) {
      std::cout << val << " ";
    }
    std::cout << "}\n";
  }

  return 0;
}