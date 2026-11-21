/**
 * @file graph_common.hpp
 * @brief Shared helpers for the graph ADT implementations.
 *
 * Contains the dense vertex-index map used by the simplified (index based)
 * graph classes, a tiny rollback guard used to give the mutating operations of
 * the full graph a strong exception guarantee, and range-detection traits.
 */
#pragma once

#include <cstddef>
#include <functional>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace graph_adt {

/// @brief Undirected edge between two dense vertex indices, `first < second`.
using edge_index = std::pair<std::size_t, std::size_t>;

namespace detail {

/// @brief Detects types that support `std::begin` / `std::end`.
template <typename T, typename = void> struct is_range : std::false_type {};

template <typename T>
struct is_range<T, std::void_t<decltype(std::begin(std::declval<const T &>())),
                               decltype(std::end(std::declval<const T &>()))>>
    : std::true_type {};

template <typename T> inline constexpr bool is_range_v = is_range<T>::value;

/**
 * @brief Runs a callable on destruction unless dismissed.
 *
 * Used to undo partially completed mutations when a later step throws. The
 * callable must not throw.
 *
 * @tparam F Nullary callable type.
 */
template <typename F> class rollback_guard {
public:
  explicit rollback_guard(F undo) : undo_(std::move(undo)) {}
  rollback_guard(const rollback_guard &) = delete;
  rollback_guard &operator=(const rollback_guard &) = delete;
  ~rollback_guard() {
    if (armed_)
      undo_();
  }
  /// @brief Disarms the guard (the operation succeeded).
  void dismiss() noexcept { armed_ = false; }

private:
  F undo_;
  bool armed_ = true;
};

} // namespace detail

/**
 * @brief Bijection between vertex elements and dense indices `[0, n)`.
 *
 * Indices are assigned in insertion order and never change.
 *
 * @tparam V        Vertex element type (copyable).
 * @tparam Hash     Hash functor for `V`.
 * @tparam KeyEqual Equality functor for `V`.
 *
 * @par Thread safety
 * Concurrent const access is safe; any concurrent mutation requires external
 * synchronisation.
 *
 * @par Complexity notation
 * Hashing/comparing one element is assumed to cost $O(1)$.
 */
template <typename V, typename Hash = std::hash<V>,
          typename KeyEqual = std::equal_to<V>>
class vertex_index {
public:
  /**
   * @brief Number of stored elements.
   * @section complexity
   * Time $O(1)$, space $O(1)$.
   */
  [[nodiscard]] std::size_t size() const noexcept { return values_.size(); }

  /**
   * @brief All elements in insertion (index) order.
   * @section complexity
   * Time $O(1)$, space $O(1)$.
   */
  [[nodiscard]] const std::vector<V> &values() const noexcept {
    return values_;
  }

  /**
   * @brief Element stored at index @p i.
   * @pre `i < size()` (unchecked).
   * @section complexity
   * Time $O(1)$, space $O(1)$.
   */
  [[nodiscard]] const V &value(std::size_t i) const noexcept {
    return values_[i];
  }

  /**
   * @brief Tests whether @p v is stored.
   * @section complexity
   * Expected $O(1)$, worst case $O(n)$ ($n$ = size); space $O(1)$.
   */
  [[nodiscard]] bool contains(const V &v) const {
    return map_.find(v) != map_.end();
  }

  /**
   * @brief Index of @p v, if present.
   * @section complexity
   * Expected $O(1)$, worst case $O(n)$; space $O(1)$.
   */
  [[nodiscard]] std::optional<std::size_t> find(const V &v) const {
    const auto it = map_.find(v);
    if (it == map_.end())
      return std::nullopt;
    return it->second;
  }

  /**
   * @brief Index of @p v.
   * @throws std::out_of_range if @p v is not stored.
   * @section complexity
   * Expected $O(1)$, worst case $O(n)$; space $O(1)$.
   */
  [[nodiscard]] std::size_t at(const V &v) const {
    const auto it = map_.find(v);
    if (it == map_.end())
      throw std::out_of_range("graph_adt: unknown vertex");
    return it->second;
  }

  /**
   * @brief Appends @p v and returns its new index.
   * @throws std::invalid_argument if @p v is already stored.
   * @note Strong exception guarantee.
   * @section complexity
   * Expected amortised $O(1)$, worst case $O(n)$; space $O(1)$ amortised.
   */
  std::size_t add(const V &v) {
    const std::size_t idx = values_.size();
    auto [it, inserted] = map_.try_emplace(v, idx);
    if (!inserted)
      throw std::invalid_argument("graph_adt: duplicate vertex element");
    try {
      values_.push_back(v);
    } catch (...) {
      map_.erase(it);
      throw;
    }
    return idx;
  }

private:
  std::vector<V> values_;
  std::unordered_map<V, std::size_t, Hash, KeyEqual> map_;
};

} // namespace graph_adt
