/**
 * @file external_majority.hpp
 * @brief External-memory majority element detection (> n/2 occurrences) via
 *        2-pass Boyer-Moore voting in O(n / B) disk transfers.
 *
 * ### Algorithm
 * 1. **Pass 1 (Candidate selection)**: Stream the array block-by-block into a
 *    1-block internal buffer, maintaining `(candidate, count)` via Boyer-Moore
 *    majority voting.
 * 2. **Pass 2 (Verification)**: Stream the array block-by-block a second time
 *    to count exact occurrences of `candidate`, returning it iff `freq > n / 2`.
 *
 * Complexity summary (n = array size, B = elements per disk block):
 *  - Disk transfers : 2 * ceil(n / B) = O(n / B) block reads, 0 writes
 *  - CPU time       : O(n)
 *  - Internal memory: O(B) (1 block buffer + O(1) state)
 *
 * @standard C++17
 */

#ifndef DS_EXTERNAL_MAJORITY_HPP
#define DS_EXTERNAL_MAJORITY_HPP

#include <algorithm>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <vector>

namespace ds {

/**
 * @struct MajorityIOStats
 * @brief Tracks disk block I/O transfers during external majority detection.
 */
struct MajorityIOStats {
  std::size_t block_reads = 0;  ///< Number of disk blocks read
  std::size_t block_writes = 0; ///< Number of disk blocks written (always 0)

  /** @brief Total block transfers (reads + writes). */
  [[nodiscard]] std::size_t total_ios() const noexcept {
    return block_reads + block_writes;
  }
};

/**
 * @class ExternalArray
 * @brief Read-only block-partitioned external-memory array with I/O counting.
 *
 * @tparam T  Element type.
 */
template <typename T> class ExternalArray {
public:
  /**
   * @brief Constructs an external array from @p data with block size @p B.
   *
   * @param data        Input elements.
   * @param block_size  Number of elements B per disk block (B >= 1).
   * @throws std::invalid_argument if block_size == 0.
   *
   * @section complexity Time: $O(n)$, where $n = \text{data.size()}$.
   */
  ExternalArray(const std::vector<T> &data, std::size_t block_size)
      : block_size_(block_size), total_size_(data.size()) {
    if (block_size_ == 0)
      throw std::invalid_argument("ExternalArray: block_size must be >= 1");

    blocks_.reserve((data.size() + block_size_ - 1) / block_size_);
    for (std::size_t i = 0; i < data.size(); i += block_size_) {
      const std::size_t end = std::min(i + block_size_, data.size());
      blocks_.emplace_back(data.begin() + static_cast<std::ptrdiff_t>(i),
                           data.begin() + static_cast<std::ptrdiff_t>(end));
    }
  }

  /**
   * @brief Reads block @p idx from external storage.
   *
   * @param idx    Zero-based block index.
   * @param stats  Optional pointer to increment block_reads.
   * @return       Block contents (up to B elements).
   * @throws std::out_of_range if idx >= num_blocks().
   *
   * @section complexity Time: $O(B)$. I/O: $1$ block read.
   */
  [[nodiscard]] std::vector<T>
  read_block(std::size_t idx, MajorityIOStats *stats = nullptr) const {
    if (idx >= blocks_.size())
      throw std::out_of_range("ExternalArray::read_block: index out of range");
    if (stats)
      ++stats->block_reads;
    return blocks_[idx];
  }

  [[nodiscard]] std::size_t size() const noexcept { return total_size_; }
  [[nodiscard]] bool empty() const noexcept { return total_size_ == 0; }
  [[nodiscard]] std::size_t block_size() const noexcept { return block_size_; }
  [[nodiscard]] std::size_t num_blocks() const noexcept {
    return blocks_.size();
  }

private:
  std::size_t block_size_;
  std::size_t total_size_;
  std::vector<std::vector<T>> blocks_;
};

/**
 * @brief Determines whether @p arr contains a value occurring strictly more
 *        than `arr.size() / 2` times using 2-pass Boyer-Moore majority voting.
 *
 * @tparam T  Element type (must be EqualityComparable and CopyAssignable).
 *
 * @param arr    External-memory array of size n.
 * @param stats  Optional pointer to record disk I/O transfers.
 * @return       The majority value if one exists, or `std::nullopt` otherwise.
 *
 * @section complexity
 *   - **Disk Transfers:** $2 \lceil n / B \rceil = O(n / B)$ block reads, $0$
 *     writes, where $n$ is `arr.size()` and $B$ is `arr.block_size()`.
 *   - **CPU Time:** $O(n)$ worst-case.
 *   - **Internal Memory:** $O(B)$ (one block buffer plus two scalar variables).
 */
template <typename T>
[[nodiscard]] std::optional<T>
find_majority_element(const ExternalArray<T> &arr,
                      MajorityIOStats *stats = nullptr) {
  if (arr.empty())
    return std::nullopt;

  const std::size_t num_blocks = arr.num_blocks();

  // ---- Pass 1: Boyer-Moore candidate selection ----------------------------
  T candidate{};
  std::size_t count = 0;

  for (std::size_t b = 0; b < num_blocks; ++b) {
    const std::vector<T> blk = arr.read_block(b, stats);
    for (const T &x : blk) {
      if (count == 0) {
        candidate = x;
        count = 1;
      } else if (x == candidate) {
        ++count;
      } else {
        --count;
      }
    }
  }

  // ---- Pass 2: Verify whether `candidate` occurs > n / 2 times ------------
  std::size_t freq = 0;
  for (std::size_t b = 0; b < num_blocks; ++b) {
    const std::vector<T> blk = arr.read_block(b, stats);
    for (const T &x : blk) {
      if (x == candidate)
        ++freq;
    }
  }

  if (freq > arr.size() / 2)
    return candidate;
  return std::nullopt;
}

} // namespace ds

#endif // DS_EXTERNAL_MAJORITY_HPP
