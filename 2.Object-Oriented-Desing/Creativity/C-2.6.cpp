// Exercise Creativity: C-2.6
/*
    Write a C++ class that is derived from the Progression class to produce
    a progression where each value is the square root of the previous value.
    (Note that you can no longer represent each value with an integer.) You
    should include a default constructor that starts with 65, 536 as the first
    value and a parametric constructor that starts with a specified (double)
    number as the first value.
*/

#include "SqrtProgression.hpp"
#include <iostream>

int main() {
  SqrtProg s1;
  std::cout << "Default constructor (65,536): \n";
  s1.printProgression(8);

  std::cout << "\n";

  SqrtProg s2(16777216);
  std::cout << "Parametric constructor (16,777,216): \n";
  s2.printProgression(8);

  return 0;
}