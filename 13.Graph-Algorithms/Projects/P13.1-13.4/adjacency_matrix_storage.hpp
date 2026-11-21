/**
 * @file adjacency_matrix_storage.hpp
 * @brief Adjacency-matrix backend for `simple_graph` (project P-13.1).
 */
#pragma once

#include <cassert>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#include "graph_common.hpp"

namespace graph_adt {

/**
 * @brief Immutable undirected simple graph stored as an $n \times n$ bit
 *        matrix (symmetric, both triangles stored).
 *
 * Operates on dense indices `[0, n)`; `simple_graph` maps vertex elements to
 * indices. Exposes no update functions.
 *
 * @par Variables used in complexity bounds
 * $n$ = number of vertices, $m$ = number of edges, $d(v)$ = degree of $v$.
 *
 * @par Thread safety
 * Immutable after construction: concurrent const access is safe.
 */
class adjacency_matrix_storage {
public:
  /// @brief Empty graph (no vertices).
  adjacency_matrix_storage() = default;

  /**
   * @brief Builds the matrix.
   * @param n     Number of vertices.
   * @param edges Edges `(i, j)` with `i < j < n`, sorted, without duplicates.
   * @throws std::length_error if $n^2$ overflows `std::size_t`.
   * @throws std::bad_alloc if the matrix cannot be allocated.
   * @note Strong exception guarantee (nothing is constructed on failure).
   * @section complexity
   * Time $O(n^2 + m)$, space $O(n^2)$ bits $+ O(n)$ words.
   */
  adjacency_matrix_storage(std::size_t n, std::vector<edge_index> edges)
      : n_(n), m_(edges.size()), degree_(n, 0) {
    if (n_ != 0 && n_ > std::numeric_limits<std::size_t>::max() / n_)
      throw std::length_error("adjacency_matrix_storage: n * n overflows");
    bits_.assign(n_ * n_, false);
    for (const auto &[i, j] : edges) {
      assert(i < j && j < n_);
      bits_[i * n_ + j] = true;
      bits_[j * n_ + i] = true;
      ++degree_[i];
      ++degree_[j];
    }
  }

  /** @brief Number of vertices. @section complexity Time $O(1)$, space $O(1)$.
   */
  [[nodiscard]] std::size_t num_vertices() const noexcept { return n_; }

  /** @brief Number of edges. @section complexity Time $O(1)$, space $O(1)$. */
  [[nodiscard]] std::size_t num_edges() const noexcept { return m_; }

  /**
   * @brief Adjacency test.
   * @param i,j Vertex indices, both `< n` (unchecked). `i == j` yields false.
   * @section complexity
   * Time $O(1)$, space $O(1)$.
   */
  [[nodiscard]] bool adjacent(std::size_t i, std::size_t j) const noexcept {
    return bits_[i * n_ + j];
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
   * Time $O(n)$ (scans one row), space $O(d(i))$ for the result.
   */
  [[nodiscard]] std::vector<std::size_t> neighbors(std::size_t i) const {
    std::vector<std::size_t> out;
    out.reserve(degree_[i]);
    for (std::size_t j = 0; j < n_; ++j)
      if (bits_[i * n_ + j])
        out.push_back(j);
    return out;
  }

  /**
   * @brief All edges `(i, j)`, `i < j`, in lexicographic order.
   * @section complexity
   * Time $O(n^2)$, space $O(m)$ for the result.
   */
  [[nodiscard]] std::vector<edge_index> edges() const {
    std::vector<edge_index> out;
    out.reserve(m_);
    for (std::size_t i = 0; i < n_; ++i)
      for (std::size_t j = i + 1; j < n_; ++j)
        if (bits_[i * n_ + j])
          out.emplace_back(i, j);
    return out;
  }

private:
  std::size_t n_ = 0;
  std::size_t m_ = 0;
  std::vector<std::size_t> degree_; // cached degrees
  std::vector<bool> bits_;          // row-major n x n, symmetric
};

} // namespace graph_adt
