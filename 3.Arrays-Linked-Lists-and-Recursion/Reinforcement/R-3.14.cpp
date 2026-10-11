// File: R-3.14.cpp
/*
R-3.14 Write a short C++ function that repeatedly selects and removes a random
    entry from an n-element array until the array holds no more entries.
    Assume that you have access to a function random(k), which returns a
random integer in the range from 0 to k.
*/
#include <iostream>
#include <memory>
#include <random>
// random(k) as specified: returns a random integer in [0, k].
int random(int k) {
  static std::mt19937 rng{std::random_device{}()};
  return std::uniform_int_distribution<int>{0, k}(rng);
}
// Repeatedly selects and removes a random entry from the array until empty.
// Strategy: pick a random index in [0, remaining-1], swap it with the last
// active element, then shrink the active range by 1 (Fisher-Yates drain).
// Running time: O(n) — one swap + one random call per element removed.
template <typename T> void drainRandom(std::unique_ptr<T[]> &A, int n) {
  for (int remaining = n; remaining > 0; --remaining) {
    int idx = random(remaining - 1); // random index in [0, remaining-1]
    std::cout << "Removed: " << A[idx] << "\n";
    // Swap chosen element with the last active slot, then shrink.
    std::swap(A[idx], A[remaining - 1]);
  }
}
int main() {
  constexpr int N = 6;
  auto A = std::make_unique<int[]>(N);
  int values[] = {10, 20, 30, 40, 50, 60};
  for (int i = 0; i < N; ++i)
    A[i] = values[i];
  std::cout << "Initial array: ";
  for (int i = 0; i < N; ++i)
    std::cout << A[i] << " ";
  std::cout << "\n\nDraining randomly:\n";
  drainRandom(A, N);
  std::cout << "\nArray is now empty.\n";
  return 0;
}
