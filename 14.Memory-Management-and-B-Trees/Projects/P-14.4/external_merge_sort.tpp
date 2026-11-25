/**
 * @file external_merge_sort.tpp
 * @brief Implementation of extsort::external_merge_sorter.
 *
 * @details
 * This file is not meant to be included directly; it is included at the
 * bottom of external_merge_sort.hpp. It contains the RAII file helpers
 * (extsort::detail), the buffered block reader/writer, run formation and the
 * k-way heap merge.
 *
 * Invariants relied upon throughout:
 *  - `block_elems_ <= mem_elems_ / 3`, hence `fan_in_ = mem_elems_ /
 *    block_elems_ - 1 >= 2` and `(fan_in_ + 1) * block_elems_ <= mem_elems_`
 *    (k input buffers + 1 output buffer fit in the memory budget);
 *  - runs are stored in input order, and groups of runs are always
 *    consecutive, so merging with a (value, run-index) tie-break is stable.
 */
#pragma once

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>

namespace extsort {
namespace detail {

/// @brief Ceiling division for unsigned integers; requires `b > 0`.
[[nodiscard]] constexpr std::uint64_t ceil_div(std::uint64_t a,
                                               std::uint64_t b) noexcept {
  return a / b + (a % b != 0 ? 1 : 0);
}

/// @brief Deleter closing a `FILE*` (errors are ignored: read paths and
///        error paths only; write paths use close_checked()).
struct file_closer {
  void operator()(std::FILE *f) const noexcept {
    if (f != nullptr)
      std::fclose(f);
  }
};
using file_ptr = std::unique_ptr<std::FILE, file_closer>;

/// @brief Opens a file, throwing `std::runtime_error` with the OS message.
[[nodiscard]] inline file_ptr open_file(const std::filesystem::path &p,
                                        const char *mode) {
  std::FILE *f = std::fopen(p.string().c_str(), mode);
  if (f == nullptr) {
    const int err = errno;
    throw std::runtime_error("extsort: cannot open '" + p.string() +
                             "': " + std::strerror(err));
  }
  return file_ptr(f);
}

/// @brief Closes a file being written and reports flush/close failures.
inline void close_checked(file_ptr &f, const std::filesystem::path &p) {
  std::FILE *raw = f.release();
  if (raw != nullptr && std::fclose(raw) != 0)
    throw std::runtime_error("extsort: error while closing '" + p.string() +
                             "'");
}

/**
 * @brief Move-only owner of a temporary file; the file is deleted on
 *        destruction or reset().
 */
class temp_file {
public:
  temp_file() = default;
  explicit temp_file(std::filesystem::path p) : path_(std::move(p)) {}
  temp_file(const temp_file &) = delete;
  temp_file &operator=(const temp_file &) = delete;
  temp_file(temp_file &&other) noexcept : path_(std::move(other.path_)) {
    other.path_.clear();
  }
  temp_file &operator=(temp_file &&other) noexcept {
    if (this != &other) {
      reset();
      path_ = std::move(other.path_);
      other.path_.clear();
    }
    return *this;
  }
  ~temp_file() { reset(); }

  /// @brief Deletes the file now (best effort, never throws).
  void reset() noexcept {
    if (!path_.empty()) {
      std::error_code ec;
      std::filesystem::remove(path_, ec);
      path_.clear();
    }
  }
  /// @brief Gives up ownership without deleting the file.
  void release() noexcept { path_.clear(); }
  [[nodiscard]] const std::filesystem::path &get() const noexcept {
    return path_;
  }

private:
  std::filesystem::path path_;
};

/// @brief Process-wide unique, monotonically increasing id (thread-safe).
[[nodiscard]] inline std::uint64_t next_unique_id() noexcept {
  static std::atomic<std::uint64_t> counter{0};
  return counter.fetch_add(1, std::memory_order_relaxed);
}

/// @brief Writes `n` records to a fresh file in one call.
template <typename Value>
void write_array(const std::filesystem::path &p, const Value *data,
                 std::size_t n) {
  file_ptr f = open_file(p, "wb");
  if (n > 0 && std::fwrite(data, sizeof(Value), n, f.get()) != n)
    throw std::runtime_error("extsort: write failed on '" + p.string() + "'");
  close_checked(f, p);
}

/**
 * @brief Sequential reader that fetches records in blocks of B elements.
 *
 * Each call to next() is amortized $O(1)$; one block transfer is issued
 * (and counted in `stats->block_reads`) per $B$ records.
 */
template <typename Value> class block_reader {
public:
  block_reader(const std::filesystem::path &p, std::size_t block_elems,
               sort_stats *stats)
      : file_(open_file(p, "rb")), path_(p), buf_(block_elems), stats_(stats) {}

  /// @brief Fetches the next record; returns false at end of file.
  bool next(Value &out) {
    if (pos_ == len_ && !refill())
      return false;
    out = buf_[pos_++];
    return true;
  }

private:
  bool refill() {
    pos_ = 0;
    len_ = std::fread(buf_.data(), sizeof(Value), buf_.size(), file_.get());
    if (len_ < buf_.size() && std::ferror(file_.get()) != 0)
      throw std::runtime_error("extsort: read failed on '" + path_.string() +
                               "'");
    if (len_ > 0)
      ++stats_->block_reads;
    return len_ > 0;
  }

  file_ptr file_;
  std::filesystem::path path_;
  std::vector<Value> buf_;
  sort_stats *stats_;
  std::size_t pos_ = 0;
  std::size_t len_ = 0;
};

/**
 * @brief Sequential writer that emits records in blocks of B elements.
 *
 * finish() must be called to flush and to surface I/O errors; destroying the
 * writer without it simply closes the file (used on error paths).
 */
template <typename Value> class block_writer {
public:
  block_writer(const std::filesystem::path &p, std::size_t block_elems,
               sort_stats *stats)
      : file_(open_file(p, "wb")), path_(p), buf_(block_elems), stats_(stats) {}

  void put(const Value &v) {
    buf_[len_++] = v;
    if (len_ == buf_.size())
      flush();
  }

  void finish() {
    flush();
    close_checked(file_, path_);
  }

private:
  void flush() {
    if (len_ == 0)
      return;
    if (std::fwrite(buf_.data(), sizeof(Value), len_, file_.get()) != len_)
      throw std::runtime_error("extsort: write failed on '" + path_.string() +
                               "'");
    ++stats_->block_writes;
    len_ = 0;
  }

  file_ptr file_;
  std::filesystem::path path_;
  std::vector<Value> buf_;
  sort_stats *stats_;
  std::size_t len_ = 0;
};

} // namespace detail

template <typename Value, typename Compare>
external_merge_sorter<Value, Compare>::external_merge_sorter(config cfg,
                                                             Compare comp)
    : cfg_(std::move(cfg)), comp_(std::move(comp)) {
  mem_elems_ = cfg_.memory_bytes / sizeof(Value);
  if (mem_elems_ < 4)
    throw std::invalid_argument(
        "extsort: memory budget must hold at least 4 records");
  if (cfg_.max_fan_in == 1)
    throw std::invalid_argument("extsort: max_fan_in must be 0 or >= 2");

  // Clamp B so that at least 3 blocks fit in memory (2 inputs + 1 output).
  block_elems_ = std::max<std::size_t>(1, cfg_.block_bytes / sizeof(Value));
  block_elems_ = std::min(block_elems_, mem_elems_ / 3);

  fan_in_ = mem_elems_ / block_elems_ - 1; // >= 2 by the clamp above
  if (cfg_.max_fan_in != 0)
    fan_in_ = std::min(fan_in_, cfg_.max_fan_in);

  // One block of the budget is reserved for the run writer's staging buffer.
  run_elems_ = mem_elems_ - block_elems_;

  temp_dir_ = cfg_.temp_dir.empty() ? std::filesystem::temp_directory_path()
                                    : cfg_.temp_dir;
  salt_ = std::random_device{}();
}

template <typename Value, typename Compare>
std::filesystem::path
external_merge_sorter<Value, Compare>::make_temp_path() const {
  return temp_dir_ / ("extsort_" + std::to_string(salt_) + "_" +
                      std::to_string(detail::next_unique_id()) + ".run");
}

template <typename Value, typename Compare>
void external_merge_sorter<Value, Compare>::form_runs(
    const std::filesystem::path &input, std::uint64_t n,
    const std::filesystem::path &direct_out,
    std::vector<detail::temp_file> &runs) {
  const bool single_run = n <= run_elems_;
  const std::size_t cap =
      static_cast<std::size_t>(std::min<std::uint64_t>(run_elems_, n));

  detail::file_ptr in = detail::open_file(input, "rb");
  std::vector<Value> chunk(cap);

  std::uint64_t remaining = n;
  while (remaining > 0) {
    const std::size_t want =
        static_cast<std::size_t>(std::min<std::uint64_t>(cap, remaining));
    const std::size_t got =
        std::fread(chunk.data(), sizeof(Value), want, in.get());
    if (got != want)
      throw std::runtime_error("extsort: short read on '" + input.string() +
                               "' (file modified during the sort?)");
    remaining -= got;
    stats_.block_reads += detail::ceil_div(got, block_elems_);

    // stable_sort + in-order runs + index tie-break in the merge => stable.
    std::stable_sort(chunk.begin(),
                     chunk.begin() + static_cast<std::ptrdiff_t>(got), comp_);

    if (single_run) {
      detail::write_array(direct_out, chunk.data(), got);
    } else {
      runs.emplace_back(make_temp_path());
      detail::write_array(runs.back().get(), chunk.data(), got);
    }
    stats_.block_writes += detail::ceil_div(got, block_elems_);
    ++stats_.initial_runs;
  }
}

template <typename Value, typename Compare>
void external_merge_sorter<Value, Compare>::merge_group(
    const std::vector<detail::temp_file> &runs, std::size_t first,
    std::size_t last, const std::filesystem::path &dest) {
  struct entry {
    Value value;
    std::size_t run; // index into `readers`
  };

  const std::size_t k = last - first;
  std::vector<detail::block_reader<Value>> readers;
  readers.reserve(k);
  for (std::size_t i = first; i < last; ++i)
    readers.emplace_back(runs[i].get(), block_elems_, &stats_);

  // std::*_heap build a max-heap w.r.t. the predicate, so the predicate says
  // "a has LOWER priority than b". Smaller value = higher priority; on ties
  // the smaller run index (earlier in the input) wins, which gives stability.
  auto lower_priority = [this](const entry &a, const entry &b) {
    if (comp_(b.value, a.value))
      return true;
    if (comp_(a.value, b.value))
      return false;
    return a.run > b.run;
  };

  std::vector<entry> heap;
  heap.reserve(k);
  for (std::size_t i = 0; i < k; ++i) {
    Value v;
    if (readers[i].next(v))
      heap.push_back(entry{v, i});
  }
  std::make_heap(heap.begin(), heap.end(), lower_priority);

  detail::block_writer<Value> writer(dest, block_elems_, &stats_);
  while (!heap.empty()) {
    std::pop_heap(heap.begin(), heap.end(), lower_priority);
    entry &top = heap.back(); // the minimum, moved to the back
    writer.put(top.value);
    if (readers[top.run].next(top.value))
      std::push_heap(heap.begin(), heap.end(), lower_priority);
    else
      heap.pop_back(); // run exhausted
  }
  writer.finish();
}

template <typename Value, typename Compare>
sort_stats external_merge_sorter<Value, Compare>::sort_file(
    const std::filesystem::path &input, const std::filesystem::path &output) {
  stats_ = sort_stats{};

  const std::uintmax_t bytes = std::filesystem::file_size(input);
  if (bytes % sizeof(Value) != 0)
    throw std::runtime_error("extsort: size of '" + input.string() +
                             "' is not a multiple of the record size");
  const std::uint64_t n = bytes / sizeof(Value);
  stats_.elements = n;

  // The result is built next to the destination and renamed on success, so a
  // failure never clobbers an existing output (or the input when in == out).
  std::filesystem::path out_tmp = output;
  out_tmp += ".extsort.tmp";
  detail::temp_file out_guard(out_tmp);

  if (n == 0) {
    detail::write_array<Value>(out_tmp, nullptr, 0);
  } else {
    std::vector<detail::temp_file> runs;
    form_runs(input, n, out_tmp, runs); // input is closed on return

    if (n > run_elems_) {
      // Intermediate passes: only while more than k runs remain. Because
      // r > k at the loop head, ceil(r / k) >= 2, so the final merge below
      // always has at least two runs.
      while (runs.size() > fan_in_) {
        std::vector<detail::temp_file> next;
        next.reserve(
            static_cast<std::size_t>(detail::ceil_div(runs.size(), fan_in_)));
        for (std::size_t first = 0; first < runs.size(); first += fan_in_) {
          const std::size_t last = std::min(runs.size(), first + fan_in_);
          if (last - first == 1) { // lone run: carry over without any I/O
            next.push_back(std::move(runs[first]));
            continue;
          }
          next.emplace_back(make_temp_path());
          merge_group(runs, first, last, next.back().get());
          for (std::size_t i = first; i < last; ++i)
            runs[i].reset(); // free disk space as soon as possible
        }
        runs = std::move(next);
        ++stats_.merge_passes;
      }
      merge_group(runs, 0, runs.size(), out_tmp);
      ++stats_.merge_passes;
    }
  }

  std::filesystem::rename(out_tmp, output);
  out_guard.release();
  return stats_;
}

template <typename Value>
void write_records(const std::filesystem::path &path, const Value *data,
                   std::size_t n) {
  static_assert(std::is_trivially_copyable_v<Value>,
                "extsort: Value must be trivially copyable");
  detail::write_array(path, data, n);
}

template <typename Value>
std::vector<Value> read_records(const std::filesystem::path &path) {
  static_assert(std::is_trivially_copyable_v<Value> &&
                    std::is_default_constructible_v<Value>,
                "extsort: Value must be trivially copyable and default "
                "constructible");
  const std::uintmax_t bytes = std::filesystem::file_size(path);
  if (bytes % sizeof(Value) != 0)
    throw std::runtime_error("extsort: size of '" + path.string() +
                             "' is not a multiple of the record size");
  const std::size_t n = static_cast<std::size_t>(bytes / sizeof(Value));
  std::vector<Value> out(n);
  detail::file_ptr f = detail::open_file(path, "rb");
  if (n > 0 && std::fread(out.data(), sizeof(Value), n, f.get()) != n)
    throw std::runtime_error("extsort: read failed on '" + path.string() + "'");
  return out;
}

} // namespace extsort
