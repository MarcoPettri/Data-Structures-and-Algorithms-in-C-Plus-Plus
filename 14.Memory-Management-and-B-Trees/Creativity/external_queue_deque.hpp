/**
 * @file external_queue_deque.hpp
 * @brief External-memory Queue and Deque ADTs achieving O(n / B) total disk
 *        transfers for any sequence of n operations (interface).
 *
 * - **ExternalQueue<T>**: Uses two 1-block internal buffers (`front_buf_`,
 *   `back_buf_` of capacity B) plus a disk block queue. Every block of B
 *   elements is written at most once and read at most once.
 * - **ExternalDeque<T>**: Uses two 2B-capacity hysteresis buffers
 *   (`front_buf_`, `back_buf_`) plus a disk block deque. Whenever an end
 *   buffer overflows (reaches 2B) or underflows (reaches 0), it is restored
 *   to size B via 1 disk transfer, guaranteeing at least B operations between
 *   consecutive transfers at the same end (no ping-pong thrashing).
 *
 * Complexity summary (n = number of operations, B = elements per disk block):
 *  - Sequence of n queue/deque ops : O(n / B) total disk transfers
 *  - Amortized I/O per operation   : O(1 / B) transfers
 *  - Internal memory               : O(B) elements
 *
 */

#ifndef DS_EXTERNAL_QUEUE_DEQUE_HPP
#define DS_EXTERNAL_QUEUE_DEQUE_HPP

#include <cstddef>
#include <deque>
#include <vector>

namespace ds {

/**
 * @struct QueueIOStats
 * @brief Tracks disk block I/O transfers for ExternalQueue and ExternalDeque.
 */
struct QueueIOStats {
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

// ============================================================================
// ExternalQueue<T>
// ============================================================================

/**
 * @class ExternalQueue
 * @brief External-memory FIFO queue with O(1 / B) amortized I/O per operation.
 *
 * @tparam T  Element type (must be Copyable / Movable).
 */
template <typename T> class ExternalQueue {
public:
  /**
   * @brief Constructs an empty external queue with block size @p block_size.
   *
   * @param block_size  Number of elements B per disk block (B >= 1).
   * @throws std::invalid_argument if block_size == 0.
   *
   * @section complexity Time: $O(1)$. I/O: $0$.
   */
  explicit ExternalQueue(std::size_t block_size);

  /**
   * @brief Enqueues @p value at the back of the queue.
   *
   * @param value  Element to enqueue.
   * @param stats  Optional pointer to record I/O transfers.
   *
   * @section complexity
   *   - **I/O Transfers:** $O(1/B)$ amortized ($0$ or $1$ block write).
   *   - **CPU Time:** $O(1)$ amortized, $O(B)$ on flush.
   */
  void enqueue(const T &value, QueueIOStats *stats = nullptr);

  /**
   * @brief Removes and returns the front element of the queue.
   *
   * @param stats  Optional pointer to record I/O transfers.
   * @return       The removed front element.
   * @throws std::underflow_error if the queue is empty.
   *
   * @section complexity
   *   - **I/O Transfers:** $O(1/B)$ amortized ($0$ or $1$ block read).
   *   - **CPU Time:** $O(1)$ amortized, $O(B)$ on block load.
   */
  T dequeue(QueueIOStats *stats = nullptr);

  /**
   * @brief Returns a const reference to the front element.
   *
   * @param stats  Optional pointer to record I/O transfers if front buffer
   *               must be loaded from disk.
   * @throws std::underflow_error if the queue is empty.
   *
   * @section complexity Time: $O(1)$ amortized.
   */
  [[nodiscard]] const T &front(QueueIOStats *stats = nullptr);

  /** @brief Number of elements currently in the queue. */
  [[nodiscard]] std::size_t size() const noexcept { return size_; }

  /** @brief True iff the queue is empty. */
  [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

  /** @brief Block capacity B. */
  [[nodiscard]] std::size_t block_size() const noexcept { return block_size_; }

  /** @brief Number of full blocks currently on disk. */
  [[nodiscard]] std::size_t num_disk_blocks() const noexcept {
    return disk_blocks_.size();
  }

private:
  void ensure_front_loaded(QueueIOStats *stats);

  std::size_t block_size_;
  std::size_t size_ = 0;
  std::deque<T> front_buf_;
  std::vector<T> back_buf_;
  std::deque<std::vector<T>> disk_blocks_;
};

// ============================================================================
// ExternalDeque<T>
// ============================================================================

/**
 * @class ExternalDeque
 * @brief External-memory double-ended queue using 2B-capacity hysteresis
 *        buffers at both ends so any sequence of n operations uses O(n / B)
 *        total disk transfers.
 *
 * @tparam T  Element type (must be Copyable / Movable).
 */
template <typename T> class ExternalDeque {
public:
  /**
   * @brief Constructs an empty external deque with block size @p block_size.
   *
   * @param block_size  Number of elements B per disk block (B >= 1).
   * @throws std::invalid_argument if block_size == 0.
   *
   * @section complexity Time: $O(1)$. I/O: $0$.
   */
  explicit ExternalDeque(std::size_t block_size);

  /**
   * @brief Inserts @p value at the front of the deque.
   *
   * @param value  Element to insert.
   * @param stats  Optional pointer to record I/O transfers.
   *
   * @section complexity
   *   - **I/O Transfers:** $O(1/B)$ amortized ($0$ or $1$ block write).
   *   - **CPU Time:** $O(1)$ amortized.
   */
  void push_front(const T &value, QueueIOStats *stats = nullptr);

  /**
   * @brief Inserts @p value at the back of the deque.
   *
   * @param value  Element to insert.
   * @param stats  Optional pointer to record I/O transfers.
   *
   * @section complexity
   *   - **I/O Transfers:** $O(1/B)$ amortized ($0$ or $1$ block write).
   *   - **CPU Time:** $O(1)$ amortized.
   */
  void push_back(const T &value, QueueIOStats *stats = nullptr);

  /**
   * @brief Removes and returns the front element.
   *
   * @param stats  Optional pointer to record I/O transfers.
   * @return       The removed front element.
   * @throws std::underflow_error if the deque is empty.
   *
   * @section complexity
   *   - **I/O Transfers:** $O(1/B)$ amortized ($0$ or $1$ block read).
   *   - **CPU Time:** $O(1)$ amortized.
   */
  T pop_front(QueueIOStats *stats = nullptr);

  /**
   * @brief Removes and returns the back element.
   *
   * @param stats  Optional pointer to record I/O transfers.
   * @return       The removed back element.
   * @throws std::underflow_error if the deque is empty.
   *
   * @section complexity
   *   - **I/O Transfers:** $O(1/B)$ amortized ($0$ or $1$ block read).
   *   - **CPU Time:** $O(1)$ amortized.
   */
  T pop_back(QueueIOStats *stats = nullptr);

  /**
   * @brief Returns a const reference to the front element.
   * @throws std::underflow_error if the deque is empty.
   * @section complexity Time: $O(1)$ amortized.
   */
  [[nodiscard]] const T &front(QueueIOStats *stats = nullptr);

  /**
   * @brief Returns a const reference to the back element.
   * @throws std::underflow_error if the deque is empty.
   * @section complexity Time: $O(1)$ amortized.
   */
  [[nodiscard]] const T &back(QueueIOStats *stats = nullptr);

  /** @brief Number of elements currently in the deque. */
  [[nodiscard]] std::size_t size() const noexcept { return size_; }

  /** @brief True iff the deque is empty. */
  [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

  /** @brief Block capacity B. */
  [[nodiscard]] std::size_t block_size() const noexcept { return block_size_; }

  /** @brief Number of full blocks currently on disk. */
  [[nodiscard]] std::size_t num_disk_blocks() const noexcept {
    return disk_blocks_.size();
  }

private:
  void ensure_front_non_empty(QueueIOStats *stats);
  void ensure_back_non_empty(QueueIOStats *stats);

  std::size_t block_size_;
  std::size_t size_ = 0;
  std::deque<T> front_buf_; ///< Up to 2*B elements at the front
  std::deque<T> back_buf_;  ///< Up to 2*B elements at the back
  std::deque<std::vector<T>> disk_blocks_;
};

} // namespace ds

#include "external_queue_deque.tpp"

#endif // DS_EXTERNAL_QUEUE_DEQUE_HPP
