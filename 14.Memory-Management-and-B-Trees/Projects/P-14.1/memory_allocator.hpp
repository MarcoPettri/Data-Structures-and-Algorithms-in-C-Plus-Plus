/**
 * @file memory_allocator.hpp
 * @brief Simulated contiguous-memory allocator with first-fit, next-fit,
 *        best-fit and worst-fit placement policies.
 *
 * @details
 * The allocator manages the abstract address range [0, capacity). Free space
 * is tracked as a set of maximal, pairwise non-adjacent "holes", indexed twice:
 * by address (std::map) and by (length, address) (std::set). Live blocks are
 * kept in a hash table keyed by start address.
 *
 * Notation used in all complexity bounds:
 *  - $h$ = current number of holes,
 *  - $a$ = current number of live allocations.
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace memsim {

/** @brief Hole-selection strategy. */
enum class fit_policy { first_fit, next_fit, best_fit, worst_fit };

/** @brief Human-readable policy name. */
[[nodiscard]] constexpr const char *to_string(fit_policy policy) noexcept {
  switch (policy) {
  case fit_policy::first_fit:
    return "first-fit";
  case fit_policy::next_fit:
    return "next-fit";
  case fit_policy::best_fit:
    return "best-fit";
  case fit_policy::worst_fit:
    return "worst-fit";
  }
  return "unknown";
}

/** @brief All policies, for iteration in experiments and tests. */
inline constexpr std::array<fit_policy, 4> all_fit_policies{
    {fit_policy::first_fit, fit_policy::next_fit, fit_policy::best_fit,
     fit_policy::worst_fit}};

/**
 * @brief Snapshot of allocator state.
 * @tparam SizeType Unsigned integer type used for sizes and addresses.
 */
template <typename SizeType = std::size_t> struct memory_stats {
  SizeType capacity{};
  SizeType used_bytes{};
  SizeType free_bytes{};
  SizeType largest_hole{};
  std::size_t hole_count{};
  std::size_t live_blocks{};
  /// $1 - \text{largest\_hole}/\text{free\_bytes}$, or 0 when nothing is free.
  double external_fragmentation{};
};

/**
 * @brief Simulates a fixed-size contiguous heap under a chosen fit policy.
 *
 * @tparam SizeType Unsigned integer type used for sizes and addresses; it
 *         models the width of the simulated address space.
 *
 * Exception safety: allocate() and deallocate() give the strong guarantee
 * (a thrown exception leaves the object unchanged). Other members are
 * no-throw except check_invariants(), which may throw std::bad_alloc.
 */
template <typename SizeType = std::size_t> class memory_allocator {
  static_assert(std::is_integral_v<SizeType> && std::is_unsigned_v<SizeType> &&
                    !std::is_same_v<SizeType, bool>,
                "memory_allocator: SizeType must be an unsigned integer type");

public:
  using size_type = SizeType;
  using address_type = SizeType;

  /**
   * @brief Creates an allocator whose memory is a single hole.
   * @param capacity Total number of addressable units; must be positive.
   * @param policy   Placement policy used by allocate().
   * @throws std::invalid_argument if @p capacity is zero.
   * @throws std::bad_alloc if the initial hole cannot be stored.
   *
   * @section complexity Complexity
   * - Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  memory_allocator(size_type capacity, fit_policy policy);

  /**
   * @brief Allocates a block of @p size units using the configured policy.
   * @param size Requested size; must be positive.
   * @return Start address of the block, or std::nullopt if no hole is large
   *         enough (the allocator is then unchanged).
   * @throws std::invalid_argument if @p size is zero.
   * @throws std::bad_alloc (strong guarantee).
   *
   * @section complexity Complexity
   * - first-fit, next-fit: worst-case $O(h)$ time (linear scan of the holes);
   *   the scan stops at the first adequate hole, and requests larger than the
   *   largest hole are rejected in $O(1)$.
   * - best-fit, worst-fit: $O(\log h)$ time (ordered-index lookup).
   * - All policies add expected $O(1)$ for the hash-table insertion and
   *   $O(\log h)$ for re-linking the carved hole.
   * - Auxiliary space: $O(1)$ (one hash node).
   */
  [[nodiscard]] std::optional<address_type> allocate(size_type size);

  /**
   * @brief Frees the block starting at @p address and coalesces neighbours.
   * @param address Start address previously returned by allocate().
   * @throws std::invalid_argument if @p address is not a live block start
   *         (this includes double frees).
   * @throws std::bad_alloc (strong guarantee).
   *
   * @section complexity Complexity
   * - Time: expected $O(\log h)$ (ordered-index operations plus expected
   *   $O(1)$ hash lookup and erase).
   * - Auxiliary space: $O(1)$.
   */
  void deallocate(address_type address);

  /**
   * @brief Size of the live block starting at @p address, if any.
   * @section complexity Complexity
   * - Time: expected $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] std::optional<size_type>
  block_size(address_type address) const noexcept;

  /** @brief Total capacity. $O(1)$. */
  [[nodiscard]] size_type capacity() const noexcept { return capacity_; }
  /** @brief Configured policy. $O(1)$. */
  [[nodiscard]] fit_policy policy() const noexcept { return policy_; }
  /** @brief Units currently allocated. $O(1)$. */
  [[nodiscard]] size_type used_bytes() const noexcept { return used_; }
  /** @brief Units currently free (sum of all holes). $O(1)$. */
  [[nodiscard]] size_type free_bytes() const noexcept {
    return static_cast<size_type>(capacity_ - used_);
  }
  /** @brief Number of holes $h$. $O(1)$. */
  [[nodiscard]] std::size_t hole_count() const noexcept {
    return holes_.size();
  }
  /** @brief Number of live allocations $a$. $O(1)$. */
  [[nodiscard]] std::size_t live_blocks() const noexcept {
    return allocated_.size();
  }
  /** @brief Length of the largest hole (0 if none). $O(1)$. */
  [[nodiscard]] size_type largest_hole() const noexcept {
    return by_size_.empty() ? size_type{0} : by_size_.rbegin()->first;
  }

  /** @brief Consistent snapshot of the counters above. $O(1)$. */
  [[nodiscard]] memory_stats<size_type> stats() const noexcept;

  /**
   * @brief Verifies every internal invariant (for tests).
   * @return true iff holes are coalesced, both hole indexes agree, and holes
   *         plus live blocks tile [0, capacity) exactly.
   * @throws std::bad_alloc
   *
   * @section complexity Complexity
   * - Time: $O((h + a)\log(h + a))$. Auxiliary space: $O(h + a)$.
   */
  [[nodiscard]] bool check_invariants() const;

private:
  using hole_map = std::map<address_type, size_type>;
  using hole_key = std::pair<size_type, address_type>; // (length, address)
  using size_index = std::set<hole_key>;
  using hole_iterator = typename hole_map::iterator;

  [[nodiscard]] hole_iterator find_hole(size_type size);
  [[nodiscard]] hole_iterator find_first_fit(size_type size);
  [[nodiscard]] hole_iterator find_next_fit(size_type size);
  [[nodiscard]] hole_iterator find_best_fit(size_type size);
  [[nodiscard]] hole_iterator find_worst_fit(size_type size);

  size_type capacity_;
  fit_policy policy_;
  size_type used_{0};
  /// Next-fit resume position: the byte right after the last allocation.
  address_type cursor_{0};
  hole_map holes_;     ///< address -> length of each hole
  size_index by_size_; ///< (length, address) of each hole
  std::unordered_map<address_type, size_type> allocated_;
};

} // namespace memsim

#include "memory_allocator.tpp"
