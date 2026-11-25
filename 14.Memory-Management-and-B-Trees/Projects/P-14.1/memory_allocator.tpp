/**
 * @file memory_allocator.tpp
 * @brief Out-of-class definitions for memsim::memory_allocator.
 *
 * @details
 * Included from the bottom of memory_allocator.hpp.
 * Exception-safety summary (see the Doxygen on the declarations for details):
 *  - allocate():   strong guarantee. The only potentially throwing step (the
 *                  insertion into the allocation table) happens before any
 *                  other state is touched; everything afterwards re-links
 *                  existing tree nodes via node handles and cannot throw.
 *  - deallocate(): strong guarantee. The non-merging path rolls back its first
 *                  insertion if the second one throws; the merging paths only
 *                  re-link existing nodes.
 */
#pragma once

#include <algorithm>
#include <iterator>
#include <vector>

namespace memsim {

template <typename SizeType>
memory_allocator<SizeType>::memory_allocator(size_type capacity,
                                             fit_policy policy)
    : capacity_(capacity), policy_(policy) {
  if (capacity == 0)
    throw std::invalid_argument("memory_allocator: capacity must be positive");
  holes_.emplace(address_type{0}, capacity);
  by_size_.emplace(capacity, address_type{0});
}

// ---------------------------------------------------------------------------
// Hole search strategies
// ---------------------------------------------------------------------------

template <typename SizeType>
typename memory_allocator<SizeType>::hole_iterator
memory_allocator<SizeType>::find_first_fit(size_type size) {
  for (auto it = holes_.begin(); it != holes_.end(); ++it)
    if (it->second >= size)
      return it;
  return holes_.end();
}

template <typename SizeType>
typename memory_allocator<SizeType>::hole_iterator
memory_allocator<SizeType>::find_next_fit(size_type size) {
  // Resume at the first hole whose END lies beyond the cursor. That is either
  // the hole straddling the cursor (it may have grown through coalescing since
  // the cursor was set) or the first hole after it.
  auto start = holes_.upper_bound(cursor_);
  if (start != holes_.begin()) {
    const auto before = std::prev(start);
    if (static_cast<std::uint64_t>(before->first) + before->second > cursor_)
      start = before;
  }
  for (auto it = start; it != holes_.end(); ++it)
    if (it->second >= size)
      return it;
  for (auto it = holes_.begin(); it != start; ++it)
    if (it->second >= size)
      return it;
  return holes_.end();
}

template <typename SizeType>
typename memory_allocator<SizeType>::hole_iterator
memory_allocator<SizeType>::find_best_fit(size_type size) {
  // Smallest hole with length >= size; ties resolved by lowest address because
  // the index is ordered by (length, address).
  const auto it = by_size_.lower_bound(hole_key{size, address_type{0}});
  return it == by_size_.end() ? holes_.end() : holes_.find(it->second);
}

template <typename SizeType>
typename memory_allocator<SizeType>::hole_iterator
memory_allocator<SizeType>::find_worst_fit(size_type size) {
  if (by_size_.empty())
    return holes_.end();
  const size_type largest = by_size_.rbegin()->first;
  if (largest < size)
    return holes_.end();
  // Lowest-addressed hole among those of maximal length (deterministic ties).
  const auto it = by_size_.lower_bound(hole_key{largest, address_type{0}});
  return holes_.find(it->second);
}

template <typename SizeType>
typename memory_allocator<SizeType>::hole_iterator
memory_allocator<SizeType>::find_hole(size_type size) {
  switch (policy_) {
  case fit_policy::first_fit:
    return find_first_fit(size);
  case fit_policy::next_fit:
    return find_next_fit(size);
  case fit_policy::best_fit:
    return find_best_fit(size);
  case fit_policy::worst_fit:
    return find_worst_fit(size);
  }
  return holes_.end();
}

// ---------------------------------------------------------------------------
// allocate / deallocate
// ---------------------------------------------------------------------------

template <typename SizeType>
std::optional<SizeType> memory_allocator<SizeType>::allocate(size_type size) {
  if (size == 0)
    throw std::invalid_argument(
        "memory_allocator::allocate: size must be positive");

  // O(1) rejection that is valid for every policy: no hole is big enough.
  if (size > largest_hole())
    return std::nullopt;

  const hole_iterator hole = find_hole(size);
  if (hole == holes_.end()) // unreachable because of the check above
    return std::nullopt;

  const address_type address = hole->first;
  const size_type hole_size = hole->second;

  // The only step that may throw (hash-table node allocation). Nothing else
  // has been modified yet, so a failure leaves the allocator untouched.
  allocated_.emplace(address, size);

  // From here on nothing can throw: we only re-link existing tree nodes.
  auto size_node = by_size_.extract(hole_key{hole_size, address});
  auto hole_node = holes_.extract(hole);
  if (hole_size > size) {
    // Carve the block from the front of the hole; the remainder keeps its
    // nodes but moves to a new key.
    const auto rest_address = static_cast<address_type>(address + size);
    const auto rest_size = static_cast<size_type>(hole_size - size);
    hole_node.key() = rest_address;
    hole_node.mapped() = rest_size;
    size_node.value() = hole_key{rest_size, rest_address};
    holes_.insert(std::move(hole_node));
    by_size_.insert(std::move(size_node));
  }
  // else: exact fit, both nodes are simply destroyed (hole disappears).

  used_ = static_cast<size_type>(used_ + size);
  cursor_ = static_cast<address_type>(address + size);
  return address;
}

template <typename SizeType>
void memory_allocator<SizeType>::deallocate(address_type address) {
  const auto block = allocated_.find(address);
  if (block == allocated_.end())
    throw std::invalid_argument("memory_allocator::deallocate: address is not "
                                "the start of a live allocation");
  const size_type size = block->second;

  // `address` is allocated, so no hole starts at it: upper_bound finds the
  // nearest hole strictly after the block, and its predecessor is the nearest
  // hole strictly before it.
  const auto next = holes_.upper_bound(address);
  const bool has_prev = next != holes_.begin();
  const auto prev = has_prev ? std::prev(next) : holes_.end();

  const bool merge_prev =
      has_prev &&
      static_cast<std::uint64_t>(prev->first) + prev->second == address;
  const bool merge_next =
      next != holes_.end() &&
      static_cast<std::uint64_t>(address) + size == next->first;

  if (merge_prev && merge_next) {
    const auto merged =
        static_cast<size_type>(prev->second + size + next->second);
    auto prev_node = by_size_.extract(hole_key{prev->second, prev->first});
    by_size_.erase(hole_key{next->second, next->first});
    prev_node.value().first = merged;
    by_size_.insert(std::move(prev_node));
    prev->second = merged;
    holes_.erase(next);
  } else if (merge_prev) {
    const auto merged = static_cast<size_type>(prev->second + size);
    auto node = by_size_.extract(hole_key{prev->second, prev->first});
    node.value().first = merged;
    by_size_.insert(std::move(node));
    prev->second = merged;
  } else if (merge_next) {
    const auto merged = static_cast<size_type>(size + next->second);
    auto size_node = by_size_.extract(hole_key{next->second, next->first});
    size_node.value() = hole_key{merged, address};
    by_size_.insert(std::move(size_node));
    auto hole_node = holes_.extract(next);
    hole_node.key() = address;
    hole_node.mapped() = merged;
    holes_.insert(std::move(hole_node));
  } else {
    // Isolated block: a brand-new hole. Two allocating insertions, with
    // rollback of the first if the second throws.
    const auto [it, inserted] = holes_.emplace(address, size);
    (void)inserted;
    try {
      by_size_.emplace(size, address);
    } catch (...) {
      holes_.erase(it);
      throw;
    }
  }

  used_ = static_cast<size_type>(used_ - size);
  allocated_.erase(block);
}

// ---------------------------------------------------------------------------
// Introspection
// ---------------------------------------------------------------------------

template <typename SizeType>
std::optional<SizeType>
memory_allocator<SizeType>::block_size(address_type address) const noexcept {
  const auto it = allocated_.find(address);
  if (it == allocated_.end())
    return std::nullopt;
  return it->second;
}

template <typename SizeType>
memory_stats<SizeType> memory_allocator<SizeType>::stats() const noexcept {
  memory_stats<size_type> s;
  s.capacity = capacity_;
  s.used_bytes = used_;
  s.free_bytes = static_cast<size_type>(capacity_ - used_);
  s.largest_hole = largest_hole();
  s.hole_count = holes_.size();
  s.live_blocks = allocated_.size();
  s.external_fragmentation = s.free_bytes == 0
                                 ? 0.0
                                 : 1.0 - static_cast<double>(s.largest_hole) /
                                             static_cast<double>(s.free_bytes);
  return s;
}

template <typename SizeType>
bool memory_allocator<SizeType>::check_invariants() const {
  if (holes_.size() != by_size_.size() || cursor_ > capacity_ ||
      used_ > capacity_)
    return false;

  std::vector<std::pair<std::uint64_t, std::uint64_t>> tiles;
  tiles.reserve(holes_.size() + allocated_.size());

  std::uint64_t free_total = 0;
  bool have_prev = false;
  std::uint64_t prev_end = 0;
  for (const auto &[address, size] : holes_) {
    if (size == 0)
      return false;
    // Holes must be disjoint AND non-adjacent (fully coalesced).
    if (have_prev && prev_end >= address)
      return false;
    if (by_size_.count(hole_key{size, address}) != 1)
      return false;
    prev_end = static_cast<std::uint64_t>(address) + size;
    have_prev = true;
    free_total += size;
    tiles.emplace_back(address, size);
  }

  std::uint64_t used_total = 0;
  for (const auto &[address, size] : allocated_) {
    if (size == 0)
      return false;
    used_total += size;
    tiles.emplace_back(address, size);
  }
  if (used_total != used_ || free_total + used_total != capacity_)
    return false;

  // Holes + allocations must tile [0, capacity) exactly: no gap, no overlap.
  std::sort(tiles.begin(), tiles.end());
  std::uint64_t expected = 0;
  for (const auto &[address, size] : tiles) {
    if (address != expected)
      return false;
    expected += size;
  }
  return expected == capacity_;
}

} // namespace memsim