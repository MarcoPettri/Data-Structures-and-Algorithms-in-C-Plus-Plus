// Exercise Reinforcement: R-2.16
/*

    Write a short C++ function that removes all the punctuation from a string s
    storing a sentence. For example, this operation would transform the string
    “Let’s try, Mike.” to “Lets try Mike”.

*/

#include <cctype>
#include <iostream>
#include <string>

std::string removePunctuation(const std::string &str);

int main() {
  std::string str = "Let's try, Mike.";
  std::cout << removePunctuation(str) << std::endl;
  return 0;
}

std::string removePunctuation(const std::string &str) {
  std::string result;
  result.reserve(str.size());
  for (const auto &ch : str) {
    if (std::ispunct(ch)) {
      continue;
    }
    result += ch;
  }
  return result;
}