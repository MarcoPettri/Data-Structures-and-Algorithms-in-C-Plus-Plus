// Exercise Reinforcement: R-2.18
/*

    Write a short C++ program that creates a Pair class that can store two
    objects declared as generic types. Demonstrate this program by creating
    and printing Pair objects that contain five different kinds of pairs, such
   as <int,string> and <float,long>

*/
#include "Pair.hpp"
#include <iostream>
#include <string>

int main() {

  Pair<int, std::string> pair1(1, "one");
  Pair<float, long> pair2(1.5f, 15L);
  Pair<char, bool> pair3('a', true);
  Pair<double, std::string> pair4(3.14, "pi");
  Pair<int, int> pair5(10, 20);

  std::cout << "pair1: " << pair1 << std::endl;
  std::cout << "pair2: " << pair2 << std::endl;
  std::cout << "pair3: " << pair3 << std::endl;
  std::cout << "pair4: " << pair4 << std::endl;
  std::cout << "pair5: " << pair5 << std::endl;

  return 0;
}