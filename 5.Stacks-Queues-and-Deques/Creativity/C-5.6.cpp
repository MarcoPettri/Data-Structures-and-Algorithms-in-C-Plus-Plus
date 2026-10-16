// File : C-5.6.cpp

/*
    C-5.6 Suppose we have an n × n two-dimensional array A that we want to use
   to store integers, but we don’t want to spend the O(n^2) work to initialize
   it to all 0’s, because we already know that we are only going to use up to n
   of these cells in our algorithm, which itself runs in O(n) time (not counting
    the time to initialize A). Show how to use an array-based stack S storing
    (i, j, k) integer triples to allow us to use the array A without
   initializing it and still implement our algorithm in O(n) time, even though
   the initial values in the cells of A might be total garbage.
*/
#include "LazyMatrix.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error("TEST FAILED: " + message);
  }
}

// Tests use initialized-but-arbitrary integers to simulate the garbage values
// that may already be present in A. This avoids reading indeterminate C++ ints.
std::vector<int> make_garbage(std::size_t n, unsigned seed) {
  std::vector<int> data(n * n);
  std::mt19937 rng(seed);
  std::uniform_int_distribution<int> wide(-1000000000, 1000000000);
  std::uniform_int_distribution<int> small(-2, static_cast<int>(n + 2));

  for (std::size_t i = 0; i < data.size(); ++i) {
    // Frequently include values that look like plausible stack indices.
    data[i] = (rng() % 2 == 0) ? small(rng) : wide(rng);
  }
  if (!data.empty()) {
    data[0] = std::numeric_limits<int>::min();
    if (data.size() > 1)
      data[1] = std::numeric_limits<int>::max();
  }
  return data;
}

void test_empty_and_basic_operations() {
  const std::size_t n = 4;
  auto backing = make_garbage(n, 11);
  LazyIntMatrix matrix(backing.data(), n);

  for (std::size_t r = 0; r < n; ++r) {
    for (std::size_t c = 0; c < n; ++c) {
      require(matrix.get(r, c) == 0, "unwritten cell must read as zero");
      require(!matrix.is_set(r, c), "unwritten cell must not be marked set");
    }
  }
  require(matrix.used_cells() == 0, "initial stack must be empty");

  matrix.set(1, 2, 73);
  require(matrix.get(1, 2) == 73, "get after first set");
  require(matrix.is_set(1, 2), "is_set after first set");
  require(matrix.used_cells() == 1, "first write uses one stack entry");

  matrix.set(1, 2, -19);
  require(matrix.get(1, 2) == -19, "overwriting a cell updates its value");
  require(matrix.used_cells() == 1,
          "overwrite must not add another stack entry");

  matrix.set(0, 0, 0);
  require(matrix.get(0, 0) == 0, "explicitly stored zero reads as zero");
  require(matrix.is_set(0, 0), "explicitly stored zero is still marked set");
  require(matrix.used_cells() == 2, "explicit zero counts as a used cell");
}

void test_garbage_index_collision() {
  const std::size_t n = 5;
  auto backing = make_garbage(n, 22);
  LazyIntMatrix matrix(backing.data(), n);

  matrix.set(0, 1, 1234);
  const int valid_stack_index = backing[0 * n + 1];
  require(valid_stack_index == 0, "first stack entry should have index zero");

  // Make an untouched cell point at a valid stack entry for a DIFFERENT
  // coordinate. The coordinate check must reject it.
  backing[4 * n + 4] = valid_stack_index;
  require(matrix.get(4, 4) == 0,
          "valid index for another coordinate is still garbage");
  require(!matrix.is_set(4, 4), "collision must not mark another cell as set");
  require(matrix.get(0, 1) == 1234, "original entry remains accessible");

  matrix.set(4, 4, 5678);
  require(matrix.get(4, 4) == 5678, "write after a rejected collision works");
  require(matrix.used_cells() == 2,
          "collision must not have created a phantom entry");
}

void test_capacity_limit() {
  const std::size_t n = 3;
  auto backing = make_garbage(n, 33);
  LazyIntMatrix matrix(backing.data(), n);

  matrix.set(0, 0, 10);
  matrix.set(0, 1, 20);
  matrix.set(1, 0, 30);
  require(matrix.used_cells() == n, "can use n distinct cells");

  bool threw = false;
  try {
    matrix.set(1, 1, 40);
  } catch (const std::length_error &) {
    threw = true;
  }
  require(threw, "writing an (n+1)-th distinct cell should throw");
  require(matrix.get(0, 0) == 10 && matrix.get(0, 1) == 20 &&
              matrix.get(1, 0) == 30,
          "capacity error preserves existing values");
  require(matrix.used_cells() == n,
          "capacity error does not change stack size");
}

void test_randomized_against_reference() {
  const std::size_t n = 32;
  const std::size_t cells = n * n;
  auto backing = make_garbage(n, 44);
  LazyIntMatrix matrix(backing.data(), n);

  std::vector<std::size_t> all(cells);
  for (std::size_t i = 0; i < cells; ++i)
    all[i] = i;
  std::mt19937 rng(123456);
  std::shuffle(all.begin(), all.end(), rng);
  all.resize(n); // Only n distinct logical cells will ever be written.

  std::vector<std::optional<int>> expected(cells);
  std::uniform_int_distribution<std::size_t> selected(0, all.size() - 1);
  std::uniform_int_distribution<std::size_t> any_cell(0, cells - 1);
  std::uniform_int_distribution<int> values(-50000, 50000);
  std::uniform_int_distribution<int> action(0, 99);

  for (int step = 0; step < 12000; ++step) {
    if (action(rng) < 55) {
      const std::size_t flat = all[selected(rng)];
      const std::size_t row = flat / n;
      const std::size_t col = flat % n;
      const int value = values(rng);
      matrix.set(row, col, value);
      expected[flat] = value;
    } else {
      const std::size_t flat = any_cell(rng);
      const std::size_t row = flat / n;
      const std::size_t col = flat % n;
      const int reference = expected[flat].has_value() ? *expected[flat] : 0;
      require(matrix.get(row, col) == reference,
              "randomized get disagrees with reference model");
      require(matrix.is_set(row, col) == expected[flat].has_value(),
              "randomized is_set disagrees with reference model");
    }
  }

  std::size_t expected_used = 0;
  for (std::size_t flat = 0; flat < cells; ++flat) {
    const std::size_t row = flat / n;
    const std::size_t col = flat % n;
    const int reference = expected[flat].has_value() ? *expected[flat] : 0;
    require(matrix.get(row, col) == reference, "final scan value mismatch");
    require(matrix.is_set(row, col) == expected[flat].has_value(),
            "final scan initialized-state mismatch");
    if (expected[flat].has_value())
      ++expected_used;
  }
  require(matrix.used_cells() == expected_used,
          "stack size matches distinct writes");
}

void test_bounds_and_zero_dimension() {
  LazyIntMatrix empty(nullptr, 0);
  require(empty.dimension() == 0 && empty.used_cells() == 0,
          "zero-dimensional matrix can be constructed");

  const std::size_t n = 2;
  auto backing = make_garbage(n, 55);
  LazyIntMatrix matrix(backing.data(), n);
  bool threw = false;
  try {
    (void)matrix.get(2, 0);
  } catch (const std::out_of_range &) {
    threw = true;
  }
  require(threw, "out-of-bounds access should throw");
}

} // namespace

int main() {
  try {
    test_empty_and_basic_operations();
    test_garbage_index_collision();
    test_capacity_limit();
    test_randomized_against_reference();
    test_bounds_and_zero_dimension();
    std::cout << "All LazyIntMatrix tests passed.\n";
    return EXIT_SUCCESS;
  } catch (const std::exception &ex) {
    std::cerr << ex.what() << '\n';
    return EXIT_FAILURE;
  }
}
