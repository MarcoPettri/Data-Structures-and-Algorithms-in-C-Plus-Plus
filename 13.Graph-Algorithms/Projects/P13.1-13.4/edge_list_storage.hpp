/**
 * @file edge_list_storage.hpp
 * @brief Edge-list backend for `simple_graph` (project P-13.3).
 */
#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <utility>
#include <vector>

#include "graph_common.hpp"

namespace graph_adt {

/**
 * @brief Immutable undirected simple graph stored as a vertex count plus a
 *        single sequence of edges kept in lexicographic order.
 *
 * Keeping the edge sequence sorted (still a plain edge list) lets adjacency
 * tests use binary search instead of a linear scan. Per-vertex degrees are
 * cached. Operates on dense indices `[0, n)`; exposes no update functions.
 *
 * @par Variables used in complexity bounds
 * $n$ = number of vertices, $m$ = number of edges, $d(v)$ = degree of $v$.
 *
 * @par Thread safety
 * Immutable after construction: concurrent const access is safe.
 */
class edge_list_storage {
public:
  /// @brief Empty graph (no vertices).
  edge_list_storage() = default;

  /**
   * @brief Builds the edge list.
   * @param n     Number of vertices.
   * @param edges Edges `(i, j)` with `i < j < n`, sorted lexicographically,
   *              without duplicates.
   * @note Strong exception guarantee.
   * @section complexity
   * Time $O(n + m)$, space $O(n + m)$.
   */
  edge_list_storage(std::size_t n, std::vector<edge_index> edges)
      : n_(n), edges_(std::move(edges)), degree_(n, 0) {
    assert(std::is_sorted(edges_.begin(), edges_.end()));
    for (const auto &[i, j] : edges_) {
      assert(i < j && j < n_);
      ++degree_[i];
      ++degree_[j];
    }
  }

  /** @brief Number of vertices. @section complexity Time $O(1)$, space $O(1)$.
   */
  [[nodiscard]] std::size_t num_vertices() const noexcept { return n_; }

  /** @brief Number of edges. @section complexity Time $O(1)$, space $O(1)$. */
  [[nodiscard]] std::size_t num_edges() const noexcept { return edges_.size(); }

  /**
   * @brief Adjacency test (binary search in the sorted edge list).
   * @param i,j Vertex indices, both `< n` (unchecked). `i == j` yields false.
   * @section complexity
   * Time $O(\log m)$ worst case, space $O(1)$.
   */
  [[nodiscard]] bool adjacent(std::size_t i, std::size_t j) const noexcept {
    const edge_index key{std::min(i, j), std::max(i, j)};
    return std::binary_search(edges_.begin(), edges_.end(), key);
  }

  /**
   * @brief Degree of vertex @p i.
   * @param i Vertex index `< n` (unchecked).
   * @section complexity
   * Time $O(1)$ (cached), space $O(1)$.
   */
  [[nodiscard]] std::size_t degree(std::size_t i) const noexcept {
    return degree_[i];
  }

  /**
   * @brief Neighbours of @p i in ascending index order.
   * @param i Vertex index `< n` (unchecked).
   * @section complexity
   * Time $O(m)$ worst case, space $O(d(i))$ for the result.
   */
  [[nodiscard]] std::vector<std::size_t> neighbors(std::size_t i) const {
    std::vector<std::size_t> out;
    out.reserve(degree_[i]);
    // Edges (a, i) with a < i come first (ascending a), then edges (i, b) with
    // b > i (ascending b); the output is therefore already sorted. Once the
    // first endpoint exceeds i nothing else can match.
    for (const auto &[a, b] : edges_) {
      if (a > i)
        break;
      if (b == i)
        out.push_back(a);
      else if (a == i)
        out.push_back(b);
    }
    return out;
  }

  /**
   * @brief All edges `(i, j)`, `i < j`, in lexicographic order.
   * @section complexity
   * Time $O(m)$, space $O(m)$ for the result.
   */
  [[nodiscard]] std::vector<edge_index> edges() const { return edges_; }

private:
  std::size_t n_ = 0;
  std::vector<edge_index> edges_; // sorted lexicographically, i < j
  std::vector<std::size_t> degree_;
};

} // namespace graph_adt
