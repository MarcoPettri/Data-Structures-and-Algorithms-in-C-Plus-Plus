// File R-3.3.cpp

/*

    R-3.3 Give a C++ code fragment that, given a n × n matrix M of type float,
    replaces M with its transpose. Try to do this without the use of a temporary
    matrix.

*/

#include <cstddef>
#include <iostream>
#include <memory>
#include <utility>

// Transpose an n x n row-major matrix in place.
void transposeInPlace(std::unique_ptr<float[]> &M, std::size_t size) {
  float *data = M.get();
  for (std::size_t i = 0; i < size; ++i) {
    for (std::size_t j = i + 1; j < size; ++j) {
      std::swap(data[i * size + j], data[j * size + i]);
    }
  }
}

void print_matrix(const std::unique_ptr<float[]> &M, std::size_t size) {
  for (std::size_t i = 0; i < size; ++i) {
    for (std::size_t j = 0; j < size; ++j)
      std::cout << M[i * size + j] << '\t';
    std::cout << '\n';
  }
}

int main() {
  constexpr std::size_t size = 4;

  // Matrix owned by a smart pointer (row-major, contiguous)
  auto M = std::make_unique<float[]>(size * size);

  for (std::size_t i = 0; i < size * size; ++i)
    M[i] = static_cast<float>(i + 1);

  print_matrix(M, size);
  std::cout << "\n";
  transposeInPlace(M, size);
  print_matrix(M, size);
  return 0;
}