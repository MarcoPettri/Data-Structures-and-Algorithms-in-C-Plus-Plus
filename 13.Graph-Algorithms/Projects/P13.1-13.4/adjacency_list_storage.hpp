/**
 * @file adjacency_list_storage.hpp
 * @brief Adjacency-list backend for `simple_graph` (project P-13.2).
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
 * @brief Immutable undirected simple graph stored as one sorted neighbour
 *        vector per vertex.
 *
 * Operates on dense indices `[0, n)`. Exposes no update functions.
 *
 * @par Variables used in complexity bounds
 * $n$ = number of vertices, $m$ = number of edges, $d(v)$ = degree of $v$.
 *
 * @par Thread safety
 * Immutable after construction: concurrent const access is safe.
 */
class adjacency_list_storage {
public:
  /// @brief Empty graph (no vertices).
  adjacency_list_storage() = default;

  /**
   * @brief Builds the adjacency lists.
   * @param n     Number of vertices.
   * @param edges Edges `(i, j)` with `i < j < n`, sorted lexicographically,
   *              without duplicates.
   * @note Strong exception guarantee.
   * @section complexity
   * Time $O(n + m)$, space $O(n + m)$.
   */
  adjacency_list_storage(std::size_t n, std::vector<edge_index> edges)
      : adj_(n), m_(edges.size()) {
    assert(std::is_sorted(edges.begin(), edges.end()));
    // Invariant: every row ends up sorted without an explicit sort. For vertex
    // v, edges (a, v) with a < v all precede edges (v, b) with b > v in the
    // lexicographic input, and inside each group the other endpoint ascends.
    for (const auto &[i, j] : edges) {
      assert(i < j && j < n);
      adj_[i].push_back(j);
      adj_[j].push_back(i);
    }
  }

  /** @brief Number of vertices. @section complexity Time $O(1)$, space $O(1)$.
   */
  [[nodiscard]] std::size_t num_vertices() const noexcept {
    return adj_.size();
  }

  /** @brief Number of edges. @section complexity Time $O(1)$, space $O(1)$. */
  [[nodiscard]] std::size_t num_edges() const noexcept { return m_; }

  /**
   * @brief Adjacency test (binary search in the shorter list).
   * @param i,j Vertex indices, both `< n` (unchecked). `i == j` yields false.
   * @section complexity
   * Time $O(\log \min(d(i), d(j)))$ worst case, space $O(1)$.
   */
  [[nodiscard]] bool adjacent(std::size_t i, std::size_t j) const noexcept {
    const auto &a = adj_[i];
    const auto &b = adj_[j];
    return a.size() <= b.size() ? std::binary_search(a.begin(), a.end(), j)
                                : std::binary_search(b.begin(), b.end(), i);
  }

  /**
   * @brief Degree of vertex @p i.
   * @param i Vertex index `< n` (unchecked).
   * @section complexity
   * Time $O(1)$, space $O(1)$.
   */
  [[nodiscard]] std::size_t degree(std::size_t i) const noexcept {
    return adj_[i].size();
  }

  /**
   * @brief Neighbours of @p i in ascending index order.
   * @param i Vertex index `< n` (unchecked).
   * @section complexity
   * Time $O(d(i))$, space $O(d(i))$ for the result.
   */
  [[nodiscard]] std::vector<std::size_t> neighbors(std::size_t i) const {
    return adj_[i];
  }

  /**
   * @brief All edges `(i, j)`, `i < j`, in lexicographic order.
   * @section complexity
   * Time $O(n + m)$, space $O(m)$ for the result.
   */
  [[nodiscard]] std::vector<edge_index> edges() const {
    std::vector<edge_index> out;
    out.reserve(m_);
    for (std::size_t i = 0; i < adj_.size(); ++i)
      for (const std::size_t j : adj_[i])
        if (j > i)
          out.emplace_back(i, j);
    return out;
  }

private:
  std::vector<std::vector<std::size_t>> adj_;
  std::size_t m_ = 0;
};

} // namespace graph_adt
