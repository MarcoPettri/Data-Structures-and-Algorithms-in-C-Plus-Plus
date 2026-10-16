// File : C-5.2.cpp

/*
    C-5.2 Suppose you have a stack S containing n elements and a queue Q that is
    initially empty. Describe how you can use Q to scan S to see if it contains
   a certain element x, with the additional constraint that your algorithm must
    return the elements back to S in their original order. You may not use
    an array or linked list—only S and Q and a constant number of reference
    variables.
*/

#include <iomanip>
#include <iostream>
#include <queue>
#include <stack>

template <typename T> bool containsAndRestore(std::stack<T> &S, const T &x);

int main() {
  std::stack<int> S;
  S.push(3);
  S.push(2);
  S.push(1);
  S.push(11); // top is 1
  std::cout << std::boolalpha;
  std::cout << "Contains 2: " << containsAndRestore(S, 2) << '\n';
  std::cout << "S.top(): " << S.top() << '\n';
  std::cout << "Contains 12: " << containsAndRestore(S, 12) << '\n';
  std::cout << "S.top(): " << S.top() << '\n';
  return 0;
}

template <typename T> bool containsAndRestore(std::stack<T> &S, const T &x) {
  std::queue<T> Q;
  bool found = false;

  // Pass 1: S -> Q (scan here)
  while (!S.empty()) {
    T e = S.top();
    S.pop();
    if (e == x)
      found = true;
    Q.push(e);
  }

  // Pass 2: Q -> S (S is now reversed)
  while (!Q.empty()) {
    S.push(Q.front());
    Q.pop();
  }

  // Pass 3: S -> Q
  while (!S.empty()) {
    Q.push(S.top());
    S.pop();
  }

  // Pass 4: Q -> S (original order restored)
  while (!Q.empty()) {
    S.push(Q.front());
    Q.pop();
  }

  return found;
}