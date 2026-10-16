// File : R-5.6.cpp

/*
    R-5.6 Give a recursive function for removing all the elements in a stack.
*/

#include <iostream>
#include <stack>

template <typename T> void clearStack(std::stack<T> &s) {
  // Base case: if the stack is already empty, stop recursion
  if (s.empty()) {
    return;
  }

  // Remove the top element
  s.pop();

  // Recursive call for the remaining stack
  clearStack(s);
}

int main() {
  std::stack<int> s;
  s.push(10);
  s.push(20);
  s.push(30);

  std::cout << "Stack size before clearing: " << s.size() << "\n";

  clearStack(s);

  std::cout << "Stack size after clearing: " << s.size() << "\n";

  return 0;
}