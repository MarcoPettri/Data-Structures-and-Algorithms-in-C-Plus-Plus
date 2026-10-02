// Exercise Reinforcement: R-2.15
/*

    Write a short C++ function that counts the number of vowels in a given
    character string.

*/

#include <iostream>
#include <string>

int countVowels(const std::string &str);

int main() {
  std::string str = "hello";
  std::cout << countVowels(str) << std::endl;
  return 0;
}

int countVowels(const std::string &str) {
  int count = 0;
  const std::string vowels = "aeiouAEIOU";
  for (const auto &ch : str) {
    if (vowels.find(ch) != std::string::npos) {
      count++;
    }
  }
  return count;
}