/**
 * @file graph.hpp
 * @brief Full graph ADT with directed and undirected edges (project P-13.4).
 *
 * `graph<V, E>` extends the adjacency-list idea of P-13.2 to the complete,
 * position-based graph ADT: vertices and edges carry elements, are addressed
 * through opaque handles, and may be inserted, removed, and (for edges)
 * re-directed. Directed and undirected edges can coexist (mixed graph), and
 * parallel edges are allowed. Self-loops are rejected.
 */
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <list>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "graph_common.hpp"

namespace graph_adt {

/// @brief Default (empty) edge element for graphs whose edges carry no data.
struct no_element {
  friend constexpr bool operator==(no_element, no_element) noexcept {
    return true;
  }
  friend constexpr bool operator!=(no_element, no_element) noexcept {
    return false;
  }
};

namespace detail {
/// @brief Process-wide unique, never reused, non-zero identifier.
inline std::uint64_t next_object_id() noexcept {
  static std::atomic<std::uint64_t> counter{1};
  return counter.fetch_add(1, std::memory_order_relaxed);
}
} // namespace detail

struct vertex_tag;
struct edge_tag;

template <typename V, typename E> class graph;

/**
 * @brief Opaque, copyable reference to a vertex or an edge of a `graph`.
 *
 * Identifiers are unique across all graphs and never reused, so a handle that
 * is stale (its object was removed), foreign (from another graph) or
 * default-constructed is detected and reported with `std::invalid_argument`
 * rather than causing undefined behaviour.
 *
 * @tparam Tag `vertex_tag` or `edge_tag`.
 */
template <typename Tag> class handle {
public:
  /// @brief Creates the null handle (never valid).
  constexpr handle() noexcept = default;

  /// @brief True iff this is not the null handle (says nothing about
  ///        staleness).
  [[nodiscard]] constexpr bool valid() const noexcept { return id_ != 0; }

  /// @brief Unique identifier (0 for the null handle).
  [[nodiscard]] constexpr std::uint64_t id() const noexcept { return id_; }

  friend constexpr bool operator==(handle a, handle b) noexcept {
    return a.id_ == b.id_;
  }
  friend constexpr bool operator!=(handle a, handle b) noexcept {
    return a.id_ != b.id_;
  }
  friend constexpr bool operator<(handle a, handle b) noexcept {
    return a.id_ < b.id_;
  }

private:
  template <typename, typename> friend class graph;
  constexpr explicit handle(std::uint64_t id) noexcept : id_(id) {}
  std::uint64_t id_ = 0;
};

using vertex_handle = handle<vertex_tag>;
using edge_handle = handle<edge_tag>;

} // namespace graph_adt

namespace std {
template <typename Tag> struct hash<graph_adt::handle<Tag>> {
  size_t operator()(graph_adt::handle<Tag> h) const noexcept {
    return hash<std::uint64_t>{}(h.id());
  }
};
} // namespace std

namespace graph_adt {

/**
 * @brief Mixed (directed + undirected) multigraph without self-loops, stored
 *        with incidence lists.
 *
 * Every vertex owns a list of its incident edges; every edge remembers its
 * position in both endpoint lists, so removing an edge is $O(1)$. Lookups by
 * handle go through hash maps (expected $O(1)$).
 *
 * @tparam V Vertex element type (movable).
 * @tparam E Edge element type (movable; default constructible only if the
 *           default `element` arguments or the collection constructor are
 *           used).
 *
 * @par Edge orientation
 * Every edge stores an ordered endpoint pair `(first, second)`. For a directed
 * edge, `first` is the origin and `second` the destination. For an undirected
 * edge the pair is just the insertion order and carries no meaning, except
 * that it is remembered if the edge later becomes directed.
 *
 * @par In/out semantics
 * An undirected edge counts as both an outgoing and an incoming edge of each
 * of its endpoints. `degree(v)` counts every incident edge once.
 *
 * @par Variables used in complexity bounds
 * $n$ = number of vertices, $m$ = number of edges, $d(v)$ = number of edges
 * incident to $v$. "Expected" bounds come from hash-map lookups of handles.
 *
 * @par Thread safety
 * Not thread-safe for concurrent mutation. Concurrent calls to const member
 * functions are safe provided no thread mutates the graph meanwhile. Distinct
 * graph objects may be used from different threads (identifier generation is
 * atomic).
 *
 * @par Exception safety
 * Unless stated otherwise: handle errors throw `std::invalid_argument` and
 * leave the graph unchanged (strong). Insertions give the strong guarantee.
 * Removals and direction changes are no-throw after argument validation (the
 * element-returning removals are strong if `V`/`E` have a non-throwing move
 * constructor).
 *
 * @par Copying
 * Copying is disabled (handles identify objects of exactly one graph). Moving
 * keeps all handles valid for the destination and leaves the source empty.
 */
template <typename V, typename E = no_element> class graph {
public:
  using vertex_type = V;
  using edge_element_type = E;
  using size_type = std::size_t;

  /// @brief Creates the empty graph. Time $O(1)$.
  graph() = default;

  /**
   * @brief Builds an undirected graph from two iterator ranges (the same
   *        constructor contract as the simplified ADT).
   * @tparam VIt Input iterator whose values convert to `V`.
   * @tparam EIt Input iterator whose values are pair-like of two values
   *             convertible to `V`.
   * @param vertex_first,vertex_last Vertex elements, pairwise distinct.
   * @param edge_first,edge_last     Edges as pairs of vertex elements; each
   *                                 becomes an undirected edge with a
   *                                 value-initialised `E`. Parallel edges in
   *                                 the input are kept as parallel edges.
   * @throws std::invalid_argument on a duplicate vertex element, an unknown
   *         endpoint, or a self-loop.
   * @note Requires `std::hash<V>`, `operator==` on `V`, and default
   *       constructible `E`.
   * @section complexity
   * Time $O(n + m)$ expected, space $O(n + m)$.
   */
  template <typename VIt, typename EIt,
            typename = std::void_t<
                typename std::iterator_traits<VIt>::value_type,
                typename std::iterator_traits<EIt>::value_type>>
  graph(VIt vertex_first, VIt vertex_last, EIt edge_first, EIt edge_last);

  /**
   * @brief Builds an undirected graph from initializer lists.
   * @throws std::invalid_argument see the iterator-range constructor.
   * @section complexity
   * Time $O(n + m)$ expected, space $O(n + m)$.
   */
  graph(std::initializer_list<V> vertex_list,
        std::initializer_list<std::pair<V, V>> edge_list)
      : graph(vertex_list.begin(), vertex_list.end(), edge_list.begin(),
              edge_list.end()) {}

  /**
   * @brief Builds an undirected graph from two containers.
   * @throws std::invalid_argument see the iterator-range constructor.
   * @section complexity
   * Time $O(n + m)$ expected, space $O(n + m)$.
   */
  template <typename VRange, typename ERange,
            typename = std::enable_if_t<detail::is_range_v<VRange> &&
                                        detail::is_range_v<ERange>>>
  graph(const VRange &vertex_range, const ERange &edge_range)
      : graph(std::begin(vertex_range), std::end(vertex_range),
              std::begin(edge_range), std::end(edge_range)) {}

  graph(const graph &) = delete;
  graph &operator=(const graph &) = delete;

  /**
   * @brief Move constructor; @p other becomes empty. No-throw.
   * @section complexity
   * Time $O(1)$, space $O(1)$.
   */
  graph(graph &&other) noexcept
      : vertices_(std::move(other.vertices_)), edges_(std::move(other.edges_)),
        vertex_lookup_(std::move(other.vertex_lookup_)),
        edge_lookup_(std::move(other.edge_lookup_)) {
    other.clear();
  }

  /**
   * @brief Move assignment; the previous contents are destroyed and @p other
   *        becomes empty. No-throw.
   * @section complexity
   * Time $O(n + m)$ (destroys the old contents), space $O(1)$.
   */
  graph &operator=(graph &&other) noexcept {
    if (this != &other) {
      vertices_ = std::move(other.vertices_);
      edges_ = std::move(other.edges_);
      vertex_lookup_ = std::move(other.vertex_lookup_);
      edge_lookup_ = std::move(other.edge_lookup_);
      other.clear();
    }
    return *this;
  }

  ~graph() = default;

  // ------------------------------------------------------------- size ----

  /** @brief Number of vertices. @section complexity Time $O(1)$, space $O(1)$.
   */
  [[nodiscard]] size_type num_vertices() const noexcept {
    return vertices_.size();
  }

  /** @brief Number of edges. @section complexity Time $O(1)$, space $O(1)$. */
  [[nodiscard]] size_type num_edges() const noexcept { return edges_.size(); }

  /** @brief True iff there are no vertices. @section complexity Time $O(1)$.
   */
  [[nodiscard]] bool empty() const noexcept { return vertices_.empty(); }

  /**
   * @brief Removes everything. No-throw. Invalidates all handles.
   * @section complexity
   * Time $O(n + m)$, space $O(1)$.
   */
  void clear() noexcept {
    edge_lookup_.clear();
    vertex_lookup_.clear();
    edges_.clear();
    vertices_.clear();
  }

  // ------------------------------------------------------- collections ----

  /**
   * @brief Handles of all vertices in insertion order (a snapshot).
   * @section complexity
   * Time $O(n)$, space $O(n)$.
   */
  [[nodiscard]] std::vector<vertex_handle> vertices() const;

  /**
   * @brief Handles of all edges in insertion order (a snapshot).
   * @section complexity
   * Time $O(m)$, space $O(m)$.
   */
  [[nodiscard]] std::vector<edge_handle> edges() const;

  /**
   * @brief Tests whether @p v refers to a live vertex of this graph.
   * @section complexity
   * Expected $O(1)$, worst case $O(n)$; space $O(1)$.
   */
  [[nodiscard]] bool contains(vertex_handle v) const;

  /**
   * @brief Tests whether @p e refers to a live edge of this graph.
   * @section complexity
   * Expected $O(1)$, worst case $O(m)$; space $O(1)$.
   */
  [[nodiscard]] bool contains(edge_handle e) const;

  /**
   * @brief Finds some vertex whose element equals @p value.
   * @return The first such vertex in insertion order, or `std::nullopt`.
   * @note Requires `operator==` on `V`.
   * @section complexity
   * Time $O(n)$, space $O(1)$.
   */
  [[nodiscard]] std::optional<vertex_handle> find_vertex(const V &value) const;

  // ---------------------------------------------------------- elements ----

  /**
   * @brief Element stored at vertex @p v.
   * @throws std::invalid_argument if @p v is invalid.
   * @section complexity
   * Expected $O(1)$, space $O(1)$.
   */
  [[nodiscard]] const V &element(vertex_handle v) const;

  /**
   * @brief Element stored at edge @p e.
   * @throws std::invalid_argument if @p e is invalid.
   * @section complexity
   * Expected $O(1)$, space $O(1)$.
   */
  [[nodiscard]] const E &element(edge_handle e) const;

  /**
   * @brief Replaces the element of vertex @p v.
   * @param v     Vertex handle.
   * @param value New element.
   * @return The previous element.
   * @throws std::invalid_argument if @p v is invalid.
   * @section complexity
   * Expected $O(1)$, space $O(1)$.
   */
  V replace(vertex_handle v, V value);

  /**
   * @brief Replaces the element of edge @p e.
   * @param e     Edge handle.
   * @param value New element.
   * @return The previous element.
   * @throws std::invalid_argument if @p e is invalid.
   * @section complexity
   * Expected $O(1)$, space $O(1)$.
   */
  E replace(edge_handle e, E value);

  // -------------------------------------------------------- edge access ----

  /**
   * @brief Endpoints `(first, second)` of @p e. For a directed edge this is
   *        `(origin, destination)`.
   * @throws std::invalid_argument if @p e is invalid.
   * @section complexity
   * Expected $O(1)$, space $O(1)$.
   */
  [[nodiscard]] std::pair<vertex_handle, vertex_handle>
  end_vertices(edge_handle e) const;

  /**
   * @brief Origin of the directed edge @p e.
   * @throws std::invalid_argument if @p e is invalid or undirected.
   * @section complexity
   * Expected $O(1)$, space $O(1)$.
   */
  [[nodiscard]] vertex_handle origin(edge_handle e) const;

  /**
   * @brief Destination of the directed edge @p e.
   * @throws std::invalid_argument if @p e is invalid or undirected.
   * @section complexity
   * Expected $O(1)$, space $O(1)$.
   */
  [[nodiscard]] vertex_handle destination(edge_handle e) const;

  /**
   * @brief The endpoint of @p e that is not @p v.
   * @throws std::invalid_argument if a handle is invalid or @p v is not an
   *         endpoint of @p e.
   * @section complexity
   * Expected $O(1)$, space $O(1)$.
   */
  [[nodiscard]] vertex_handle opposite(vertex_handle v, edge_handle e) const;

  /**
   * @brief True iff @p e is directed.
   * @throws std::invalid_argument if @p e is invalid.
   * @section complexity
   * Expected $O(1)$, space $O(1)$.
   */
  [[nodiscard]] bool is_directed(edge_handle e) const;

  // ------------------------------------------------------ vertex access ----

  /**
   * @brief Number of edges incident to @p v (directed or not).
   * @throws std::invalid_argument if @p v is invalid.
   * @section complexity
   * Expected $O(1)$, space $O(1)$.
   */
  [[nodiscard]] size_type degree(vertex_handle v) const;

  /**
   * @brief Number of edges that can be traversed into @p v (undirected edges
   *        and directed edges with destination @p v).
   * @throws std::invalid_argument if @p v is invalid.
   * @section complexity
   * Expected $O(1)$, space $O(1)$.
   */
  [[nodiscard]] size_type in_degree(vertex_handle v) const;

  /**
   * @brief Number of edges that can be traversed out of @p v (undirected edges
   *        and directed edges with origin @p v).
   * @throws std::invalid_argument if @p v is invalid.
   * @section complexity
   * Expected $O(1)$, space $O(1)$.
   */
  [[nodiscard]] size_type out_degree(vertex_handle v) const;

  /**
   * @brief All edges incident to @p v in the order they were inserted.
   * @throws std::invalid_argument if @p v is invalid.
   * @section complexity
   * Time $O(d(v))$ expected, space $O(d(v))$.
   */
  [[nodiscard]] std::vector<edge_handle> incident_edges(vertex_handle v) const;

  /**
   * @brief Incident edges traversable out of @p v.
   * @throws std::invalid_argument if @p v is invalid.
   * @section complexity
   * Time $O(d(v))$ expected, space $O(d(v))$.
   */
  [[nodiscard]] std::vector<edge_handle> out_edges(vertex_handle v) const;

  /**
   * @brief Incident edges traversable into @p v.
   * @throws std::invalid_argument if @p v is invalid.
   * @section complexity
   * Time $O(d(v))$ expected, space $O(d(v))$.
   */
  [[nodiscard]] std::vector<edge_handle> in_edges(vertex_handle v) const;

  /**
   * @brief The opposite endpoint of every incident edge of @p v, ignoring
   *        direction; one entry per edge (repeats for parallel edges).
   * @throws std::invalid_argument if @p v is invalid.
   * @section complexity
   * Time $O(d(v))$ expected, space $O(d(v))$.
   */
  [[nodiscard]] std::vector<vertex_handle>
  adjacent_vertices(vertex_handle v) const;

  /**
   * @brief Opposite endpoints of the edges in `out_edges(v)`.
   * @throws std::invalid_argument if @p v is invalid.
   * @section complexity
   * Time $O(d(v))$ expected, space $O(d(v))$.
   */
  [[nodiscard]] std::vector<vertex_handle>
  out_adjacent_vertices(vertex_handle v) const;

  /**
   * @brief Opposite endpoints of the edges in `in_edges(v)`.
   * @throws std::invalid_argument if @p v is invalid.
   * @section complexity
   * Time $O(d(v))$ expected, space $O(d(v))$.
   */
  [[nodiscard]] std::vector<vertex_handle>
  in_adjacent_vertices(vertex_handle v) const;

  /**
   * @brief Tests whether some edge joins @p v and @p w, ignoring direction.
   *        A vertex is never adjacent to itself.
   * @throws std::invalid_argument if a handle is invalid.
   * @section complexity
   * Time $O(\min(d(v), d(w)))$ expected, space $O(1)$.
   */
  [[nodiscard]] bool are_adjacent(vertex_handle v, vertex_handle w) const;

  /**
   * @brief Some edge that can be traversed from @p v to @p w (an undirected
   *        edge between them, or a directed edge `v -> w`).
   * @return The edge, or `std::nullopt`. With parallel edges any one of them
   *         may be returned.
   * @throws std::invalid_argument if a handle is invalid.
   * @section complexity
   * Time $O(\min(d(v), d(w)))$ expected, space $O(1)$.
   */
  [[nodiscard]] std::optional<edge_handle> get_edge(vertex_handle v,
                                                    vertex_handle w) const;

  // ------------------------------------------------------------ updates ----

  /**
   * @brief Inserts an isolated vertex.
   * @param value Its element.
   * @return Handle of the new vertex.
   * @note Strong exception guarantee.
   * @section complexity
   * Expected amortised $O(1)$, space $O(1)$.
   */
  vertex_handle insert_vertex(V value);

  /**
   * @brief Inserts an undirected edge between @p v and @p w.
   * @param v,w   Distinct endpoints.
   * @param value Edge element.
   * @return Handle of the new edge.
   * @throws std::invalid_argument if a handle is invalid or `v == w`.
   * @note Strong exception guarantee. Parallel edges are allowed.
   * @section complexity
   * Expected $O(1)$, space $O(1)$.
   */
  edge_handle insert_edge(vertex_handle v, vertex_handle w, E value = E{});

  /**
   * @brief Inserts a directed edge `v -> w`.
   * @param v     Origin.
   * @param w     Destination, different from @p v.
   * @param value Edge element.
   * @return Handle of the new edge.
   * @throws std::invalid_argument if a handle is invalid or `v == w`.
   * @note Strong exception guarantee.
   * @section complexity
   * Expected $O(1)$, space $O(1)$.
   */
  edge_handle insert_directed_edge(vertex_handle v, vertex_handle w,
                                   E value = E{});

  /**
   * @brief Removes edge @p e.
   * @return Its element.
   * @throws std::invalid_argument if @p e is invalid.
   * @section complexity
   * Expected $O(1)$, space $O(1)$.
   */
  E remove_edge(edge_handle e);

  /**
   * @brief Removes vertex @p v together with all its incident edges.
   * @return Its element.
   * @throws std::invalid_argument if @p v is invalid.
   * @section complexity
   * Expected $O(1 + d(v))$, space $O(1)$.
   */
  V remove_vertex(vertex_handle v);

  /**
   * @brief Swaps origin and destination of the directed edge @p e.
   * @throws std::invalid_argument if @p e is invalid or undirected.
   * @section complexity
   * Expected $O(1)$, space $O(1)$.
   */
  void reverse_direction(edge_handle e);

  /**
   * @brief Makes @p e directed with origin @p v (no-op if it already is).
   * @param e Edge handle (directed or undirected).
   * @param v One of the endpoints of @p e.
   * @throws std::invalid_argument if a handle is invalid or @p v is not an
   *         endpoint of @p e.
   * @section complexity
   * Expected $O(1)$, space $O(1)$.
   */
  void set_direction_from(edge_handle e, vertex_handle v);

  /**
   * @brief Makes @p e directed with destination @p v (no-op if it already is).
   * @param e Edge handle (directed or undirected).
   * @param v One of the endpoints of @p e.
   * @throws std::invalid_argument if a handle is invalid or @p v is not an
   *         endpoint of @p e.
   * @section complexity
   * Expected $O(1)$, space $O(1)$.
   */
  void set_direction_to(edge_handle e, vertex_handle v);

  /**
   * @brief Turns @p e into an undirected edge (no-op if it already is).
   * @throws std::invalid_argument if @p e is invalid.
   * @section complexity
   * Expected $O(1)$, space $O(1)$.
   */
  void make_undirected(edge_handle e);

private:
  struct edge_node;

  struct vertex_node {
    vertex_node(std::uint64_t node_id, V value)
        : id(node_id), element(std::move(value)) {}
    std::uint64_t id;
    V element;
    std::list<edge_node *> incident; // insertion order
    std::size_t out_degree = 0;
    std::size_t in_degree = 0;
  };

  struct edge_node {
    edge_node(std::uint64_t node_id, E value, vertex_node *from,
              vertex_node *to, bool is_directed)
        : id(node_id), element(std::move(value)), first(from), second(to),
          directed(is_directed) {}
    std::uint64_t id;
    E element;
    vertex_node *first;  // origin if directed
    vertex_node *second; // destination if directed
    bool directed;
    // Positions of this edge inside first->incident / second->incident.
    // Invariant: first_pos belongs to `first`, second_pos to `second`.
    typename std::list<edge_node *>::iterator first_pos{};
    typename std::list<edge_node *>::iterator second_pos{};
  };

  using vertex_container = std::list<vertex_node>;
  using edge_container = std::list<edge_node>;

  // Resolves handles; throws std::invalid_argument when stale/foreign/null.
  // (The lookup maps store iterators, so a const lookup can still yield a
  // mutable node reference; const member functions only read through it.)
  vertex_node &vnode(vertex_handle h) const {
    const auto it = vertex_lookup_.find(h.id());
    if (it == vertex_lookup_.end())
      throw std::invalid_argument(
          "graph: stale, foreign or null vertex handle");
    return *it->second;
  }

  edge_node &enode(edge_handle h) const {
    const auto it = edge_lookup_.find(h.id());
    if (it == edge_lookup_.end())
      throw std::invalid_argument("graph: stale, foreign or null edge handle");
    return *it->second;
  }

  static vertex_handle make_handle(const vertex_node &n) noexcept {
    return vertex_handle(n.id);
  }
  static edge_handle make_handle(const edge_node &n) noexcept {
    return edge_handle(n.id);
  }

  // Degree bookkeeping. An undirected edge is in/out for both endpoints; a
  // directed edge is out for `first` and in for `second`.
  static void count_edge(const edge_node &e) noexcept {
    ++e.first->out_degree;
    ++e.second->in_degree;
    if (!e.directed) {
      ++e.first->in_degree;
      ++e.second->out_degree;
    }
  }

  static void uncount_edge(const edge_node &e) noexcept {
    --e.first->out_degree;
    --e.second->in_degree;
    if (!e.directed) {
      --e.first->in_degree;
      --e.second->out_degree;
    }
  }

  // Unlinks and destroys an edge. @p n must not be used afterwards.
  void erase_edge(edge_node &n) noexcept {
    uncount_edge(n);
    n.first->incident.erase(n.first_pos);
    n.second->incident.erase(n.second_pos);
    const auto map_it = edge_lookup_.find(n.id);
    const auto list_it = map_it->second;
    edge_lookup_.erase(map_it);
    edges_.erase(list_it);
  }

  edge_handle link_edge(vertex_handle v, vertex_handle w, E value,
                        bool directed);

  // Swaps the stored endpoint order (and matching list positions).
  static void swap_endpoints(edge_node &n) noexcept {
    std::swap(n.first, n.second);
    std::swap(n.first_pos, n.second_pos);
  }

  // Declaration order matters for the move constructor's init list.
  vertex_container vertices_;
  edge_container edges_;
  std::unordered_map<std::uint64_t, typename vertex_container::iterator>
      vertex_lookup_;
  std::unordered_map<std::uint64_t, typename edge_container::iterator>
      edge_lookup_;
};

} // namespace graph_adt

#include "graph.tpp"
