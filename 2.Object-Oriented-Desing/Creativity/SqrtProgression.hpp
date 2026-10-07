// File : SqrtProgression.hpp

#ifndef SqrtProgression_H
#define SqrtProgression_H

#include <cmath>
#include <iostream>
#include <stdexcept>
class Progression { // a generic progression
public:
  Progression(double f = 0) // constructor
      : first(f), cur(f) {}
  virtual ~Progression() = default; // destructor
  void printProgression(int n);     // print the first n values
protected:
  virtual double firstValue(); // reset
  virtual double nextValue();  // advance
protected:
  double first; // first value
  double cur;   // current value
};

class SqrtProg : public Progression {
public:
  SqrtProg(double f = 65536)
      : Progression(f) {} // default and parametric in one

protected:
  double firstValue() override {
    cur = first;
    return cur;
  }
  double nextValue() override {
    cur = std::sqrt(cur);
    return cur;
  }
};

double Progression::firstValue() {
  cur = first; // reset to first value
  return cur;
}

double Progression::nextValue() {
  return ++cur; // advance to next value
}

void Progression::printProgression(int n) {
  if (n <= 0) {
    throw std::invalid_argument("Number of values must be positive");
  }

  // Print the first value
  std::cout << firstValue();

  // Print the remaining n-1 values
  for (int i = 2; i <= n; ++i) {
    std::cout << " " << nextValue();
  }
  std::cout << std::endl;
}
#endif