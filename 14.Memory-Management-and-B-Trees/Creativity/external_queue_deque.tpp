/**
 * @file external_queue_deque.tpp
 * @brief Implementation of ds::ExternalQueue and ds::ExternalDeque.
 *        Included by external_queue_deque.hpp; do not compile directly.
 */

#ifndef DS_EXTERNAL_QUEUE_DEQUE_TPP
#define DS_EXTERNAL_QUEUE_DEQUE_TPP

#include <iterator>
#include <stdexcept>
#include <utility>

namespace ds {

// ============================================================================
// ExternalQueue<T>
// ============================================================================

template <typename T>
ExternalQueue<T>::ExternalQueue(std::size_t block_size)
    : block_size_(block_size) {
  if (block_size_ == 0)
    throw std::invalid_argument("ExternalQueue: block_size must be >= 1");
  back_buf_.reserve(block_size_);
}

template <typename T>
void ExternalQueue<T>::enqueue(const T &value, QueueIOStats *stats) {
  back_buf_.push_back(value);
  ++size_;
  if (back_buf_.size() == block_size_) {
    // Flush full block to disk (1 block write).
    if (stats)
      ++stats->block_writes;
    disk_blocks_.push_back(std::move(back_buf_));
    back_buf_.clear();
    back_buf_.reserve(block_size_);
  }
}

template <typename T>
void ExternalQueue<T>::ensure_front_loaded(QueueIOStats *stats) {
  if (!front_buf_.empty())
    return;
  if (!disk_blocks_.empty()) {
    if (stats)
      ++stats->block_reads;
    auto blk = std::move(disk_blocks_.front());
    disk_blocks_.pop_front();
    for (auto &x : blk)
      front_buf_.push_back(std::move(x));
  } else {
    // No disk blocks: move directly from internal back buffer (0 I/Os).
    for (auto &x : back_buf_)
      front_buf_.push_back(std::move(x));
    back_buf_.clear();
    back_buf_.reserve(block_size_);
  }
}

template <typename T> T ExternalQueue<T>::dequeue(QueueIOStats *stats) {
  if (empty())
    throw std::underflow_error("ExternalQueue::dequeue: queue is empty");
  ensure_front_loaded(stats);
  T val = std::move(front_buf_.front());
  front_buf_.pop_front();
  --size_;
  return val;
}

template <typename T> const T &ExternalQueue<T>::front(QueueIOStats *stats) {
  if (empty())
    throw std::underflow_error("ExternalQueue::front: queue is empty");
  ensure_front_loaded(stats);
  return front_buf_.front();
}

// ============================================================================
// ExternalDeque<T>
// ============================================================================

template <typename T>
ExternalDeque<T>::ExternalDeque(std::size_t block_size)
    : block_size_(block_size) {
  if (block_size_ == 0)
    throw std::invalid_argument("ExternalDeque: block_size must be >= 1");
}

template <typename T>
void ExternalDeque<T>::push_front(const T &value, QueueIOStats *stats) {
  if (front_buf_.size() == 2 * block_size_) {
    // Flush the innermost B elements (back half of front_buf_) to disk,
    // leaving the outermost B elements in front_buf_ (hysteresis).
    std::vector<T> blk;
    blk.reserve(block_size_);
    auto split_it =
        front_buf_.begin() + static_cast<std::ptrdiff_t>(block_size_);
    for (auto it = split_it; it != front_buf_.end(); ++it)
      blk.push_back(std::move(*it));
    front_buf_.erase(split_it, front_buf_.end());

    if (stats)
      ++stats->block_writes;
    disk_blocks_.push_front(std::move(blk));
  }
  front_buf_.push_front(value);
  ++size_;
}

template <typename T>
void ExternalDeque<T>::push_back(const T &value, QueueIOStats *stats) {
  if (back_buf_.size() == 2 * block_size_) {
    // Flush the innermost B elements (front half of back_buf_) to disk,
    // leaving the outermost B elements in back_buf_ (hysteresis).
    std::vector<T> blk;
    blk.reserve(block_size_);
    auto split_it =
        back_buf_.begin() + static_cast<std::ptrdiff_t>(block_size_);
    for (auto it = back_buf_.begin(); it != split_it; ++it)
      blk.push_back(std::move(*it));
    back_buf_.erase(back_buf_.begin(), split_it);

    if (stats)
      ++stats->block_writes;
    disk_blocks_.push_back(std::move(blk));
  }
  back_buf_.push_back(value);
  ++size_;
}

template <typename T>
void ExternalDeque<T>::ensure_front_non_empty(QueueIOStats *stats) {
  if (!front_buf_.empty())
    return;
  if (!disk_blocks_.empty()) {
    if (stats)
      ++stats->block_reads;
    auto blk = std::move(disk_blocks_.front());
    disk_blocks_.pop_front();
    for (auto &x : blk)
      front_buf_.push_back(std::move(x));
  } else {
    // Disk is empty: rebalance half of back_buf_ (at least 1) into front_buf_
    // with 0 disk I/Os.
    const std::size_t take = (back_buf_.size() + 1) / 2;
    for (std::size_t i = 0; i < take; ++i) {
      front_buf_.push_back(std::move(back_buf_.front()));
      back_buf_.pop_front();
    }
  }
}

template <typename T>
void ExternalDeque<T>::ensure_back_non_empty(QueueIOStats *stats) {
  if (!back_buf_.empty())
    return;
  if (!disk_blocks_.empty()) {
    if (stats)
      ++stats->block_reads;
    auto blk = std::move(disk_blocks_.back());
    disk_blocks_.pop_back();
    for (auto &x : blk)
      back_buf_.push_back(std::move(x));
  } else {
    // Disk is empty: rebalance half of front_buf_ (at least 1) into back_buf_
    // with 0 disk I/Os.
    const std::size_t take = (front_buf_.size() + 1) / 2;
    for (std::size_t i = 0; i < take; ++i) {
      back_buf_.push_front(std::move(front_buf_.back()));
      front_buf_.pop_back();
    }
  }
}

template <typename T> T ExternalDeque<T>::pop_front(QueueIOStats *stats) {
  if (empty())
    throw std::underflow_error("ExternalDeque::pop_front: deque is empty");
  ensure_front_non_empty(stats);
  T val = std::move(front_buf_.front());
  front_buf_.pop_front();
  --size_;
  return val;
}

template <typename T> T ExternalDeque<T>::pop_back(QueueIOStats *stats) {
  if (empty())
    throw std::underflow_error("ExternalDeque::pop_back: deque is empty");
  ensure_back_non_empty(stats);
  T val = std::move(back_buf_.back());
  back_buf_.pop_back();
  --size_;
  return val;
}

template <typename T> const T &ExternalDeque<T>::front(QueueIOStats *stats) {
  if (empty())
    throw std::underflow_error("ExternalDeque::front: deque is empty");
  ensure_front_non_empty(stats);
  return front_buf_.front();
}

template <typename T> const T &ExternalDeque<T>::back(QueueIOStats *stats) {
  if (empty())
    throw std::underflow_error("ExternalDeque::back: deque is empty");
  ensure_back_non_empty(stats);
  return back_buf_.back();
}

} // namespace ds

#endif // DS_EXTERNAL_QUEUE_DEQUE_TPP
