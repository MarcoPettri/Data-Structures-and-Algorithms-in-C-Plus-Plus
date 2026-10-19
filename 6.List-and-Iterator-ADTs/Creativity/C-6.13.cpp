// File : C-6.13.cpp

/*
    C-6.13 Describe an efficient way of putting a vector representing a deck of
   n cards into random order. You may use a function, randomInteger(n), which
    returns a random number between 0 and n − 1, inclusive. Your method
    should guarantee that every possible ordering is equally likely. What is
    the running time of your function?
*/

#include <cstddef>
#include <iostream>
#include <random>
#include <utility>
#include <vector>
/**
 * @brief Uniformly shuffles a vector in place using the Fisher-Yates algorithm.
 * @tparam T Element type.
 * @tparam Rng Uniform random bit generator type (e.g., std::mt19937).
 * @param a The vector representing the deck of cards to shuffle.
 * @param rng Reference to a random number generator instance.
 * @note Complexity: O(n) time, O(1) extra space.
 */

template <typename T, typename Rng>
void random_shuffle_deck(std::vector<T> &a, Rng &rng) {
  for (std::size_t i = a.size(); i > 1; --i) {
    // Uniform distribution in the range [0, i - 1]
    std::uniform_int_distribution<std::size_t> pick(0, i - 1);
    std::swap(a[i - 1], a[pick(rng)]);
  }
}

int main() {
  // Initialize a deck of cards (represented as integers 1 to 10)
  std::vector<int> deck = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

  // Set up a standard Mersenne Twister random number engine with a random seed
  std::random_device rd;
  std::mt19937 rng(rd());

  // Shuffle the deck in-place
  random_shuffle_deck(deck, rng);

  // Print the shuffled deck
  std::cout << "Shuffled deck: ";
  for (int card : deck) {
    std::cout << card << " ";
  }
  std::cout << "\n";

  return 0;
}