/**
 * @file red_blue_join.tpp
 * @brief Implementation of ds::BlockSequence and ds::find_red_blue_pairs.
 *        Included by red_blue_join.hpp; do not compile directly.
 */

#ifndef DS_RED_BLUE_JOIN_TPP
#define DS_RED_BLUE_JOIN_TPP

#include <algorithm>
#include <queue>
#include <stdexcept>

namespace ds {

// ============================================================================
// BlockSequence<T>
// ============================================================================

template <typename T>
BlockSequence<T>::BlockSequence(std::size_t block_size)
    : block_size_(block_size) {
  if (block_size_ == 0)
    throw std::invalid_argument("BlockSequence: block_size must be >= 1");
}

template <typename T>
BlockSequence<T>::BlockSequence(std::vector<T> data, std::size_t block_size)
    : block_size_(block_size), total_size_(data.size()) {
  if (block_size_ == 0)
    throw std::invalid_argument("BlockSequence: block_size must be >= 1");

  blocks_.reserve((data.size() + block_size_ - 1) / block_size_);
  for (std::size_t i = 0; i < data.size(); i += block_size_) {
    const std::size_t end = std::min(i + block_size_, data.size());
    std::vector<T> blk;
    blk.reserve(end - i);
    for (std::size_t j = i; j < end; ++j)
      blk.push_back(std::move(data[j]));
    blocks_.push_back(std::move(blk));
  }
}

template <typename T>
std::vector<T> BlockSequence<T>::read_block(std::size_t idx,
                                            RedBlueIOStats *stats) const {
  if (idx >= blocks_.size())
    throw std::out_of_range("BlockSequence::read_block: index out of range");
  if (stats)
    ++stats->block_reads;
  return blocks_[idx];
}

template <typename T>
void BlockSequence<T>::append_block(std::vector<T> block,
                                    RedBlueIOStats *stats) {
  if (block.empty() || block.size() > block_size_)
    throw std::invalid_argument(
        "BlockSequence::append_block: block size must be in [1, B]");
  total_size_ += block.size();
  blocks_.push_back(std::move(block));
  if (stats)
    ++stats->block_writes;
}

template <typename T> std::vector<T> BlockSequence<T>::to_vector() const {
  std::vector<T> out;
  out.reserve(total_size_);
  for (const auto &blk : blocks_)
    out.insert(out.end(), blk.begin(), blk.end());
  return out;
}

// ============================================================================
// External k-way merge sort + streaming red-blue join
// ============================================================================

namespace detail {

inline bool elem_less(const ColoredElement &a,
                      const ColoredElement &b) noexcept {
  if (a.key != b.key)
    return a.key < b.key;
  if (a.color != b.color)
    return static_cast<std::uint8_t>(a.color) <
           static_cast<std::uint8_t>(b.color);
  return a.id < b.id;
}

inline BlockSequence<ColoredElement>
merge_colored_runs(const std::vector<BlockSequence<ColoredElement>> &runs,
                   std::size_t first_run, std::size_t last_run,
                   std::size_t block_size, RedBlueIOStats *stats) {
  const std::size_t k = last_run - first_run;

  struct Cursor {
    std::size_t next_block = 0;
    std::size_t pos = 0;
    std::vector<ColoredElement> buf;
  };
  std::vector<Cursor> cursors(k);

  struct HeapItem {
    ColoredElement elem;
    std::size_t slot;
  };
  auto cmp = [](const HeapItem &a, const HeapItem &b) {
    return elem_less(b.elem, a.elem);
  };
  std::priority_queue<HeapItem, std::vector<HeapItem>, decltype(cmp)> pq(cmp);

  for (std::size_t i = 0; i < k; ++i) {
    const auto &run = runs[first_run + i];
    if (run.num_blocks() > 0) {
      cursors[i].buf = run.read_block(0, stats);
      cursors[i].next_block = 1;
      cursors[i].pos = 1;
      pq.push(HeapItem{cursors[i].buf[0], i});
    }
  }

  BlockSequence<ColoredElement> merged(block_size);
  std::vector<ColoredElement> out_buf;
  out_buf.reserve(block_size);

  while (!pq.empty()) {
    HeapItem top = pq.top();
    pq.pop();

    out_buf.push_back(top.elem);
    if (out_buf.size() == block_size) {
      merged.append_block(std::move(out_buf), stats);
      out_buf.clear();
      out_buf.reserve(block_size);
    }

    auto &cur = cursors[top.slot];
    const auto &run = runs[first_run + top.slot];
    if (cur.pos < cur.buf.size()) {
      pq.push(HeapItem{cur.buf[cur.pos++], top.slot});
    } else if (cur.next_block < run.num_blocks()) {
      cur.buf = run.read_block(cur.next_block++, stats);
      cur.pos = 1;
      pq.push(HeapItem{cur.buf[0], top.slot});
    }
  }

  if (!out_buf.empty())
    merged.append_block(std::move(out_buf), stats);

  return merged;
}

inline BlockSequence<ColoredElement>
external_sort_colored(const BlockSequence<ColoredElement> &input,
                      std::size_t mem_capacity, RedBlueIOStats *stats) {
  const std::size_t B = input.block_size();
  const std::size_t blocks_per_mem = mem_capacity / B;
  const std::size_t k = blocks_per_mem - 1;

  std::vector<BlockSequence<ColoredElement>> runs;
  const std::size_t total_blocks = input.num_blocks();

  for (std::size_t start = 0; start < total_blocks; start += blocks_per_mem) {
    const std::size_t end = std::min(start + blocks_per_mem, total_blocks);
    std::vector<ColoredElement> buf;
    buf.reserve((end - start) * B);
    for (std::size_t b = start; b < end; ++b) {
      auto blk = input.read_block(b, stats);
      buf.insert(buf.end(), blk.begin(), blk.end());
    }
    std::sort(buf.begin(), buf.end(), elem_less);

    BlockSequence<ColoredElement> run(B);
    for (std::size_t i = 0; i < buf.size(); i += B) {
      const std::size_t chunk_end = std::min(i + B, buf.size());
      run.append_block(
          std::vector<ColoredElement>(
              buf.begin() + static_cast<std::ptrdiff_t>(i),
              buf.begin() + static_cast<std::ptrdiff_t>(chunk_end)),
          stats);
    }
    runs.push_back(std::move(run));
  }

  while (runs.size() > 1) {
    if (stats)
      ++stats->merge_passes;
    std::vector<BlockSequence<ColoredElement>> next_runs;
    next_runs.reserve((runs.size() + k - 1) / k);
    for (std::size_t i = 0; i < runs.size(); i += k) {
      const std::size_t end = std::min(i + k, runs.size());
      if (end - i == 1)
        next_runs.push_back(std::move(runs[i]));
      else
        next_runs.push_back(merge_colored_runs(runs, i, end, B, stats));
    }
    runs = std::move(next_runs);
  }

  return std::move(runs.front());
}

} // namespace detail

inline BlockSequence<RedBluePair>
find_red_blue_pairs(const BlockSequence<ColoredElement> &input,
                    std::size_t mem_capacity, RedBlueIOStats *stats) {
  const std::size_t B = input.block_size();
  if (mem_capacity < 3 * B) {
    throw std::invalid_argument(
        "find_red_blue_pairs: mem_capacity (M) must be >= 3 * block_size (B)");
  }

  BlockSequence<RedBluePair> result(B);
  if (input.empty())
    return result;

  // Step 1: External merge sort by (key, color, id).
  // After sorting, for every key x, all Red(x) elements precede all Blue(x).
  BlockSequence<ColoredElement> sorted =
      detail::external_sort_colored(input, mem_capacity, stats);

  // Step 2: Streaming join over sorted blocks with O(M) internal memory.
  // Reserve 2*B elements for 1 input read buffer and 1 output write buffer.
  const std::size_t red_cap = mem_capacity - 2 * B;

  std::vector<RedBluePair> out_buf;
  out_buf.reserve(B);
  auto emit_pair = [&](const ColoredElement &r, const ColoredElement &b) {
    out_buf.emplace_back(r, b);
    if (out_buf.size() == B) {
      result.append_block(std::move(out_buf), stats);
      out_buf.clear();
      out_buf.reserve(B);
    }
  };

  // Cursor over `sorted`
  struct SortedStream {
    const BlockSequence<ColoredElement> &seq;
    RedBlueIOStats *stats;
    std::size_t blk_idx = 0;
    std::size_t pos = 0;
    std::vector<ColoredElement> buf;

    bool has_next() {
      while (pos >= buf.size()) {
        if (blk_idx >= seq.num_blocks())
          return false;
        buf = seq.read_block(blk_idx++, stats);
        pos = 0;
      }
      return true;
    }

    const ColoredElement &peek() { return buf[pos]; }
    ColoredElement next() { return buf[pos++]; }
    std::pair<std::size_t, std::size_t> position() const noexcept {
      return {blk_idx, pos};
    }
    void seek(std::size_t saved_blk, std::size_t saved_pos) {
      if (saved_blk == blk_idx) {
        pos = saved_pos;
      } else {
        // Reload the block that was active at saved_pos.
        blk_idx = saved_blk;
        buf = (blk_idx > 0) ? seq.read_block(blk_idx - 1, stats)
                            : std::vector<ColoredElement>{};
        pos = saved_pos;
      }
    }
  };

  SortedStream stream{sorted, stats, 0, 0, {}};
  std::vector<ColoredElement> red_chunk;
  red_chunk.reserve(red_cap);

  while (stream.has_next()) {
    const int current_key = stream.peek().key;

    // Collect Red elements for current_key in chunks of size <= red_cap.
    // Remember where Blue(current_key) starts if we have multiple Red chunks.
    red_chunk.clear();
    while (stream.has_next() && stream.peek().key == current_key &&
           stream.peek().color == Color::Red && red_chunk.size() < red_cap) {
      red_chunk.push_back(stream.next());
    }

    if (red_chunk.empty()) {
      // No Red elements for current_key; skip all Blue(current_key).
      while (stream.has_next() && stream.peek().key == current_key)
        (void)stream.next();
      continue;
    }

    // Check if there are MORE Red(current_key) elements beyond red_cap.
    const bool more_reds = stream.has_next() &&
                           stream.peek().key == current_key &&
                           stream.peek().color == Color::Red;

    if (!more_reds) {
      // Fast path (|Red(x)| <= M - 2B): single scan over Blue(current_key).
      while (stream.has_next() && stream.peek().key == current_key) {
        ColoredElement b_elem = stream.next();
        for (const auto &r_elem : red_chunk)
          emit_pair(r_elem, b_elem);
      }
    } else {
      // Overflow path (|Red(x)| > M - 2B): block-nested loop over chunks of
      // Red(x) and Blue(x) without exceeding M internal memory.
      auto red_resume = stream.position();

      // Advance past remaining Red(x) to locate Blue(x) start.
      while (stream.has_next() && stream.peek().key == current_key &&
             stream.peek().color == Color::Red) {
        (void)stream.next();
      }
      const auto blue_start = stream.position();

      while (true) {
        // Pair current red_chunk with all Blue(current_key) elements.
        while (stream.has_next() && stream.peek().key == current_key) {
          ColoredElement b_elem = stream.next();
          for (const auto &r_elem : red_chunk)
            emit_pair(r_elem, b_elem);
        }
        const auto after_key = stream.position();

        // Load next Red(current_key) chunk from red_resume.
        stream.seek(red_resume.first, red_resume.second);
        red_chunk.clear();
        while (stream.has_next() && stream.peek().key == current_key &&
               stream.peek().color == Color::Red &&
               red_chunk.size() < red_cap) {
          red_chunk.push_back(stream.next());
        }

        if (red_chunk.empty()) {
          // All Red(current_key) chunks processed; resume at next key.
          stream.seek(after_key.first, after_key.second);
          break;
        }
        red_resume = stream.position();
        stream.seek(blue_start.first, blue_start.second);
      }
    }
  }

  if (!out_buf.empty())
    result.append_block(std::move(out_buf), stats);

  return result;
}

} // namespace ds

#endif // DS_RED_BLUE_JOIN_TPP
