// File: progression.cpp

#include "progression.hpp"
#include <iostream>
#include <stdexcept>

long Progression::firstValue() {
  cur = first; // reset to first value
  return cur;
}

long Progression::nextValue() {
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