/**
 * @file biconnectivity.hpp
 * @brief Algorithms for graph biconnectivity and biconnected augmentation (C-13.20).

 * This header provides:
 *   - undirected_graph: An adjacency-list representation for undirected graphs.
 *   - edge: Normalized representation of an undirected edge.
 *   - biconnectivity_result: Detailed diagnostic containing cut vertices,
 *     bridges, and biconnected components (blocks).
 *   - analyze_biconnectivity: Hopcroft-Tarjan DFS algorithm running in $O(n + m)$
 *     time to find all articulation vertices and blocks.
 *   - is_biconnected: Fast predicate for 2-vertex connectivity.
 *   - biconnect_graph: The $O(n + m)$ augmentation algorithm that determines a
 *     set of at most $n$ new edges whose addition guarantees $G$ is biconnected.
 */

#ifndef BICONNECTIVITY_HPP
#define BICONNECTIVITY_HPP

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

namespace dsa {

/**
 * @brief Normalized undirected edge between two vertices.
 *
 * Invariant: u <= v at all times.
 *
 * @section complexity
 * Storage: $O(1)$ auxiliary space.
 */
struct edge {
  std::size_t u{}; ///< First endpoint index (u <= v).
  std::size_t v{}; ///< Second endpoint index.

  constexpr edge() noexcept = default;

  /**
   * @brief Constructs a normalized edge where u <= v.
   * @param a First endpoint.
   * @param b Second endpoint.
   */
  constexpr edge(std::size_t a, std::size_t b) noexcept
      : u(a < b ? a : b), v(a < b ? b : a) {}

  constexpr bool operator==(const edge &other) const noexcept {
    return u == other.u && v == other.v;
  }

  constexpr bool operator!=(const edge &other) const noexcept {
    return !(*this == other);
  }

  constexpr bool operator<(const edge &other) const noexcept {
    return (u < other.u) || (u == other.u && v < other.v);
  }
};

/**
 * @class undirected_graph
 * @brief Adjacency-list graph model for undirected networks.
 *
 * Vertices are 0-indexed: $\{0, 1, \dots, n - 1\}$.
 */
class undirected_graph {
public:
  /**
   * @brief Constructs an undirected graph with n vertices and no edges.
   * @param num_vertices Number of vertices ($n$).
   *
   * @section complexity
   * Time: $O(n)$ worst-case. Auxiliary space: $O(n)$.
   */
  explicit undirected_graph(std::size_t num_vertices)
      : adj_(num_vertices), num_vertices_(num_vertices) {}

  /**
   * @brief Adds an undirected edge between vertices u and v.
   * @param u First endpoint index ($0 \le u < n$).
   * @param v Second endpoint index ($0 \le v < n$).
   * @throws std::out_of_range if u or v is out of bounds.
   * @throws std::invalid_argument if u == v (self-loops not permitted).
   *
   * @section complexity
   * Time: $O(1)$ amortized worst-case. Auxiliary space: $O(1)$ amortized.
   */
  void add_edge(std::size_t u, std::size_t v) {
    validate_vertex(u, "add_edge: u out of range");
    validate_vertex(v, "add_edge: v out of range");
    if (u == v) {
      throw std::invalid_argument("add_edge: self-loops not permitted");
    }
    adj_[u].push_back(v);
    adj_[v].push_back(u);
    edges_.push_back(edge(u, v));
  }

  /**
   * @brief Returns the total number of vertices $n$.
   * @return Number of vertices.
   *
   * @section complexity
   * Time: $O(1)$ worst-case. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] std::size_t num_vertices() const noexcept {
    return num_vertices_;
  }

  /**
   * @brief Returns the total number of undirected edges $m$.
   * @return Number of edges.
   *
   * @section complexity
   * Time: $O(1)$ worst-case. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] std::size_t num_edges() const noexcept { return edges_.size(); }

  /**
   * @brief Accesses the adjacency list of vertex u.
   * @param u Vertex index ($0 \le u < n$).
   * @return Const reference to neighbor indices.
   * @throws std::out_of_range if u is invalid.
   *
   * @section complexity
   * Time: $O(1)$ worst-case. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] const std::vector<std::size_t> &adjacent(std::size_t u) const {
    validate_vertex(u, "adjacent: vertex out of range");
    return adj_[u];
  }

  /**
   * @brief Checks whether an edge exists between u and v.
   * @param u First endpoint.
   * @param v Second endpoint.
   * @return True if edge (u, v) exists, false otherwise.
   * @throws std::out_of_range if u or v is out of range.
   *
   * @section complexity
   * Time: $O(\min(\mathrm{deg}(u), \mathrm{deg}(v)))$ worst-case. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] bool has_edge(std::size_t u, std::size_t v) const {
    validate_vertex(u, "has_edge: u out of range");
    validate_vertex(v, "has_edge: v out of range");
    const auto &neighbors =
        (adj_[u].size() <= adj_[v].size()) ? adj_[u] : adj_[v];
    const std::size_t target = (adj_[u].size() <= adj_[v].size()) ? v : u;
    return std::find(neighbors.begin(), neighbors.end(), target) !=
           neighbors.end();
  }

  /**
   * @brief Returns the collection of all undirected edges in the graph.
   * @return Const reference to vector of edges.
   *
   * @section complexity
   * Time: $O(1)$ worst-case. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] const std::vector<edge> &all_edges() const noexcept {
    return edges_;
  }

  /**
   * @brief Checks if the graph is connected using Breadth-First Search.
   * @return True if connected or empty, false if disconnected.
   *
   * @section complexity
   * Time: $O(n + m)$ worst-case, where $n = |V|$ and $m = |E|$.
   * Auxiliary space: $O(n)$ for visited flags and BFS queue.
   */
  [[nodiscard]] bool is_connected() const {
    if (num_vertices_ <= 1) {
      return true;
    }
    std::vector<bool> visited(num_vertices_, false);
    std::queue<std::size_t> q;
    visited[0] = true;
    q.push(0);
    std::size_t count = 0;

    while (!q.empty()) {
      const std::size_t u = q.front();
      q.pop();
      ++count;

      for (std::size_t v : adj_[u]) {
        if (!visited[v]) {
          visited[v] = true;
          q.push(v);
        }
      }
    }
    return count == num_vertices_;
  }

private:
  void validate_vertex(std::size_t u, const char *msg) const {
    if (u >= num_vertices_) {
      throw std::out_of_range(msg);
    }
  }

  std::vector<std::vector<std::size_t>> adj_{};
  std::vector<edge> edges_{};
  std::size_t num_vertices_{0};
};

// ============================================================================
// Biconnectivity Diagnostic Types & Hopcroft-Tarjan Algorithm
// ============================================================================

/**
 * @brief Diagnostic result of a biconnectivity analysis.
 *
 * @section complexity
 * Storage: $O(n + m)$ where $n = |V|$ and $m = |E|$.
 */
struct biconnectivity_result {
  /// True if the graph is connected and has no articulation vertices (for n >= 3).
  bool is_biconnected{false};

  /// List of articulation vertices (cut vertices) whose removal disconnects G.
  std::vector<std::size_t> cut_vertices{};

  /// List of bridge edges whose removal increases component count.
  std::vector<edge> bridges{};

  /// Biconnected components (blocks), each represented as a vector of edges.
  std::vector<std::vector<edge>> blocks{};
};

/**
 * @brief Analyzes the biconnectivity of an undirected graph using Hopcroft-Tarjan DFS.
 *
 * Computes discovery times (`tin`) and low-link values (`low`) in a single DFS pass.
 * Detects all articulation points, bridges, and edge-partitioned biconnected blocks.
 *
 * @param g The undirected graph.
 * @return biconnectivity_result containing cut vertices, bridges, and blocks.
 *
 * @section complexity
 * Time: $O(n + m)$ worst-case, where $n = |V|$ and $m = |E|$.
 * Auxiliary space: $O(n + m)$ for recursion stack, discovery arrays, and edge stack.
 *
 * @note Exception safety: Strong guarantee; graph g is unmodified.
 */
[[nodiscard]] inline biconnectivity_result
analyze_biconnectivity(const undirected_graph &g) {
  const std::size_t n = g.num_vertices();
  biconnectivity_result res;

  if (n < 3) {
    res.is_biconnected = (n == 2 && g.num_edges() >= 1) || (n <= 1);
    return res;
  }

  if (!g.is_connected()) {
    res.is_biconnected = false;
    return res;
  }

  std::vector<std::size_t> tin(n, 0);
  std::vector<std::size_t> low(n, 0);
  std::vector<bool> is_cut(n, false);
  std::vector<edge> edge_stack;
  std::size_t timer = 0;

  struct frame {
    std::size_t u;
    std::size_t p;
    std::size_t edge_idx;
    std::size_t children;
  };

  std::vector<frame> call_stack;
  call_stack.reserve(n);

  tin[0] = low[0] = ++timer;
  call_stack.push_back({0, n, 0, 0});

  while (!call_stack.empty()) {
    frame &curr = call_stack.back();
    const std::size_t u = curr.u;
    const std::size_t p = curr.p;
    const auto &adj = g.adjacent(u);

    if (curr.edge_idx < adj.size()) {
      const std::size_t v = adj[curr.edge_idx++];
      if (v == p) {
        continue;
      }
      if (tin[v] != 0) {
        // Back-edge
        low[u] = std::min(low[u], tin[v]);
        if (tin[v] < tin[u]) {
          edge_stack.push_back(edge(u, v));
        }
      } else {
        // Tree-edge
        ++curr.children;
        edge_stack.push_back(edge(u, v));
        tin[v] = low[v] = ++timer;
        call_stack.push_back({v, u, 0, 0});
      }
    } else {
      // Completed all neighbors of u
      if (p == n && curr.children > 1) {
        is_cut[u] = true;
      }

      call_stack.pop_back();

      if (!call_stack.empty()) {
        frame &parent_frame = call_stack.back();
        const std::size_t parent_u = parent_frame.u;
        low[parent_u] = std::min(low[parent_u], low[u]);

        if (parent_frame.p != n && low[u] >= tin[parent_u]) {
          is_cut[parent_u] = true;
        }

        if (low[u] > tin[parent_u]) {
          res.bridges.push_back(edge(parent_u, u));
        }

        if (low[u] >= tin[parent_u]) {
          std::vector<edge> block;
          const edge target(parent_u, u);
          while (!edge_stack.empty()) {
            const edge e = edge_stack.back();
            edge_stack.pop_back();
            block.push_back(e);
            if (e == target) {
              break;
            }
          }
          if (!block.empty()) {
            res.blocks.push_back(std::move(block));
          }
        }
      }
    }
  }

  // If any edges remain on the stack
  if (!edge_stack.empty()) {
    std::vector<edge> block;
    while (!edge_stack.empty()) {
      block.push_back(edge_stack.back());
      edge_stack.pop_back();
    }
    res.blocks.push_back(std::move(block));
  }

  for (std::size_t i = 0; i < n; ++i) {
    if (is_cut[i]) {
      res.cut_vertices.push_back(i);
    }
  }

  res.is_biconnected = res.cut_vertices.empty();
  return res;
}

/**
 * @brief Checks whether an undirected graph is biconnected (has no articulation points).
 * @param g The undirected graph.
 * @return True if biconnected and n >= 3, false otherwise.
 *
 * @section complexity
 * Time: $O(n + m)$ worst-case. Auxiliary space: $O(n + m)$.
 */
[[nodiscard]] inline bool is_biconnected(const undirected_graph &g) {
  if (g.num_vertices() < 3) {
    return false;
  }
  return analyze_biconnectivity(g).is_biconnected;
}

// ============================================================================
// Biconnected Augmentation Algorithm (C-13.20)
// ============================================================================

/**
 * @brief Guarantees biconnectivity by adding at most n edges in O(n + m) time (C-13.20).
 *
 * The algorithm:
 * 1. Verifies that G is connected and n >= 3.
 * 2. Runs Hopcroft-Tarjan analysis: if G is already biconnected, returns 0 added edges.
 * 3. Traverses G using Depth-First Search to record the vertex discovery sequence.
 * 4. Checks each cycle edge (v_i, v_{(i+1) mod n}) against G's existing edges in
 *    O(deg(v_i)) time, summing to O(m) total time.
 * 5. Returns only the newly required edges, guaranteeing |E_{added}| <= n.
 *
 * @param g Connected undirected graph with n >= 3 and m >= n - 1.
 * @return std::vector<edge> List of at most n new edges to add to g.
 * @throws std::invalid_argument if n < 3 or g is not connected.
 *
 * @section complexity
 * Time: $O(n + m)$ worst-case, where $n = |V|$ and $m = |E|$.
 * Auxiliary space: $O(n)$ for DFS discovery ordering, visited flags, and return vector.
 *
 * @note Exception safety: Strong guarantee; graph g is unmodified.
 */
[[nodiscard]] inline std::vector<edge>
biconnect_graph(const undirected_graph &g) {
  const std::size_t n = g.num_vertices();
  if (n < 3) {
    throw std::invalid_argument(
        "biconnect_graph: graph must have at least 3 vertices");
  }
  if (!g.is_connected()) {
    throw std::invalid_argument("biconnect_graph: graph must be connected");
  }

  // If already biconnected, 0 edges need to be added
  if (is_biconnected(g)) {
    return {};
  }

  // Step 1: Collect DFS discovery order iteratively
  std::vector<std::size_t> order;
  order.reserve(n);
  std::vector<bool> visited(n, false);
  std::vector<std::pair<std::size_t, std::size_t>> stack;
  stack.reserve(n);

  visited[0] = true;
  order.push_back(0);
  stack.push_back({0, 0});

  while (!stack.empty()) {
    auto &[u, idx] = stack.back();
    const auto &adj = g.adjacent(u);
    if (idx < adj.size()) {
      const std::size_t v = adj[idx++];
      if (!visited[v]) {
        visited[v] = true;
        order.push_back(v);
        stack.push_back({v, 0});
      }
    } else {
      stack.pop_back();
    }
  }

  // Step 2: Form Hamiltonian cycle along the DFS traversal order
  // and add only edges that are not already present in G.
  std::vector<edge> added_edges;
  added_edges.reserve(n);

  for (std::size_t i = 0; i < n; ++i) {
    const std::size_t u = order[i];
    const std::size_t v = order[(i + 1) % n];

    // u != v is guaranteed because all elements in order are distinct and n >= 3
    if (!g.has_edge(u, v)) {
      added_edges.push_back(edge(u, v));
    }
  }

  assert(added_edges.size() <= n && "Added edges must not exceed n");
  return added_edges;
}

} // namespace dsa

#endif // BICONNECTIVITY_HPP
