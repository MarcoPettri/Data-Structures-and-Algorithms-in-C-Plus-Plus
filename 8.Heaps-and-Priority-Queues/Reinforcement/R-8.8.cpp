// File : R-8.8.cpp

/*
    R-8.8 Although it is correct to use a “reverse” comparator with our priority
    queue ADT so that we retrieve and remove an element with the maxi-
    mum key each time, it is confusing to have an element with the maximum
    key returned by a function named “removeMin.” Write a short adapter
    class that can take any priority queue P and an associated comparator C
    and implement a priority queue that concentrates on the element with the
    maximum key, using functions with names like removeMax.
    (Hint: Define a new comparator C′ in terms of C.)
*/

#include "MaxPriorityQueueAdapter.hpp"
#include <cassert>
#include <functional>
#include <iostream>
#include <vector>

void runTests() {
  std::cout << "Running MaxPriorityQueueAdapter tests...\n";

  // Instantiate using std::vector as the container and std::less as the
  // comparator C
  MaxPriorityQueueAdapter<int, std::vector<int>, std::less<int>> maxPq;

  assert(maxPq.empty());
  assert(maxPq.size() == 0);

  // Insert elements
  maxPq.insert(10);
  maxPq.insert(5);
  maxPq.insert(20);
  maxPq.insert(15);

  assert(!maxPq.empty());
  assert(maxPq.size() == 4);

  // getMax should return the maximum element (20)
  assert(maxPq.getMax() == 20);
  std::cout << "getMax() passed: " << maxPq.getMax() << "\n";

  // Remove max and check next maximum (15)
  maxPq.removeMax();
  assert(maxPq.getMax() == 15);
  assert(maxPq.size() == 3);

  // Insert another larger element
  maxPq.insert(25);
  assert(maxPq.getMax() == 25);

  // Drain the queue
  maxPq.removeMax(); // removes 25
  maxPq.removeMax(); // removes 15
  maxPq.removeMax(); // removes 10
  maxPq.removeMax(); // removes 5

  assert(maxPq.empty());
  assert(maxPq.size() == 0);

  std::cout << "All tests passed successfully!\n";
}

int main() {
  runTests();
  return 0;
}