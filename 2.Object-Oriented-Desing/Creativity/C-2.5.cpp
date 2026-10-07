// Exercise Creativity: C-2.5
/*
    Write a C++ class that is derived from the Progression class to produce a
    progression where each value is the absolute value of the difference between
    the previous two values. You should include a default constructor that
   starts with 2 and 200 as the first two values and a parametric constructor
   that starts with a specified pair of numbers as the first two values.
*/

#include "Abs_diff_prog.hpp"
#include <iostream>

int main() {
  AbsDiffProg prog1;         // default constructor: 2, 200, 198, ...
  AbsDiffProg prog2(2, 200); // parametric constructor
  AbsDiffProg prog3(10, 3);  // another example

  std::cout << "Default progression: ";
  prog1.printProgression(10);

  std::cout << "2, 200 progression: ";
  prog2.printProgression(10);

  std::cout << "10, 3 progression: ";
  prog3.printProgression(10);

  return 0;
}
