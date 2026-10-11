// File : R-3.4.cpp

/*
    R-3.4 Describe a way to use recursion to compute the sum of all the elements
    in a n × n (two-dimensional) array of integers.
*/

#include <cstddef>
#include <iostream>
#include <memory>

long long recursive_sum(const std::unique_ptr<int[]> &M, std::size_t size,
                        int idx = 0);

void print_matrix(const std::unique_ptr<int[]> &M, std::size_t size) {
  for (std::size_t i = 0; i < size; ++i) {
    for (std::size_t j = 0; j < size; ++j)
      std::cout << M[i * size + j] << '\t';
    std::cout << '\n';
  }
}

int main() {
  constexpr std::size_t size = 4;

  auto M = std::make_unique<int[]>(size * size);

  for (int i = 0; i < size * size; ++i) {
    M[i] = (i + 1);
  }
  std::cout << "Array:\n";
  print_matrix(M, size);
  std::cout << "\nSum: " << recursive_sum(M, size) << std::endl;

  return 0;
}

long long recursive_sum(const std::unique_ptr<int[]> &array, std::size_t size,
                        int idx) {
  if (idx == size * size) {
    return 0;
  }
  auto row = idx / size;
  auto col = idx % size;

  return array[row * size + col] + recursive_sum(array, size, idx + 1);
}