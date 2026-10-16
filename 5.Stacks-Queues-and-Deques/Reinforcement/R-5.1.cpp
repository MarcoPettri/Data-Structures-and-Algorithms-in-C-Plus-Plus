// File : R-5.1.cpp

/*
    R-5.1 Describe how to implement a capacity-limited stack, which uses the
    functions of a capacity-limited deque to perform the functions of the stack
    ADT in ways that do not throw exceptions when we attempt to perform a
    push on a full stack or a pop on an empty stack.
 */

#include "LimitedStack.hpp"
#include <iostream>

int main() {
  LimitedStack<int> s(2);

  std::cout << s.push(10) << '\n'; // 1 (ok)
  std::cout << s.push(20) << '\n'; // 1 (ok)
  std::cout << s.push(30) << '\n'; // 0 (full, no exception)

  if (auto t = s.top())
    std::cout << *t << '\n'; // 20

  s.pop();
  s.pop();
  std::cout << s.pop() << '\n'; // 0 (empty, no exception)

  auto v = s.top();
  std::cout << v.has_value() << '\n'; // 0

  return 0;
}