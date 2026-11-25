/**
 * @file external_unordered_map.tpp
 * @brief Implementation of ds::ExternalUnorderedMap. Included by
 *        external_unordered_map.hpp; do not compile directly.
 */

#ifndef DS_EXTERNAL_UNORDERED_MAP_TPP
#define DS_EXTERNAL_UNORDERED_MAP_TPP

#include <stdexcept>

namespace ds {

template <typename Key, typename Value>
ExternalUnorderedMap<Key, Value>::ExternalUnorderedMap(
    std::size_t block_capacity)
    : block_capacity_(block_capacity) {
  if (block_capacity_ == 0) {
    throw std::invalid_argument(
        "ExternalUnorderedMap: block_capacity must be >= 1");
  }
  tail_buffer_.reserve(block_capacity_);
}

template <typename Key, typename Value>
typename ExternalUnorderedMap<Key, Value>::block_type
ExternalUnorderedMap<Key, Value>::read_disk_block(
    std::size_t idx, UnorderedMapIOStats *stats) const {
  if (stats)
    ++stats->block_reads;
  return disk_blocks_[idx];
}

template <typename Key, typename Value>
void ExternalUnorderedMap<Key, Value>::flush_tail(UnorderedMapIOStats *stats) {
  if (tail_buffer_.empty())
    return;
  if (stats)
    ++stats->block_writes;
  disk_blocks_.push_back(std::move(tail_buffer_));
  tail_buffer_.clear();
  tail_buffer_.reserve(block_capacity_);
}

template <typename Key, typename Value>
void ExternalUnorderedMap<Key, Value>::put(const Key &key, const Value &value,
                                           UnorderedMapIOStats *stats) {
  tail_buffer_.emplace_back(key, value);
  if (tail_buffer_.size() == block_capacity_) {
    flush_tail(stats);
  }
}

template <typename Key, typename Value>
std::optional<Value>
ExternalUnorderedMap<Key, Value>::find(const Key &key,
                                       UnorderedMapIOStats *stats) const {
  // 1. Scan internal tail buffer from newest to oldest (0 disk transfers).
  for (std::size_t i = tail_buffer_.size(); i > 0; --i) {
    if (tail_buffer_[i - 1].first == key)
      return tail_buffer_[i - 1].second;
  }

  // 2. Scan disk blocks from newest to oldest so the latest update wins.
  for (std::size_t b = disk_blocks_.size(); b > 0; --b) {
    block_type blk = read_disk_block(b - 1, stats);
    for (std::size_t i = blk.size(); i > 0; --i) {
      if (blk[i - 1].first == key)
        return blk[i - 1].second;
    }
  }

  return std::nullopt;
}

template <typename Key, typename Value>
bool ExternalUnorderedMap<Key, Value>::contains(
    const Key &key, UnorderedMapIOStats *stats) const {
  return find(key, stats).has_value();
}

template <typename Key, typename Value>
bool ExternalUnorderedMap<Key, Value>::erase(const Key &key,
                                             UnorderedMapIOStats *stats) {
  // Single-pass in-order compaction preserves chronological order (so newest
  // updates remain after older ones for all other keys) in O(n / B) I/Os.
  bool removed = false;
  std::vector<block_type> new_blocks;
  block_type new_tail;
  new_tail.reserve(block_capacity_);

  auto emit = [&](entry_type &&e) {
    new_tail.push_back(std::move(e));
    if (new_tail.size() == block_capacity_) {
      if (stats)
        ++stats->block_writes;
      new_blocks.push_back(std::move(new_tail));
      new_tail.clear();
      new_tail.reserve(block_capacity_);
    }
  };

  for (std::size_t b = 0; b < disk_blocks_.size(); ++b) {
    block_type blk = read_disk_block(b, stats);
    for (auto &entry : blk) {
      if (entry.first == key) {
        removed = true;
      } else {
        emit(std::move(entry));
      }
    }
  }

  for (const auto &entry : tail_buffer_) {
    if (entry.first == key) {
      removed = true;
    } else {
      emit(entry_type(entry));
    }
  }

  if (removed) {
    disk_blocks_ = std::move(new_blocks);
    tail_buffer_ = std::move(new_tail);
  }
  return removed;
}

} // namespace ds

#endif // DS_EXTERNAL_UNORDERED_MAP_TPP
