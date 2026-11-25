/**
 * @file external_merge_sort.hpp
 * @brief External-memory (disk-based) stable merge sort for files of
 *        fixed-size binary records.
 *
 * @details
 * Implements the classic two-phase external merge sort in the
 * Aggarwal-Vitter I/O model:
 *
 *  1. **Run formation** - the input is read in chunks of at most
 *     `M - B` elements, each chunk is sorted in RAM with `std::stable_sort`
 *     and written to its own temporary file (a *run*).
 *  2. **Multi-way merging** - runs are merged `k = M/B - 1` at a time with a
 *     binary min-heap, one `B`-element buffer per input run plus one output
 *     buffer, until a single sorted file remains. Intermediate passes are
 *     only needed when the number of runs exceeds `k`.
 *
 * Ties are broken by (run index), and runs are kept in input order, so the
 * whole sort is **stable**.
 *
 * Notation used in all complexity blocks:
 *  - $n$ : number of records in the input file,
 *  - $M$ : internal-memory budget in records (`memory_bytes / sizeof(T)`),
 *  - $B$ : block size in records (`block_bytes / sizeof(T)`),
 *  - $k$ : merge fan-in (`min(max_fan_in, M/B - 1)`),
 *  - $r = \lceil n / (M - B) \rceil$ : number of initial runs,
 *  - $P = \lceil \log_k r \rceil$ : number of merge passes ($0$ if $r \le 1$).
 *
 * Record requirements: trivially copyable and default constructible
 * (records are stored as raw bytes in native byte order, so files are not
 * portable across architectures).
 *
 * Thread safety: an `external_merge_sorter` object is **not** thread-safe;
 * distinct objects may be used concurrently (temporary file names are
 * globally unique).
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <type_traits>
#include <vector>

namespace extsort {

namespace detail {
class temp_file;
} // namespace detail

/**
 * @brief Tuning parameters of the external sorter.
 *
 * @note `memory_bytes` bounds the main record buffers (run buffer and merge
 *       block buffers). During run formation `std::stable_sort` may
 *       additionally allocate a scratch buffer of up to about the size of the
 *       run, so peak RAM can approach `2 * memory_bytes`.
 */
struct config {
  /// Internal-memory budget in bytes (the model parameter $M$).
  std::size_t memory_bytes = std::size_t{64} << 20;
  /// Disk-block (I/O buffer) size in bytes (the model parameter $B$).
  std::size_t block_bytes = std::size_t{64} << 10;
  /// Upper bound on the merge fan-in; `0` means "as large as memory allows".
  std::size_t max_fan_in = 0;
  /// Directory for temporary runs; empty means the system temp directory.
  std::filesystem::path temp_dir{};
};

/// @brief Counters describing one call to external_merge_sorter::sort_file.
struct sort_stats {
  std::uint64_t elements = 0;     ///< Records sorted ($n$).
  std::uint64_t initial_runs = 0; ///< Runs produced by run formation ($r$).
  std::uint64_t merge_passes = 0; ///< Merge passes including the final one.
  std::uint64_t block_reads = 0;  ///< Block-sized read transfers issued.
  std::uint64_t block_writes = 0; ///< Block-sized write transfers issued.
};

/**
 * @brief Stable external merge sorter for binary files of records.
 *
 * @tparam Value   Record type; must be trivially copyable and default
 *                 constructible.
 * @tparam Compare Strict weak ordering on `Value`; invocable as
 *                 `bool(const Value&, const Value&)`.
 *
 * @par Exception safety
 * Constructor: strong. `sort_file`: strong with respect to the output path
 * (the result is written to a temporary file and atomically renamed over the
 * destination only on success; the input file is never modified) and no
 * temporary files are leaked. The sorter's statistics are unspecified after a
 * failure.
 */
template <typename Value, typename Compare = std::less<Value>>
class external_merge_sorter {
  static_assert(std::is_trivially_copyable_v<Value>,
                "extsort: records are stored as raw bytes, so Value must be "
                "trivially copyable");
  static_assert(std::is_default_constructible_v<Value>,
                "extsort: Value must be default constructible");
  static_assert(
      std::is_invocable_r_v<bool, Compare &, const Value &, const Value &>,
      "extsort: Compare must be callable as bool(const Value&, "
      "const Value&)");

public:
  using value_type = Value;
  using compare_type = Compare;

  /**
   * @brief Validates the configuration and derives the sorting plan.
   *
   * @param cfg  Memory/block/fan-in parameters.
   * @param comp Strict weak ordering used to compare records.
   * @throws std::invalid_argument if the memory budget holds fewer than 4
   *         records or `max_fan_in` is `1`.
   * @throws std::filesystem::filesystem_error if the system temp directory
   *         cannot be determined.
   *
   * @section complexity_ctor Complexity
   * Time $O(1)$, auxiliary space $O(1)$.
   */
  explicit external_merge_sorter(config cfg = {}, Compare comp = Compare());

  /**
   * @brief Sorts a binary file of records into another file.
   *
   * `input` and `output` may be the same path: the input is fully consumed
   * before the output is atomically moved into place.
   *
   * @param input  Path of a file containing raw `Value` records.
   * @param output Path of the sorted result (created or replaced).
   * @return Statistics of this sort (also available via stats()).
   * @throws std::filesystem::filesystem_error if `input` does not exist.
   * @throws std::runtime_error on I/O failure or if the size of `input` is
   *         not a multiple of `sizeof(Value)`.
   * @throws Any exception thrown by `Compare` or by memory allocation.
   *
   * @section complexity_sort_file Complexity
   * - Comparisons: $O(n \log M)$ for run formation plus $O(n \log k)$ per
   *   merge pass, i.e. $O(n \log M + n \log k \cdot \lceil \log_k r \rceil)
   *   = O(n \log n)$ in the worst case.
   * - Block transfers: $\Theta\!\left(\frac{n}{B}(1 + P)\right) =
   *   O\!\left(\frac{n}{B}\log_{M/B}\frac{n}{B}\right)$ (each pass reads and
   *   writes the data once).
   * - Auxiliary internal space: $O(M)$ records (+ $O(k)$ for the heap).
   * - Auxiliary disk space: at most $2n$ records of temporary files
   *   ($n$ in the common case of at most one merge pass).
   * - Space is $O(1)$ when $n = 0$.
   */
  sort_stats sort_file(const std::filesystem::path &input,
                       const std::filesystem::path &output);

  /**
   * @brief Statistics of the most recent sort_file call.
   * @return Reference valid until the next call to sort_file.
   *
   * @section complexity_stats Complexity
   * Time $O(1)$, space $O(1)$.
   */
  [[nodiscard]] const sort_stats &stats() const noexcept { return stats_; }

  /// @brief Records per initial run ($M - B$); see @ref complexity_ctor.
  [[nodiscard]] std::size_t run_elems() const noexcept { return run_elems_; }
  /// @brief Block size $B$ in records after clamping to the memory budget.
  [[nodiscard]] std::size_t block_elems() const noexcept {
    return block_elems_;
  }
  /// @brief Effective merge fan-in $k$.
  [[nodiscard]] std::size_t fan_in() const noexcept { return fan_in_; }
  /// @brief Memory budget $M$ in records.
  [[nodiscard]] std::size_t memory_elems() const noexcept { return mem_elems_; }

private:
  /// Builds a unique temporary-file path inside `temp_dir_`.
  [[nodiscard]] std::filesystem::path make_temp_path() const;

  /// Phase 1. Writes `n` records either as one sorted file at `direct_out`
  /// (if everything fits in one run) or as runs appended to `runs`.
  void form_runs(const std::filesystem::path &input, std::uint64_t n,
                 const std::filesystem::path &direct_out,
                 std::vector<detail::temp_file> &runs);

  /// Phase 2 helper. Merges runs[first, last) into `dest` with a heap.
  void merge_group(const std::vector<detail::temp_file> &runs,
                   std::size_t first, std::size_t last,
                   const std::filesystem::path &dest);

  config cfg_;
  Compare comp_;
  std::filesystem::path temp_dir_;
  std::uint32_t salt_ = 0;
  std::size_t mem_elems_ = 0;
  std::size_t block_elems_ = 0;
  std::size_t run_elems_ = 0;
  std::size_t fan_in_ = 0;
  sort_stats stats_{};
};

/**
 * @brief Writes `n` records to a binary file (truncating it).
 *
 * @tparam Value Trivially copyable record type.
 * @param path Destination file.
 * @param data Pointer to the first record (may be null iff `n == 0`).
 * @param n    Number of records.
 * @throws std::runtime_error on I/O failure.
 *
 * @section complexity_write_records Complexity
 * Time $O(n)$, auxiliary space $O(1)$.
 */
template <typename Value>
void write_records(const std::filesystem::path &path, const Value *data,
                   std::size_t n);

/**
 * @brief Reads a whole binary file of records into memory.
 *
 * @tparam Value Trivially copyable, default constructible record type.
 * @param path Source file.
 * @return All records of the file in order.
 * @throws std::runtime_error on I/O failure or if the file size is not a
 *         multiple of `sizeof(Value)`.
 *
 * @section complexity_read_records Complexity
 * Time $O(n)$, auxiliary space $O(n)$ where $n$ is the record count.
 */
template <typename Value>
[[nodiscard]] std::vector<Value>
read_records(const std::filesystem::path &path);

} // namespace extsort

#include "external_merge_sort.tpp"
