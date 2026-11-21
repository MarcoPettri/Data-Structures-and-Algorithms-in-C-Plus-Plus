/**
 * @file floyd_warshall.hpp
 * @brief Floyd-Warshall All-Pairs Shortest Path (APSP) variation (C-13.24).
 *
 * Features provided:
 *   - weighted_digraph<Weight>: Directed weighted graph representation.
 *   - apsp_result<Weight>: Matrix of shortest distances, path reconstruction table,
 *     and negative-weight cycle diagnostics.
 *   - floyd_warshall: $O(n^3)$ algorithm computing shortest path lengths for all pairs,
 *     detecting negative cycles and marking paths corrupted by $-\infty$.
 *   - bellman_ford_all_pairs: $O(n^2 m)$ independent oracle for verification.
 */

#ifndef FLOYD_WARSHALL_HPP
#define FLOYD_WARSHALL_HPP

#include <cassert>
#include <cstddef>
#include <limits>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace dsa {

// ============================================================================
// Weighted Digraph
// ============================================================================

/**
 * @brief Directed edge with arithmetic weight.
 * @tparam Weight Arithmetic numeric type (e.g. double, int64_t).
 */
template <typename Weight = double> struct directed_edge {
  static_assert(std::is_arithmetic_v<Weight>,
                "Weight must be a numeric arithmetic type.");
  std::size_t to{}; ///< Target vertex.
  Weight weight{};  ///< Directed edge weight (can be negative).
};

/**
 * @class weighted_digraph
 * @brief Adjacency-list directed graph model with edge weights.
 *
 * Vertices are 0-indexed: $\{0, 1, \dots, n - 1\}$.
 *
 * @tparam Weight Arithmetic numeric type.
 */
template <typename Weight = double> class weighted_digraph {
public:
  static_assert(std::is_arithmetic_v<Weight>,
                "Weight must be a numeric arithmetic type.");

  /**
   * @brief Constructs an empty digraph with n vertices.
   * @param num_vertices Total vertex count $n$.
   *
   * @section complexity
   * Time: $O(n)$. Auxiliary space: $O(n)$.
   */
  explicit weighted_digraph(std::size_t num_vertices)
      : adj_(num_vertices), num_vertices_(num_vertices) {}

  /**
   * @brief Adds a directed edge from u to v with specified weight.
   * @param u Source vertex index ($0 \le u < n$).
   * @param v Destination vertex index ($0 \le v < n$).
   * @param weight Edge weight (may be negative, zero, or positive).
   * @throws std::out_of_range if u or v is out of bounds.
   *
   * @section complexity
   * Time: $O(1)$ amortized. Auxiliary space: $O(1)$ amortized.
   */
  void add_edge(std::size_t u, std::size_t v, Weight weight) {
    validate_vertex(u, "add_edge: source u out of range");
    validate_vertex(v, "add_edge: destination v out of range");
    adj_[u].push_back(directed_edge<Weight>{v, weight});
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
   * @brief Returns the total number of directed edges $m$.
   * @return Number of edges.
   *
   * @section complexity
   * Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] std::size_t num_edges() const noexcept { return num_edges_; }

  /**
   * @brief Accesses outgoing edges from vertex u.
   * @param u Source vertex index.
   * @return Const reference to outgoing edges.
   * @throws std::out_of_range if u is invalid.
   *
   * @section complexity
   * Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] const std::vector<directed_edge<Weight>> &
  outgoing(std::size_t u) const {
    validate_vertex(u, "outgoing: vertex out of range");
    return adj_[u];
  }

private:
  void validate_vertex(std::size_t u, const char *msg) const {
    if (u >= num_vertices_) {
      throw std::out_of_range(msg);
    }
  }

  std::vector<std::vector<directed_edge<Weight>>> adj_{};
  std::size_t num_vertices_{0};
  std::size_t num_edges_{0};
};

// ============================================================================
// APSP Result Structure
// ============================================================================

/**
 * @brief Result matrix and diagnostics of All-Pairs Shortest Paths.
 * @tparam Weight Arithmetic numeric type.
 *
 * @section complexity
 * Storage: $O(n^2)$ auxiliary space.
 */
template <typename Weight = double> struct apsp_result {
  /// dist[i][j]: Length of the shortest path from vertex i to j.
  /// std::nullopt represents that j is unreachable from i.
  std::vector<std::vector<std::optional<Weight>>> dist{};

  /// next_vertex[i][j]: The immediate next vertex on the shortest path from i to j.
  /// std::nullopt if no path exists.
  std::vector<std::vector<std::optional<std::size_t>>> next_vertex{};

  /// True if any negative-weight cycle was detected anywhere in G.
  bool has_negative_cycle{false};

  /// neg_cycle_path[i][j]: True if there is a path from i to j that can be made
  /// arbitrarily short (distance $-\infty$) by traversing a negative-weight cycle.
  std::vector<std::vector<bool>> neg_cycle_path{};

  /**
   * @brief Reconstructs the sequence of vertices along the shortest path from u to v.
   * @param u Source vertex.
   * @param v Destination vertex.
   * @return Vector of vertices starting at u and ending at v.
   *         Empty if unreachable or if corrupted by a negative cycle.
   *
   * @section complexity
   * Time: $O(k)$ where $k \le n$ is path length. Auxiliary space: $O(k)$.
   */
  [[nodiscard]] std::vector<std::size_t> get_path(std::size_t u,
                                                  std::size_t v) const {
    if (u >= dist.size() || v >= dist.size() || !dist[u][v].has_value()) {
      return {};
    }
    if (neg_cycle_path[u][v]) {
      return {}; // Path is undefined / minus infinity
    }

    std::vector<std::size_t> path;
    std::size_t curr = u;
    path.push_back(curr);

    while (curr != v) {
      if (!next_vertex[curr][v].has_value()) {
        return {};
      }
      curr = *next_vertex[curr][v];
      path.push_back(curr);
      if (path.size() > dist.size() + 1) {
        // Cycle encountered in path reconstruction
        return {};
      }
    }
    return path;
  }
};

// ============================================================================
// Floyd-Warshall APSP Algorithm (C-13.24)
// ============================================================================

/**
 * @brief Computes shortest paths between all pairs of vertices in O(n^3) time (C-13.24).
 *
 * Solves the All-Pairs Shortest Paths problem using the dynamic programming recurrence:
 *   dist^{(k)}[i][j] = min(dist^{(k-1)}[i][j], dist^{(k-1)}[i][k] + dist^{(k-1)}[k][j])
 * where k ranges from 0 to n - 1.
 *
 * Diagnostics:
 *   - Correctly handles positive and negative edge weights.
 *   - Identifies whether the graph contains any negative-weight cycle (if dist[i][i] < 0).
 *   - Marks pairs (u, v) whose distance can be made arbitrarily small (-infinity).
 *   - Maintains a successor matrix (next_vertex) allowing linear-time path reconstruction.
 *
 * @tparam Weight Arithmetic numeric type.
 * @param g The weighted directed graph.
 * @return apsp_result<Weight> containing the distance matrix, successor table, and cycle status.
 *
 * @section complexity
 * Time: $O(n^3)$ worst-case, where $n = |V|$ is the number of vertices.
 * Auxiliary space: $O(n^2)$ for distance, successor, and cycle matrices.
 *
 * @note Exception safety: Strong guarantee; graph g is unmodified.
 */
template <typename Weight = double>
[[nodiscard]] apsp_result<Weight>
floyd_warshall(const weighted_digraph<Weight> &g) {
  const std::size_t n = g.num_vertices();
  apsp_result<Weight> res;

  if (n == 0) {
    return res;
  }

  // Sentinel representation for infinity
  constexpr bool has_inf = std::numeric_limits<Weight>::has_infinity;
  const Weight inf = has_inf ? std::numeric_limits<Weight>::infinity()
                             : (std::numeric_limits<Weight>::max() / 2);

  // Initialize distance matrix with infinity and successor table
  std::vector<std::vector<Weight>> d(n, std::vector<Weight>(n, inf));
  std::vector<std::vector<std::optional<std::size_t>>> nxt(
      n, std::vector<std::optional<std::size_t>>(n, std::nullopt));

  // Distance from any vertex to itself is initially 0
  for (std::size_t i = 0; i < n; ++i) {
    d[i][i] = Weight{0};
    nxt[i][i] = i;
  }

  // Load edges from digraph; if parallel edges exist, keep the minimum weight
  for (std::size_t u = 0; u < n; ++u) {
    for (const auto &edge : g.outgoing(u)) {
      const std::size_t v = edge.to;
      if (edge.weight < d[u][v]) {
        d[u][v] = edge.weight;
        nxt[u][v] = v;
      }
    }
  }

  // Floyd-Warshall main dynamic programming loop: O(n^3)
  for (std::size_t k = 0; k < n; ++k) {
    for (std::size_t i = 0; i < n; ++i) {
      if (d[i][k] >= inf) {
        continue;
      }
      for (std::size_t j = 0; j < n; ++j) {
        if (d[k][j] >= inf) {
          continue;
        }
        const Weight through_k = d[i][k] + d[k][j];
        if (through_k < d[i][j]) {
          d[i][j] = through_k;
          nxt[i][j] = nxt[i][k];
        }
      }
    }
  }

  // Check for negative-weight cycles (dist[i][i] < 0)
  res.has_negative_cycle = false;
  std::vector<std::vector<bool>> neg_cycle(n, std::vector<bool>(n, false));

  for (std::size_t k = 0; k < n; ++k) {
    if (d[k][k] < Weight{0}) {
      res.has_negative_cycle = true;
      // Any pair (i, j) where i reaches k and k reaches j has distance -infinity
      for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
          if (d[i][k] < inf && d[k][j] < inf) {
            neg_cycle[i][j] = true;
          }
        }
      }
    }
  }

  // Convert raw distances to std::optional
  res.dist.assign(n, std::vector<std::optional<Weight>>(n, std::nullopt));
  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t j = 0; j < n; ++j) {
      if (d[i][j] < inf && !neg_cycle[i][j]) {
        res.dist[i][j] = d[i][j];
      }
    }
  }

  res.next_vertex = std::move(nxt);
  res.neg_cycle_path = std::move(neg_cycle);

  return res;
}

// ============================================================================
// Bellman-Ford All-Pairs Oracle (for Independent Verification)
// ============================================================================

/**
 * @brief Independent oracle running Bellman-Ford from all n sources.
 *
 * @tparam Weight Arithmetic numeric type.
 * @param g The weighted directed graph.
 * @return apsp_result<Weight> containing shortest paths or negative cycle indication.
 *
 * @section complexity
 * Time: $O(n^2 m)$ worst-case ($n$ executions of $O(n \cdot m)$ Bellman-Ford).
 * Auxiliary space: $O(n^2)$.
 */
template <typename Weight = double>
[[nodiscard]] apsp_result<Weight>
bellman_ford_all_pairs(const weighted_digraph<Weight> &g) {
  const std::size_t n = g.num_vertices();
  apsp_result<Weight> res;
  if (n == 0) {
    return res;
  }

  constexpr bool has_inf = std::numeric_limits<Weight>::has_infinity;
  const Weight inf = has_inf ? std::numeric_limits<Weight>::infinity()
                             : (std::numeric_limits<Weight>::max() / 2);

  res.dist.assign(n, std::vector<std::optional<Weight>>(n, std::nullopt));
  res.neg_cycle_path.assign(n, std::vector<bool>(n, false));
  res.next_vertex.assign(
      n, std::vector<std::optional<std::size_t>>(n, std::nullopt));

  // Run Bellman-Ford from each source s in 0 .. n - 1
  for (std::size_t s = 0; s < n; ++s) {
    std::vector<Weight> d(n, inf);
    std::vector<std::size_t> parent(n, n);
    d[s] = Weight{0};

    // Relax all edges n - 1 times
    for (std::size_t step = 0; step + 1 < n; ++step) {
      for (std::size_t u = 0; u < n; ++u) {
        if (d[u] >= inf) {
          continue;
        }
        for (const auto &edge : g.outgoing(u)) {
          const std::size_t v = edge.to;
          if (d[u] + edge.weight < d[v]) {
            d[v] = d[u] + edge.weight;
            parent[v] = u;
          }
        }
      }
    }

    // Check for negative cycles reachable from s
    std::vector<bool> in_neg_cycle(n, false);
    for (std::size_t u = 0; u < n; ++u) {
      if (d[u] >= inf) {
        continue;
      }
      for (const auto &edge : g.outgoing(u)) {
        const std::size_t v = edge.to;
        if (d[u] + edge.weight < d[v]) {
          res.has_negative_cycle = true;
          in_neg_cycle[v] = true;
        }
      }
    }

    // Propagate negative cycle reachability
    if (res.has_negative_cycle) {
      for (std::size_t step = 0; step < n; ++step) {
        for (std::size_t u = 0; u < n; ++u) {
          if (!in_neg_cycle[u]) {
            continue;
          }
          for (const auto &edge : g.outgoing(u)) {
            in_neg_cycle[edge.to] = true;
          }
        }
      }
    }

    for (std::size_t v = 0; v < n; ++v) {
      if (in_neg_cycle[v]) {
        res.neg_cycle_path[s][v] = true;
      } else if (d[v] < inf) {
        res.dist[s][v] = d[v];
      }
    }
  }

  return res;
}

} // namespace dsa

#endif // FLOYD_WARSHALL_HPP
