/**
 * @file external_dedup.tpp
 * @brief Implementation of ds::ExternalVector and ds::external_deduplicate.
 *        Included by external_dedup.hpp; do not compile directly.
 */

#ifndef DS_EXTERNAL_DEDUP_TPP
#define DS_EXTERNAL_DEDUP_TPP

#include <algorithm>
#include <queue>
#include <stdexcept>
#include <utility>

namespace ds {

// ============================================================================
// ExternalVector<T>
// ============================================================================

template <typename T>
ExternalVector<T>::ExternalVector(std::size_t block_size)
    : block_size_(block_size) {
  if (block_size_ == 0)
    throw std::invalid_argument("ExternalVector: block_size must be >= 1");
}

template <typename T>
ExternalVector<T>::ExternalVector(std::vector<T> data, std::size_t block_size)
    : block_size_(block_size), total_size_(data.size()) {
  if (block_size_ == 0)
    throw std::invalid_argument("ExternalVector: block_size must be >= 1");

  blocks_.reserve((data.size() + block_size_ - 1) / block_size_);
  for (std::size_t i = 0; i < data.size(); i += block_size_) {
    std::size_t end = std::min(i + block_size_, data.size());
    std::vector<T> blk;
    blk.reserve(end - i);
    for (std::size_t j = i; j < end; ++j)
      blk.push_back(std::move(data[j]));
    blocks_.push_back(std::move(blk));
  }
}

template <typename T>
std::vector<T> ExternalVector<T>::read_block(std::size_t block_idx,
                                             ExternalMemoryStats *stats) const {
  if (block_idx >= blocks_.size())
    throw std::out_of_range("ExternalVector::read_block: index out of range");
  if (stats)
    ++stats->block_reads;
  return blocks_[block_idx];
}

template <typename T>
void ExternalVector<T>::append_block(std::vector<T> block,
                                     ExternalMemoryStats *stats) {
  if (block.empty() || block.size() > block_size_)
    throw std::invalid_argument(
        "ExternalVector::append_block: block size must be in [1, B]");
  total_size_ += block.size();
  blocks_.push_back(std::move(block));
  if (stats)
    ++stats->block_writes;
}

template <typename T> std::vector<T> ExternalVector<T>::to_vector() const {
  std::vector<T> out;
  out.reserve(total_size_);
  for (const auto &blk : blocks_)
    out.insert(out.end(), blk.begin(), blk.end());
  return out;
}

// ============================================================================
// Internal helpers for external_deduplicate
// ============================================================================

namespace detail {

template <typename T, typename Compare>
[[nodiscard]] bool equivalent(const T &a, const T &b, const Compare &comp) {
  return !comp(a, b) && !comp(b, a);
}

template <typename T, typename Compare>
ExternalVector<T>
merge_runs_with_dedup(const std::vector<ExternalVector<T>> &runs,
                      std::size_t first_run, std::size_t last_run,
                      std::size_t block_size, ExternalMemoryStats *stats,
                      const Compare &comp) {
  const std::size_t k = last_run - first_run;

  struct RunCursor {
    std::size_t next_block = 0;
    std::size_t pos = 0;
    std::vector<T> buffer;
  };

  std::vector<RunCursor> cursors(k);

  struct HeapItem {
    T value;
    std::size_t run_slot;
  };

  auto heap_cmp = [&comp](const HeapItem &a, const HeapItem &b) {
    // std::priority_queue is a max-heap, so invert comparison for min-heap.
    return comp(b.value, a.value);
  };
  std::priority_queue<HeapItem, std::vector<HeapItem>, decltype(heap_cmp)> pq(
      heap_cmp);

  // Prime each of the k input buffers with its first block.
  for (std::size_t i = 0; i < k; ++i) {
    const auto &run = runs[first_run + i];
    if (run.num_blocks() > 0) {
      cursors[i].buffer = run.read_block(0, stats);
      cursors[i].next_block = 1;
      cursors[i].pos = 1;
      pq.push(HeapItem{cursors[i].buffer[0], i});
    }
  }

  ExternalVector<T> merged(block_size);
  std::vector<T> out_buf;
  out_buf.reserve(block_size);

  bool has_last = false;
  T last_emitted{};

  while (!pq.empty()) {
    HeapItem top = std::move(const_cast<HeapItem &>(pq.top()));
    pq.pop();

    // Emit only if strictly different from the previous emitted element.
    if (!has_last || !equivalent(top.value, last_emitted, comp)) {
      last_emitted = top.value;
      has_last = true;
      out_buf.push_back(std::move(top.value));
      if (out_buf.size() == block_size) {
        merged.append_block(std::move(out_buf), stats);
        out_buf.clear();
        out_buf.reserve(block_size);
      }
    }

    // Advance cursor for the run that supplied `top`.
    const std::size_t slot = top.run_slot;
    auto &cur = cursors[slot];
    const auto &run = runs[first_run + slot];
    if (cur.pos < cur.buffer.size()) {
      pq.push(HeapItem{std::move(cur.buffer[cur.pos++]), slot});
    } else if (cur.next_block < run.num_blocks()) {
      cur.buffer = run.read_block(cur.next_block++, stats);
      cur.pos = 1;
      pq.push(HeapItem{std::move(cur.buffer[0]), slot});
    }
  }

  if (!out_buf.empty())
    merged.append_block(std::move(out_buf), stats);

  return merged;
}

} // namespace detail

// ============================================================================
// external_deduplicate
// ============================================================================

template <typename T, typename Compare>
ExternalVector<T>
external_deduplicate(const ExternalVector<T> &input, std::size_t mem_capacity,
                     ExternalMemoryStats *stats, Compare comp) {
  const std::size_t B = input.block_size();
  // Need at least 2 input block buffers + 1 output block buffer for 2-way merge.
  if (mem_capacity < 3 * B) {
    throw std::invalid_argument(
        "external_deduplicate: mem_capacity (M) must be >= 3 * block_size (B)");
  }

  if (input.empty())
    return ExternalVector<T>(B);

  const std::size_t blocks_per_mem = mem_capacity / B;
  // Reserve 1 block for the output buffer during k-way merge.
  const std::size_t k = blocks_per_mem - 1;

  // ---- Phase 1: Generate sorted, locally deduplicated runs ----------------
  std::vector<ExternalVector<T>> runs;
  const std::size_t total_blocks = input.num_blocks();

  for (std::size_t blk_start = 0; blk_start < total_blocks;
       blk_start += blocks_per_mem) {
    const std::size_t blk_end =
        std::min(blk_start + blocks_per_mem, total_blocks);

    std::vector<T> mem_buf;
    mem_buf.reserve((blk_end - blk_start) * B);
    for (std::size_t b_idx = blk_start; b_idx < blk_end; ++b_idx) {
      auto blk = input.read_block(b_idx, stats);
      mem_buf.insert(mem_buf.end(), std::make_move_iterator(blk.begin()),
                     std::make_move_iterator(blk.end()));
    }

    std::sort(mem_buf.begin(), mem_buf.end(), comp);
    auto unique_end = std::unique(mem_buf.begin(), mem_buf.end(),
                                  [&comp](const T &a, const T &b) {
                                    return detail::equivalent(a, b, comp);
                                  });
    mem_buf.erase(unique_end, mem_buf.end());

    ExternalVector<T> run(B);
    for (std::size_t i = 0; i < mem_buf.size(); i += B) {
      const std::size_t end = std::min(i + B, mem_buf.size());
      std::vector<T> out_blk;
      out_blk.reserve(end - i);
      for (std::size_t j = i; j < end; ++j)
        out_blk.push_back(std::move(mem_buf[j]));
      run.append_block(std::move(out_blk), stats);
    }
    runs.push_back(std::move(run));
  }

  if (stats)
    stats->initial_runs = runs.size();

  // ---- Phase 2: k-way merge passes with fused deduplication ---------------
  while (runs.size() > 1) {
    if (stats)
      ++stats->merge_passes;

    std::vector<ExternalVector<T>> next_runs;
    next_runs.reserve((runs.size() + k - 1) / k);

    for (std::size_t i = 0; i < runs.size(); i += k) {
      const std::size_t end = std::min(i + k, runs.size());
      if (end - i == 1) {
        // Odd leftover run: carry forward without extra I/O.
        next_runs.push_back(std::move(runs[i]));
      } else {
        next_runs.push_back(
            detail::merge_runs_with_dedup(runs, i, end, B, stats, comp));
      }
    }
    runs = std::move(next_runs);
  }

  return std::move(runs.front());
}

} // namespace ds

#endif // DS_EXTERNAL_DEDUP_TPP
