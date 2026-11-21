/**
 * @file simple_graph.hpp
 * @brief Simplified undirected graph ADT (projects P-13.1, P-13.2, P-13.3).
 *
 * One class template, `simple_graph<V, Storage>`, implements the simplified
 * graph ADT; the `Storage` policy selects the representation:
 *
 *  - `adjacency_matrix_storage` -> `adjacency_matrix_graph` (P-13.1)
 *  - `adjacency_list_storage`   -> `adjacency_list_graph`   (P-13.2)
 *  - `edge_list_storage`        -> `edge_list_graph`        (P-13.3)
 *
 * The ADT is undirected, simple (no self-loops, no parallel edges), and
 * read-only after construction (no update functions).
 */
#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "adjacency_list_storage.hpp"
#include "adjacency_matrix_storage.hpp"
#include "edge_list_storage.hpp"
#include "graph_common.hpp"

namespace graph_adt {

/**
 * @brief Simplified, immutable, undirected simple graph.
 *
 * @tparam V        Vertex element type: copyable, hashable, equality
 *                  comparable. Vertex elements must be distinct.
 * @tparam Storage  Representation policy. Must be constructible from
 *                  `(std::size_t n, std::vector<edge_index> sorted_edges)` and
 *                  provide `num_edges`, `adjacent`, `degree`, `neighbors`,
 *                  `edges` (see the three provided storages).
 * @tparam Hash     Hash functor for `V`.
 * @tparam KeyEqual Equality functor for `V`.
 *
 * @par Ordering
 * `vertices()` is in construction order. `neighbors(v)` lists neighbours in the
 * construction order of the vertices. `edges()` lists each edge once as
 * `(u, v)` where `u` was inserted before `v`, ordered lexicographically by
 * vertex construction order.
 *
 * @par Variables used in complexity bounds
 * $n$ = number of vertices, $m$ = number of edges, $d(v)$ = degree of $v$.
 * Hashing or comparing one element costs $O(1)$; "expected" bounds assume a
 * good hash function.
 *
 * @par Thread safety
 * Immutable after construction: concurrent const access is safe.
 *
 * @par Exception safety
 * Constructors: strong (no object is created on failure). All queries:
 * no-throw guarantee except for the documented exceptions (and
 * `std::bad_alloc` for queries returning containers), and they never modify
 * the graph.
 */
template <typename V, typename Storage, typename Hash = std::hash<V>,
          typename KeyEqual = std::equal_to<V>>
class simple_graph {
  static_assert(
      std::is_constructible_v<Storage, std::size_t, std::vector<edge_index>>,
      "simple_graph: Storage must be constructible from (size_t, "
      "std::vector<edge_index>)");

public:
  using vertex_type = V;
  using edge_type = std::pair<V, V>;
  using size_type = std::size_t;

  /// @brief Creates the empty graph. Time $O(1)$.
  simple_graph() = default;

  /**
   * @brief Builds the graph represented by a vertex collection and an edge
   *        collection.
   * @param vertex_list Vertex elements, pairwise distinct.
   * @param edge_list   Edges as pairs of vertex elements (order inside a pair
   *                    is irrelevant).
   * @throws std::invalid_argument on a duplicate vertex, an edge endpoint that
   *         is not a vertex, a self-loop, or a duplicate edge
   *         (`{u, v}` and `{v, u}` are the same edge).
   * @section complexity
   * Time $O(n + m \log m)$ expected plus the storage build cost
   * ($O(n^2)$ matrix, $O(n + m)$ list/edge list); space $O(n + m)$.
   */
  simple_graph(std::initializer_list<V> vertex_list,
               std::initializer_list<edge_type> edge_list)
      : simple_graph(vertex_list.begin(), vertex_list.end(), edge_list.begin(),
                     edge_list.end()) {}

  /**
   * @brief Builds the graph from two iterator ranges.
   * @tparam VIt Input iterator whose values convert to `V`.
   * @tparam EIt Input iterator whose values are pair-like (`std::pair`,
   *             `std::tuple`, `std::array`) of two values convertible to `V`.
   * @param vertex_first,vertex_last Range of vertex elements.
   * @param edge_first,edge_last     Range of edges.
   * @throws std::invalid_argument as for the initializer-list constructor.
   * @section complexity
   * Same as the initializer-list constructor.
   */
  template <typename VIt, typename EIt,
            typename = std::void_t<
                typename std::iterator_traits<VIt>::value_type,
                typename std::iterator_traits<EIt>::value_type>>
  simple_graph(VIt vertex_first, VIt vertex_last, EIt edge_first,
               EIt edge_last);

  /**
   * @brief Builds the graph from two containers (any type with begin/end).
   * @param vertex_range Container of vertex elements.
   * @param edge_range   Container of pair-like edges.
   * @throws std::invalid_argument as for the initializer-list constructor.
   * @section complexity
   * Same as the initializer-list constructor.
   */
  template <typename VRange, typename ERange,
            typename = std::enable_if_t<detail::is_range_v<VRange> &&
                                        detail::is_range_v<ERange>>>
  simple_graph(const VRange &vertex_range, const ERange &edge_range)
      : simple_graph(std::begin(vertex_range), std::end(vertex_range),
                     std::begin(edge_range), std::end(edge_range)) {}

  /** @brief Number of vertices. @section complexity Time $O(1)$, space $O(1)$.
   */
  [[nodiscard]] size_type num_vertices() const noexcept {
    return index_.size();
  }

  /** @brief Number of edges. @section complexity Time $O(1)$, space $O(1)$. */
  [[nodiscard]] size_type num_edges() const noexcept {
    return storage_.num_edges();
  }

  /** @brief True iff there are no vertices. @section complexity Time $O(1)$. */
  [[nodiscard]] bool empty() const noexcept { return index_.size() == 0; }

  /**
   * @brief Vertex elements in construction order.
   * @section complexity
   * Time $O(1)$, space $O(1)$ (returns a reference).
   */
  [[nodiscard]] const std::vector<V> &vertices() const noexcept {
    return index_.values();
  }

  /**
   * @brief Tests whether @p v is a vertex.
   * @section complexity
   * Expected $O(1)$, worst case $O(n)$; space $O(1)$.
   */
  [[nodiscard]] bool contains_vertex(const V &v) const {
    return index_.contains(v);
  }

  /**
   * @brief Tests whether the edge `{u, v}` exists (symmetric). A vertex is not
   *        adjacent to itself.
   * @throws std::out_of_range if @p u or @p v is not a vertex.
   * @section complexity
   * Expected: matrix $O(1)$; adjacency list $O(\log \min(d(u), d(v)))$; edge
   * list $O(\log m)$. Space $O(1)$.
   */
  [[nodiscard]] bool are_adjacent(const V &u, const V &v) const;

  /**
   * @brief Degree of @p v.
   * @throws std::out_of_range if @p v is not a vertex.
   * @section complexity
   * Expected $O(1)$ for all three storages; space $O(1)$.
   */
  [[nodiscard]] size_type degree(const V &v) const;

  /**
   * @brief Neighbours of @p v in vertex construction order.
   * @throws std::out_of_range if @p v is not a vertex.
   * @section complexity
   * Expected: matrix $O(n)$; adjacency list $O(d(v))$; edge list $O(m)$.
   * Space $O(d(v))$ for the result.
   */
  [[nodiscard]] std::vector<V> neighbors(const V &v) const;

  /**
   * @brief All edges, each exactly once (see the class description for the
   *        orientation and order).
   * @section complexity
   * Time: matrix $O(n^2)$; adjacency list $O(n + m)$; edge list $O(m)$.
   * Space $O(m)$ for the result.
   */
  [[nodiscard]] std::vector<edge_type> edges() const;

private:
  vertex_index<V, Hash, KeyEqual> index_;
  Storage storage_;
};

/// @brief Simplified graph stored as an adjacency matrix (P-13.1).
template <typename V, typename Hash = std::hash<V>,
          typename KeyEqual = std::equal_to<V>>
using adjacency_matrix_graph =
    simple_graph<V, adjacency_matrix_storage, Hash, KeyEqual>;

/// @brief Simplified graph stored as adjacency lists (P-13.2).
template <typename V, typename Hash = std::hash<V>,
          typename KeyEqual = std::equal_to<V>>
using adjacency_list_graph =
    simple_graph<V, adjacency_list_storage, Hash, KeyEqual>;

/// @brief Simplified graph stored as an edge list (P-13.3).
template <typename V, typename Hash = std::hash<V>,
          typename KeyEqual = std::equal_to<V>>
using edge_list_graph = simple_graph<V, edge_list_storage, Hash, KeyEqual>;

} // namespace graph_adt

#include "simple_graph.tpp"
