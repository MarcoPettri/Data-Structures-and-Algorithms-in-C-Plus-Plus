/**
 * @file red_blue_join.hpp
 * @brief External-memory algorithm to find all matching (red, blue) pairs
 *        with equal keys in a sequence S of size n (interface).
 *
 * Algorithm:
 *  1. External k-way merge sort S by (key, color) in
 *     O((n / B) * (1 + log_{M/B}(n / M))) disk transfers.
 *  2. Stream through the sorted sequence, grouping contiguous runs of Red
 *     and Blue elements per distinct key, and emitting their Cartesian product
 *     using a memory-bounded chunked scan (O(n / B + T / B) transfers).
 *
 * Total disk transfers: O((n / B) * (1 + log_{M/B}(n / M)) + T / B),
 * where T is the number of output red-blue pairs.
 *
 */

#ifndef DS_RED_BLUE_JOIN_HPP
#define DS_RED_BLUE_JOIN_HPP

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace ds {

/** @brief Color tag for elements in S. */
enum class Color : std::uint8_t { Red = 0, Blue = 1 };

/**
 * @struct ColoredElement
 * @brief Element of S with an integer key, color, and identifier/payload.
 */
struct ColoredElement {
  int key = 0;
  Color color = Color::Red;
  std::size_t id = 0;

  bool operator==(const ColoredElement &other) const noexcept {
    return key == other.key && color == other.color && id == other.id;
  }
};

/** @brief Output pair of matching (red, blue) elements with equal keys. */
using RedBluePair = std::pair<ColoredElement, ColoredElement>;

/**
 * @struct RedBlueIOStats
 * @brief Tracks disk block I/O transfers during external red-blue pairing.
 */
struct RedBlueIOStats {
  std::size_t block_reads = 0;
  std::size_t block_writes = 0;
  std::size_t merge_passes = 0;

  /** @brief Total block transfers (reads + writes). */
  [[nodiscard]] std::size_t total_ios() const noexcept {
    return block_reads + block_writes;
  }
};

/**
 * @class BlockSequence
 * @brief Block-partitioned external-memory sequence with explicit I/O counting.
 *
 * @tparam T  Element type.
 */
template <typename T> class BlockSequence {
public:
  /**
   * @brief Constructs an empty block sequence.
   * @param block_size  Number of elements B per disk block (B >= 1).
   * @throws std::invalid_argument if block_size == 0.
   * @section complexity Time: $O(1)$.
   */
  explicit BlockSequence(std::size_t block_size);

  /**
   * @brief Constructs a block sequence from @p data.
   * @param data        Initial elements.
   * @param block_size  Number of elements B per disk block (B >= 1).
   * @throws std::invalid_argument if block_size == 0.
   * @section complexity Time: $O(n)$, where $n = \text{data.size()}$.
   */
  BlockSequence(std::vector<T> data, std::size_t block_size);

  /**
   * @brief Reads block @p idx from external storage.
   * @section complexity Time: $O(B)$. I/O: $1$ block read.
   */
  [[nodiscard]] std::vector<T>
  read_block(std::size_t idx, RedBlueIOStats *stats = nullptr) const;

  /**
   * @brief Appends a block of at most B elements to external storage.
   * @section complexity Time: $O(B)$. I/O: $1$ block write.
   */
  void append_block(std::vector<T> block, RedBlueIOStats *stats = nullptr);

  [[nodiscard]] std::size_t size() const noexcept { return total_size_; }
  [[nodiscard]] bool empty() const noexcept { return total_size_ == 0; }
  [[nodiscard]] std::size_t block_size() const noexcept { return block_size_; }
  [[nodiscard]] std::size_t num_blocks() const noexcept {
    return blocks_.size();
  }

  /** @brief Materializes all elements into an in-memory vector (for tests). */
  [[nodiscard]] std::vector<T> to_vector() const;

private:
  std::size_t block_size_;
  std::size_t total_size_ = 0;
  std::vector<std::vector<T>> blocks_;
};

/**
 * @brief Finds all pairs (e, f) in @p input such that e.color == Red,
 *        f.color == Blue, and e.key == f.key.
 *
 * @param input         External-memory sequence S of size n.
 * @param mem_capacity  Internal memory capacity M in elements (M >= 3 * B).
 * @param stats         Optional pointer to record disk I/O transfers.
 * @return              BlockSequence<RedBluePair> containing all matching pairs.
 * @throws std::invalid_argument if mem_capacity < 3 * input.block_size().
 *
 * @section complexity
 *   - **Disk Transfers:** $O\!\left(\frac{n}{B}\left(1 + \log_{M/B}\frac{n}{M}\right) + \frac{T}{B}\right)$,
 *     where $n = |S|$, $B$ is the block size, $M$ is internal memory capacity,
 *     and $T$ is the number of output red-blue pairs.
 *   - **Internal Memory:** $O(M)$ elements.
 */
[[nodiscard]] inline BlockSequence<RedBluePair>
find_red_blue_pairs(const BlockSequence<ColoredElement> &input,
                    std::size_t mem_capacity, RedBlueIOStats *stats = nullptr);

} // namespace ds

#include "red_blue_join.tpp"

#endif // DS_RED_BLUE_JOIN_HPP
