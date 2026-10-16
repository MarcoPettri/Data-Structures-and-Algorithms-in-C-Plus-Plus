// File : R-5.2.cpp

/*
    R-5.2 Describe how to implement a capacity-limited queue, which uses the
    functions of a capacity-limited deque to perform the functions of the queue
    ADT in ways that do not throw exceptions when we attempt to perform a
    enqueue on a full queue or a dequeue on an empty queue.
*/

#include "LimitedQueue.hpp"
#include <iostream>

int main() {
  LimitedQueue<int> q(2);

  std::cout << q.enqueue(10) << '\n'; // 1 (ok)
  std::cout << q.enqueue(20) << '\n'; // 1 (ok)
  std::cout << q.enqueue(30) << '\n'; // 0 (full, no exception)

  if (auto f = q.front())
    std::cout << *f << '\n'; // 10 (FIFO)

  (void)q.dequeue();                // removes 10
  (void)q.dequeue();                // removes 20
  std::cout << q.dequeue() << '\n'; // 0 (empty, no exception)

  std::cout << q.front().has_value() << '\n'; // 0

  return 0;
}