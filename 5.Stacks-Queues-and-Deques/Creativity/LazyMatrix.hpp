// File : LazyMatrix.hpp

#ifndef LAZY_MATRIX_HPP
#define LAZY_MATRIX_HPP

#include <cstddef>
#include <limits>
#include <optional>
#include <stdexcept>
#include <vector>

// A logical n x n integer matrix whose initially-used backing cells are not
// initialized by this class. The caller supplies n*n readable int objects.
// Each backing cell stores an index into the stack; the stack stores
// (row, column, value) triples for cells that have been written.
class LazyIntMatrix {
public:
  explicit LazyIntMatrix(int *backing, std::size_t n)
      : backing_(backing), n_(n) {
    if (n_ > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
      throw std::length_error(
          "matrix dimension does not fit in int coordinates");
    }
    if (n_ != 0 && n_ > std::numeric_limits<std::size_t>::max() / n_) {
      throw std::length_error("n*n overflows size_t");
    }
    if (n_ != 0 && backing_ == nullptr) {
      throw std::invalid_argument("backing must point to n*n int cells");
    }

    // At most n distinct cells may be used. Reserving avoids reallocations
    // while preserving the O(n) total-time bound for up to n used cells.
    stack_.reserve(n_);
  }

  std::size_t dimension() const noexcept { return n_; }
  std::size_t used_cells() const noexcept { return stack_.size(); }

  // Unwritten logical cells behave as if they contained 0.
  int get(std::size_t row, std::size_t col) const {
    const std::optional<std::size_t> pos = find_entry(row, col);
    return pos.has_value() ? stack_[*pos].value : 0;
  }

  bool is_set(std::size_t row, std::size_t col) const {
    return find_entry(row, col).has_value();
  }

  // The backing cell is only an index into stack_; the logical value lives
  // in the matching stack triple. Repeated writes update the existing triple.
  void set(std::size_t row, std::size_t col, int value) {
    const std::size_t cell = flat_index(row, col);
    const std::optional<std::size_t> pos = find_entry_at(cell, row, col);
    if (pos.has_value()) {
      stack_[*pos].value = value;
      return;
    }

    // The stated guarantee is that at most n distinct cells are used.
    if (stack_.size() >= n_) {
      throw std::length_error(
          "LazyIntMatrix supports at most n distinct written cells");
    }

    const std::size_t new_pos = stack_.size();
    stack_.push_back(
        Entry{static_cast<int>(row), static_cast<int>(col), value});
    // new_pos <= n-1 <= INT_MAX-1, so the index is representable as int.
    backing_[cell] = static_cast<int>(new_pos);
  }

private:
  struct Entry {
    int row;
    int col;
    int value;
  };

  int *backing_;
  std::size_t n_;
  std::vector<Entry> stack_;

  std::size_t flat_index(std::size_t row, std::size_t col) const {
    if (row >= n_ || col >= n_) {
      throw std::out_of_range("matrix coordinates out of range");
    }
    return row * n_ + col;
  }

  std::optional<std::size_t> find_entry(std::size_t row,
                                        std::size_t col) const {
    const std::size_t cell = flat_index(row, col);
    return find_entry_at(cell, row, col);
  }

  std::optional<std::size_t> find_entry_at(std::size_t cell, std::size_t row,
                                           std::size_t col) const {
    // backing_[cell] can be arbitrary. Check the range before indexing S.
    const int candidate = backing_[cell];
    if (candidate < 0) {
      return std::nullopt;
    }

    const std::size_t pos = static_cast<std::size_t>(candidate);
    if (pos >= stack_.size()) {
      return std::nullopt;
    }

    const Entry &entry = stack_[pos];
    // This coordinate check is what prevents garbage in an untouched A
    // cell from being mistaken for an initialized cell.
    if (entry.row != static_cast<int>(row) ||
        entry.col != static_cast<int>(col)) {
      return std::nullopt;
    }
    return pos;
  }
};

#endif // LAZY_MATRIX_HPP
