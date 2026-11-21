/**
 * @file bipartite.hpp
 * @brief Algorithms for testing graph bipartiteness (2-colorability) (C-13.26).
 *   - undirected_graph: Adjacency-list graph model.
 *   - bipartite_result: Partition sets X and Y, vertex colors, and odd-cycle certificate.
 *   - check_bipartite: $O(n + m)$ BFS-based 2-coloring algorithm.
 *   - is_bipartite: Convenience predicate returning boolean status.
 */

#ifndef BIPARTITE_HPP
#define BIPARTITE_HPP

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

namespace dsa {

/**
 * @class undirected_graph
 * @brief Adjacency-list representation for undirected graphs.
 *
 * Vertices are 0-indexed: $\{0, 1, \dots, n - 1\}$.
 */
class undirected_graph {
public:
  /**
   * @brief Constructs an undirected graph with n vertices and no initial edges.
   * @param num_vertices Total vertex count $n$.
   *
   * @section complexity
   * Time: $O(n)$. Auxiliary space: $O(n)$.
   */
  explicit undirected_graph(std::size_t num_vertices)
      : adj_(num_vertices), num_vertices_(num_vertices) {}

  /**
   * @brief Adds an undirected edge between vertices u and v.
   * @param u First endpoint index ($0 \le u < n$).
   * @param v Second endpoint index ($0 \le v < n$).
   * @throws std::out_of_range if u or v is out of bounds.
   *
   * @section complexity
   * Time: $O(1)$ amortized. Auxiliary space: $O(1)$ amortized.
   */
  void add_edge(std::size_t u, std::size_t v) {
    validate_vertex(u, "add_edge: vertex u out of range");
    validate_vertex(v, "add_edge: vertex v out of range");
    adj_[u].push_back(v);
    if (u != v) {
      adj_[v].push_back(u);
    }
    ++num_edges_;
  }

  /**
   * @brief Returns the total number of vertices $n$.
   * @return Number of vertices.
   *
   * @section complexity
   * Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] std::size_t num_vertices() const noexcept {
    return num_vertices_;
  }

  /**
   * @brief Returns the total number of edges $m$.
   * @return Number of edges.
   *
   * @section complexity
   * Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] std::size_t num_edges() const noexcept { return num_edges_; }

  /**
   * @brief Accesses adjacent vertices of vertex u.
   * @param u Vertex index.
   * @return Const reference to neighbor list.
   * @throws std::out_of_range if u is invalid.
   *
   * @section complexity
   * Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] const std::vector<std::size_t> &adjacent(std::size_t u) const {
    validate_vertex(u, "adjacent: vertex out of range");
    return adj_[u];
  }

  /**
   * @brief Checks whether an edge exists between u and v.
   * @param u First vertex.
   * @param v Second vertex.
   * @return True if (u, v) exists, false otherwise.
   *
   * @section complexity
   * Time: $O(\min(\mathrm{deg}(u), \mathrm{deg}(v)))$. Auxiliary space: $O(1)$.
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

private:
  void validate_vertex(std::size_t u, const char *msg) const {
    if (u >= num_vertices_) {
      throw std::out_of_range(msg);
    }
  }

  std::vector<std::vector<std::size_t>> adj_{};
  std::size_t num_vertices_{0};
  std::size_t num_edges_{0};
};

// ============================================================================
// Bipartite Result Diagnostic Structure
// ============================================================================

/**
 * @brief Diagnostic result of bipartiteness testing.
 *
 * @section complexity
 * Storage: $O(n)$ auxiliary space where $n = |V|$.
 */
struct bipartite_result {
  /// True if graph is bipartite (2-colorable), false otherwise.
  bool is_bipartite{false};

  /// Vertices assigned to set X (color 0).
  std::vector<std::size_t> set_x{};

  /// Vertices assigned to set Y (color 1).
  std::vector<std::size_t> set_y{};

  /// colors[u]: 0 if in X, 1 if in Y, -1 if isolated or uncolored.
  std::vector<int> colors{};

  /// If non-bipartite, contains a concrete odd-length cycle certifying non-bipartiteness:
  /// (v_0, v_1, ..., v_{2k}, v_0) with 2k + 1 distinct vertices.
  std::vector<std::size_t> odd_cycle{};
};

// ============================================================================
// Bipartiteness Testing Algorithm (C-13.26)
// ============================================================================

/**
 * @brief Determines if an undirected graph G is bipartite in O(n + m) time (C-13.26).
 *
 * Traverses all connected components of G using Breadth-First Search.
 * Greedily assigns alternating colors $\{0, 1\}$ (corresponding to partitions $X$ and $Y$).
 * If an edge is discovered linking two vertices of the identical color, the graph contains
 * an odd cycle and is declared non-bipartite. An explicit odd cycle certificate is reconstructed
 * by tracing paths back to their lowest common ancestor in the BFS tree.
 *
 * @param g The undirected graph.
 * @return bipartite_result containing bipartiteness flag, partitions X and Y, and odd cycle if present.
 *
 * @section complexity
 * Time: $O(n + m)$ worst-case, where $n = |V|$ is the number of vertices and $m = |E|$ is the number of edges.
 * Auxiliary space: $O(n)$ for color array, parent pointers, BFS levels, and queue.
 *
 * @note Exception safety: Strong guarantee; graph g is unmodified.
 */
[[nodiscard]] inline bipartite_result
check_bipartite(const undirected_graph &g) {
  const std::size_t n = g.num_vertices();
  bipartite_result res;
  res.colors.assign(n, -1);

  if (n == 0) {
    res.is_bipartite = true;
    return res;
  }

  std::vector<std::size_t> parent(n, n);
  std::vector<std::size_t> level(n, 0);
  std::queue<std::size_t> q;

  // Process each connected component
  for (std::size_t start = 0; start < n; ++start) {
    if (res.colors[start] != -1) {
      continue;
    }

    res.colors[start] = 0;
    parent[start] = n;
    level[start] = 0;
    q.push(start);

    while (!q.empty()) {
      const std::size_t u = q.front();
      q.pop();

      for (std::size_t v : g.adjacent(u)) {
        if (v == u) {
          // Self-loop is an odd cycle of length 1: u -> u
          res.is_bipartite = false;
          res.odd_cycle = {u, u};
          return res;
        }

        if (res.colors[v] == -1) {
          // Unvisited neighbor: assign opposite color
          res.colors[v] = 1 - res.colors[u];
          parent[v] = u;
          level[v] = level[u] + 1;
          q.push(v);
        } else if (res.colors[v] == res.colors[u]) {
          // Conflict! Same color neighbor detected -> odd cycle found!
          res.is_bipartite = false;

          // Reconstruct odd cycle via BFS parent pointers
          std::vector<std::size_t> path_u;
          std::vector<std::size_t> path_v;

          std::size_t curr_u = u;
          std::size_t curr_v = v;

          // Equalize levels if different
          while (level[curr_u] > level[curr_v]) {
            path_u.push_back(curr_u);
            curr_u = parent[curr_u];
          }
          while (level[curr_v] > level[curr_u]) {
            path_v.push_back(curr_v);
            curr_v = parent[curr_v];
          }

          // Move up in tandem until lowest common ancestor (LCA)
          while (curr_u != curr_v) {
            path_u.push_back(curr_u);
            path_v.push_back(curr_v);
            curr_u = parent[curr_u];
            curr_v = parent[curr_v];
          }

          // curr_u is the LCA
          std::vector<std::size_t> cycle;
          // Path from LCA down to u
          cycle.push_back(curr_u);
          for (auto it = path_u.rbegin(); it != path_u.rend(); ++it) {
            cycle.push_back(*it);
          }
          // Edge (u, v) connects to path from v up to LCA
          for (std::size_t node : path_v) {
            cycle.push_back(node);
          }
          // Close the cycle
          cycle.push_back(curr_u);

          res.odd_cycle = std::move(cycle);
          return res;
        }
      }
    }
  }

  // All components successfully 2-colored
  res.is_bipartite = true;
  for (std::size_t i = 0; i < n; ++i) {
    if (res.colors[i] == 0) {
      res.set_x.push_back(i);
    } else {
      res.set_y.push_back(i);
    }
  }

  return res;
}

/**
 * @brief Convenience predicate for graph bipartiteness.
 * @param g The undirected graph.
 * @return True if G is bipartite, false otherwise.
 *
 * @section complexity
 * Time: $O(n + m)$. Auxiliary space: $O(n)$.
 */
[[nodiscard]] inline bool is_bipartite(const undirected_graph &g) {
  return check_bipartite(g).is_bipartite;
}

} // namespace dsa

#endif // BIPARTITE_HPP
