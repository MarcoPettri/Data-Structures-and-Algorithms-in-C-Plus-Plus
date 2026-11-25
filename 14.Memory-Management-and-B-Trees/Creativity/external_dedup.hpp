/**
 * @file external_dedup.hpp
 * @brief External-memory deduplication via k-way merge sort with fused
 *        duplicate elimination (interface).
 *
 * Models the External Memory (DAM) model with:
 *  - Internal memory capacity M (elements)
 *  - Disk block transfer size B (elements per I/O)
 *  - Multi-way merge fan-in k = floor(M / B) - 1 (>= 2, so M >= 3B)
 *
 * Complexity summary (N = input size, M = memory capacity, B = block size):
 *  - I/O transfers : O((N / B) * (1 + log_{M/B}(N / M)))
 *  - Internal CPU  : O(N log N)
 *  - Disk space    : O(N) blocks (at most 2x input during merge passes)
 *
 */

#ifndef DS_EXTERNAL_DEDUP_HPP
#define DS_EXTERNAL_DEDUP_HPP

#include <cstddef>
#include <functional>
#include <vector>

namespace ds {

/**
 * @struct ExternalMemoryStats
 * @brief Tracks I/O operations performed during external-memory execution.
 */
struct ExternalMemoryStats {
  std::size_t block_reads = 0; ///< Number of blocks read from external storage
  std::size_t block_writes =
      0; ///< Number of blocks written to external storage
  std::size_t initial_runs = 0; ///< Number of sorted runs generated in Phase 1
  std::size_t merge_passes = 0; ///< Number of k-way merge passes in Phase 2

  /** @brief Total block I/O transfers (reads + writes). */
  [[nodiscard]] std::size_t total_ios() const noexcept {
    return block_reads + block_writes;
  }
};

/**
 * @class ExternalVector
 * @brief Simulates an external-memory vector partitioned into fixed-size
 *        blocks of @p B elements, counting every block read and write.
 *
 * @tparam T  Element type (must be Copyable / Movable).
 */
template <typename T> class ExternalVector {
public:
  /**
   * @brief Constructs an empty external vector with block size @p block_size.
   *
   * @param block_size  Number of elements per disk block (B >= 1).
   * @throws std::invalid_argument if block_size == 0.
   *
   * @section complexity Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  explicit ExternalVector(std::size_t block_size);

  /**
   * @brief Constructs an external vector initialized from @p data.
   *
   * @param data        Initial elements.
   * @param block_size  Number of elements per disk block (B >= 1).
   * @throws std::invalid_argument if block_size == 0.
   *
   * @section complexity Time: $O(N)$, where $N = \text{data.size()}$. Space: $O(N)$.
   */
  ExternalVector(std::vector<T> data, std::size_t block_size);

  /**
   * @brief Reads block @p block_idx from external storage.
   *
   * @param block_idx  Zero-based block index.
   * @param stats      Optional I/O statistics counter to increment.
   * @return           Vector containing up to B elements of that block.
   * @throws std::out_of_range if block_idx >= num_blocks().
   *
   * @section complexity Time: $O(B)$, where $B$ is the block size. I/O: 1 block read.
   */
  [[nodiscard]] std::vector<T>
  read_block(std::size_t block_idx, ExternalMemoryStats *stats = nullptr) const;

  /**
   * @brief Appends a block of at most B elements to external storage.
   *
   * @param block  Block contents (size in [1, B]).
   * @param stats  Optional I/O statistics counter to increment.
   * @throws std::invalid_argument if block is empty or exceeds B elements.
   *
   * @section complexity Time: $O(B)$, where $B$ is the block size. I/O: 1 block write.
   */
  void append_block(std::vector<T> block, ExternalMemoryStats *stats = nullptr);

  /** @brief Total number of elements stored. */
  [[nodiscard]] std::size_t size() const noexcept { return total_size_; }

  /** @brief True iff no elements are stored. */
  [[nodiscard]] bool empty() const noexcept { return total_size_ == 0; }

  /** @brief Block capacity B. */
  [[nodiscard]] std::size_t block_size() const noexcept { return block_size_; }

  /** @brief Number of blocks currently stored. */
  [[nodiscard]] std::size_t num_blocks() const noexcept {
    return blocks_.size();
  }

  /**
   * @brief Materializes all elements into an in-memory std::vector (for testing).
   *
   * @return std::vector<T> containing all elements in order.
   * @section complexity Time: $O(N)$, where $N = \text{size()}$. Space: $O(N)$.
   */
  [[nodiscard]] std::vector<T> to_vector() const;

private:
  std::size_t block_size_;
  std::size_t total_size_ = 0;
  std::vector<std::vector<T>> blocks_;
};

/**
 * @brief Removes all duplicate entries from @p input using an external-memory
 *        k-way merge sort with fused deduplication.
 *
 * ### Algorithm
 * 1. **Run generation**: Stream @p input in chunks of at most @p mem_capacity
 *    ($M$) elements, sort and deduplicate each chunk in internal memory, and
 *    write the resulting sorted runs to external storage.
 * 2. **k-way merge with deduplication**: With $k = \lfloor M / B \rfloor - 1$,
 *    repeatedly merge up to $k$ sorted runs using a min-heap over the head of
 *    each run's $B$-element input buffer. Equal consecutive elements are skipped
 *    before writing to the $B$-element output buffer.
 *
 * @tparam T        Element type.
 * @tparam Compare  Strict weak ordering comparator (default std::less<T>).
 *
 * @param input         External-memory vector of size N.
 * @param mem_capacity  Internal memory limit M in elements (must satisfy M >= 3*B).
 * @param stats         Optional pointer to record I/O statistics.
 * @param comp          Comparator instance.
 * @return              New ExternalVector<T> containing all unique elements in
 *                      sorted order.
 * @throws std::invalid_argument if mem_capacity < 3 * input.block_size().
 *
 * @section complexity
 *   - **I/O Complexity:** $O\left(\frac{N}{B}\left(1 + \log_{M/B}\frac{N}{M}\right)\right)$
 *     block transfers, where $N$ is the input size, $M$ is internal memory
 *     capacity, and $B$ is the block size.
 *   - **CPU Complexity:** $O(N \log N)$ worst-case.
 *   - **Internal Memory:** $O(M)$ elements.
 */
template <typename T, typename Compare = std::less<T>>
[[nodiscard]] ExternalVector<T>
external_deduplicate(const ExternalVector<T> &input, std::size_t mem_capacity,
                     ExternalMemoryStats *stats = nullptr,
                     Compare comp = Compare{});

} // namespace ds

#include "external_dedup.tpp"

#endif // DS_EXTERNAL_DEDUP_HPP
