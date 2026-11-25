/**
 * @file external_unordered_map.hpp
 * @brief External-memory Map ADT backed by an unordered block-sequence
 *        (interface).
 *
 * Stores key-value entries in an unordered sequence of fixed-capacity disk
 * blocks (capacity @p B entries per block) plus a 1-block internal-memory
 * tail buffer.
 *
 * Complexity summary (n = number of entries, B = entries per disk block):
 *  - put (insert / update) .. O(1) I/O transfers worst-case (<= 1 block write)
 *  - find / contains ........ O(n / B) I/O transfers worst-case
 *  - erase .................. O(n / B) I/O transfers worst-case
 *  - record_count / empty ... O(1) CPU, 0 I/O transfers
 *
 * @standard C++17
 */

#ifndef DS_EXTERNAL_UNORDERED_MAP_HPP
#define DS_EXTERNAL_UNORDERED_MAP_HPP

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace ds {

/**
 * @struct UnorderedMapIOStats
 * @brief Tracks disk block I/O transfers for ExternalUnorderedMap.
 */
struct UnorderedMapIOStats {
  std::size_t block_reads = 0;  ///< Number of disk blocks read
  std::size_t block_writes = 0; ///< Number of disk blocks written

  /** @brief Total block transfers (reads + writes). */
  [[nodiscard]] std::size_t total_ios() const noexcept {
    return block_reads + block_writes;
  }

  /** @brief Resets all counters to zero. */
  void reset() noexcept {
    block_reads = 0;
    block_writes = 0;
  }
};

/**
 * @class ExternalUnorderedMap
 * @brief External-memory map using an unordered block sequence.
 *
 * ### Design
 * - **Disk storage**: A sequence of full blocks, each holding @p B entries.
 * - **Internal memory**: One tail buffer of capacity @p B entries and one
 *   scratch read buffer of capacity @p B entries ($O(B)$ internal memory).
 * - **Insertion (`put`)**: Appends `(key, value)` to the tail buffer. If the
 *   tail buffer becomes full (@p B entries), it is flushed to disk in 1 block
 *   write. Thus every insertion performs at most 1 disk transfer ($O(1)$).
 * - **Search (`find` / `contains`)**: Scans from newest to oldest (tail buffer
 *   first, then disk blocks `num_blocks()-1` down to `0`). Returning the first
 *   match guarantees the most recent value for @p key is returned without
 *   needing to scan on `put`. Worst-case transfers: $\lceil n / B \rceil = O(n/B)$.
 * - **Removal (`erase`)**: Scans the sequence in order and compacts out all
 *   records matching @p key in a single streaming pass ($O(n/B)$ transfers),
 *   preserving chronological order of remaining entries.
 *
 * @tparam Key    Key type (must be EqualityComparable).
 * @tparam Value  Mapped value type.
 */
template <typename Key, typename Value> class ExternalUnorderedMap {
public:
  using entry_type = std::pair<Key, Value>;

  /**
   * @brief Constructs an empty external unordered map.
   *
   * @param block_capacity  Number of entries B that fit in one disk block (B >= 1).
   * @throws std::invalid_argument if block_capacity == 0.
   *
   * @section complexity Time: $O(1)$. I/O: $0$ transfers.
   */
  explicit ExternalUnorderedMap(std::size_t block_capacity);

  /**
   * @brief Inserts or updates `(key, value)` in the map.
   *
   * Appends the entry to the internal tail buffer; flushes 1 block to disk
   * when the tail buffer reaches capacity @p B.
   *
   * @param key    Key to insert or update.
   * @param value  Associated value.
   * @param stats  Optional pointer to record I/O transfers.
   *
   * @section complexity
   *   - **I/O Transfers:** $O(1)$ worst-case (at most $1$ block write).
   *   - **CPU Time:** $O(1)$ amortized, $O(B)$ worst-case on flush, where $B$
   *     is the block capacity.
   */
  void put(const Key &key, const Value &value,
           UnorderedMapIOStats *stats = nullptr);

  /**
   * @brief Searches for @p key and returns its most recent mapped value.
   *
   * Scans the internal tail buffer and then disk blocks from newest to oldest.
   *
   * @param key    Target search key.
   * @param stats  Optional pointer to record I/O transfers.
   * @return       `std::optional<Value>` containing the value if found, or
   *               `std::nullopt` otherwise.
   *
   * @section complexity
   *   - **I/O Transfers:** $O(n / B)$ worst-case (at most $\lfloor n / B \rfloor$
   *     disk block reads, since the tail block is in internal memory), where
   *     $n$ is `record_count()` and $B$ is the block capacity.
   *   - **CPU Time:** $O(n)$ worst-case.
   */
  [[nodiscard]] std::optional<Value>
  find(const Key &key, UnorderedMapIOStats *stats = nullptr) const;

  /**
   * @brief Checks whether @p key is present in the map.
   *
   * @param key    Target search key.
   * @param stats  Optional pointer to record I/O transfers.
   * @return       true if @p key exists, false otherwise.
   *
   * @section complexity
   *   - **I/O Transfers:** $O(n / B)$ worst-case, where $n$ is `record_count()`
   *     and $B$ is the block capacity.
   */
  [[nodiscard]] bool contains(const Key &key,
                              UnorderedMapIOStats *stats = nullptr) const;

  /**
   * @brief Removes all entries matching @p key from the sequence.
   *
   * Compacts the sequence in a single streaming pass while preserving the
   * chronological order of all remaining entries.
   *
   * @param key    Key to remove.
   * @param stats  Optional pointer to record I/O transfers.
   * @return       true if at least one entry with @p key was removed.
   *
   * @section complexity
   *   - **I/O Transfers:** $O(n / B)$ worst-case, where $n$ is `record_count()`
   *     and $B$ is the block capacity.
   *   - **CPU Time:** $O(n)$ worst-case.
   */
  bool erase(const Key &key, UnorderedMapIOStats *stats = nullptr);

  /**
   * @brief Total number of records currently in the sequence.
   * @section complexity Time: $O(1)$. I/O: $0$.
   */
  [[nodiscard]] std::size_t record_count() const noexcept {
    return disk_blocks_.size() * block_capacity_ + tail_buffer_.size();
  }

  /**
   * @brief True iff no records are stored.
   * @section complexity Time: $O(1)$. I/O: $0$.
   */
  [[nodiscard]] bool empty() const noexcept { return record_count() == 0; }

  /** @brief Block capacity B. */
  [[nodiscard]] std::size_t block_capacity() const noexcept {
    return block_capacity_;
  }

  /** @brief Number of full blocks currently flushed to disk. */
  [[nodiscard]] std::size_t num_disk_blocks() const noexcept {
    return disk_blocks_.size();
  }

private:
  using block_type = std::vector<entry_type>;

  block_type read_disk_block(std::size_t idx, UnorderedMapIOStats *stats) const;
  void flush_tail(UnorderedMapIOStats *stats);

  std::size_t block_capacity_;
  std::vector<block_type> disk_blocks_; ///< Simulated external disk blocks
  block_type tail_buffer_;              ///< 1-block internal memory buffer
};

} // namespace ds

#include "external_unordered_map.tpp"

#endif // DS_EXTERNAL_UNORDERED_MAP_HPP
