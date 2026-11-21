/**
 * @file negative_weight_sssp.hpp
 * @brief Signed-weight SSSP utilities for C-13.15.
 *
 * Provides:
 *   - signed_edge       — directed, signed-weight edge.
 *   - signed_graph      — adjacency-list graph alias.
 *   - sssp_result       — common result type for both algorithms.
 *   - dijkstra()        — standard greedy SSSP (incorrect on negative edges).
 *   - bellman_ford()    — exact SSSP oracle with negative-cycle detection.
 *   - make_c1315_graph()   — the definitive 4-vertex counterexample.
 *   - make_c1315_family()  — parameterised k-vertex counterexample family.
 *
 */

#ifndef NEGATIVE_WEIGHT_SSSP_HPP
#define NEGATIVE_WEIGHT_SSSP_HPP

#include <cstddef>
#include <optional>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

namespace dsa {

// ============================================================
//  Graph representation
// ============================================================

/**
 * @brief A single directed, signed-weight edge.
 * @section complexity Storage: $O(1)$.
 */
struct signed_edge {
  std::size_t to{}; ///< Destination vertex index.
  int weight{};     ///< Edge weight; may be negative.
};

/// Weighted directed adjacency list: graph[u] holds all outgoing edges of u.
using signed_graph = std::vector<std::vector<signed_edge>>;

// ============================================================
//  Result type
// ============================================================

/**
 * @brief Result returned by dijkstra() and bellman_ford().
 *
 * @section complexity Storage: $O(n)$ where $n = |V|$.
 */
struct sssp_result {
  /// Per-vertex shortest distance from the source.
  /// std::nullopt means the vertex is unreachable from the source.
  std::vector<std::optional<int>> dist;

  /// True iff a negative-weight cycle reachable from the source was detected.
  /// Only meaningful when returned by bellman_ford(); dijkstra() always
  /// leaves this false.
  bool negative_cycle{false};
};

// ============================================================
//  Dijkstra's algorithm  (standard greedy — BREAKS on negative edges)
// ============================================================

/**
 * @brief Runs Dijkstra's algorithm on a weighted directed graph.
 *
 * Uses a lazy-deletion binary min-heap (std::priority_queue).  A vertex is
 * *settled* the first time it is extracted; it is never re-examined.  This
 * greedy invariant is correct only when all edge weights are non-negative.
 * When negative edges exist, the settled distance may be larger than the true
 * shortest distance
 *
 * @param g      Weighted directed adjacency list ($n$ vertices).
 * @param source Source vertex index; must satisfy source < g.size().
 * @return sssp_result with per-vertex distance estimates.
 *         sssp_result::negative_cycle is always false.
 * @throws std::out_of_range if source >= g.size().
 *
 * @section complexity
 * Time: $O((n + m) \log n)$ worst-case, where $n = |V|$, $m = |E|$.
 * Auxiliary space: $O(n + m)$ for the heap in the worst case.
 *
 * @note Exception safety: Strong guarantee — g is never modified.
 */
[[nodiscard]] inline sssp_result dijkstra(const signed_graph &g,
                                          std::size_t source) {
  const std::size_t n = g.size();
  if (source >= n) {
    throw std::out_of_range("dijkstra: source vertex out of range.");
  }

  sssp_result result;
  result.dist.assign(n, std::nullopt);
  std::vector<bool> settled(n, false);

  // Min-heap of (tentative_distance, vertex).
  using pq_entry = std::pair<int, std::size_t>;
  std::priority_queue<pq_entry, std::vector<pq_entry>, std::greater<pq_entry>>
      pq;

  result.dist[source] = 0;
  pq.push({0, source});

  while (!pq.empty()) {
    const auto [d_u, u] = pq.top();
    pq.pop();

    // Lazy deletion: skip stale (higher-distance) copies of u.
    if (settled[u]) {
      continue;
    }
    // KEY INVARIANT (breaks for negative edges):
    // Once extracted, d_u is assumed to be the final shortest distance for u.
    // If a negative edge later reduces d[u] via an unsettled predecessor,
    // u is never re-examined and the wrong value persists.
    settled[u] = true;

    for (const signed_edge &e : g[u]) {
      if (settled[e.to]) {
        continue; // Never revisit a settled vertex.
      }
      const int candidate = d_u + e.weight;
      if (!result.dist[e.to].has_value() || candidate < *result.dist[e.to]) {
        result.dist[e.to] = candidate;
        pq.push({candidate, e.to});
      }
    }
  }

  return result;
}

// ============================================================
//  Bellman-Ford algorithm  (exact oracle, handles negative edges)
// ============================================================

/**
 * @brief Runs the Bellman-Ford algorithm on a weighted directed graph.
 *
 * Performs up to $n - 1$ relaxation passes over all edges, then one
 * additional pass to detect negative-weight cycles reachable from source.
 * Correctly handles negative-weight edges; serves as the trusted oracle in
 * the C-13.15 counterexample tests.
 *
 * @param g      Weighted directed adjacency list ($n$ vertices).
 * @param source Source vertex index; must satisfy source < g.size().
 * @return sssp_result.  If sssp_result::negative_cycle is true a
 *         negative-weight cycle reachable from source was found, and the
 *         dist values for affected vertices are unreliable.
 * @throws std::out_of_range if source >= g.size().
 *
 * @section complexity
 * Time: $O(n \cdot m)$ worst-case, where $n = |V|$, $m = |E|$.
 * Auxiliary space: $O(n)$.
 *
 * @note Exception safety: Strong guarantee — g is never modified.
 */
[[nodiscard]] inline sssp_result bellman_ford(const signed_graph &g,
                                              std::size_t source) {
  const std::size_t n = g.size();
  if (source >= n) {
    throw std::out_of_range("bellman_ford: source vertex out of range.");
  }

  sssp_result result;
  result.dist.assign(n, std::nullopt);
  result.dist[source] = 0;

  // n-1 relaxation rounds: any shortest path in a graph without negative
  // cycles has at most n-1 edges, so n-1 passes always converge.
  for (std::size_t round = 0; round + 1 < n; ++round) {
    bool updated = false;
    for (std::size_t u = 0; u < n; ++u) {
      if (!result.dist[u].has_value()) {
        continue; // u not yet reached.
      }
      for (const signed_edge &e : g[u]) {
        const int candidate = *result.dist[u] + e.weight;
        if (!result.dist[e.to].has_value() || candidate < *result.dist[e.to]) {
          result.dist[e.to] = candidate;
          updated = true;
        }
      }
    }
    if (!updated) {
      break; // Converged early.
    }
  }

  // Negative-cycle detection: a further relaxation after n-1 rounds signals
  // a negative-weight cycle reachable from source.
  for (std::size_t u = 0; u < n; ++u) {
    if (!result.dist[u].has_value()) {
      continue;
    }
    for (const signed_edge &e : g[u]) {
      if (!result.dist[e.to].has_value() ||
          *result.dist[u] + e.weight < *result.dist[e.to]) {
        result.negative_cycle = true;
        return result;
      }
    }
  }

  return result;
}

// ============================================================
//  Graph factory helpers
// ============================================================

/**
 * @brief Builds the definitive C-13.15 counterexample graph.
 *
 * Four vertices: s = 0, A = 1, B = 2, C = 3.
 *
 * @verbatim
 *   s=0 --1--> A=1
 *   A=1 --4--> B=2
 *   s=0 --2--> C=3    <-- cheap direct edge; Dijkstra settles C=2 too early
 *   B=2 --(-6)--> C=3 <-- negative edge; true d[C] = 1+4-6 = -1
 * @endverbatim
 *
 * Dijkstra extraction order:
 *   1. Extract s (d=0).  Relax: d[A]=1, d[C]=2.
 *   2. Extract A (d=1).  Relax: d[B]=5.
 *   3. Extract C (d=2).  C is settled — d[C] = 2 finalised.
 *   4. Extract B (d=5).  Tries to relax C: 5 + (-6) = -1 < 2,
 *      but C is already settled and NEVER revisited.
 *      => Dijkstra reports d[C] = 2.  True d[C] = -1.  WRONG.
 *
 * No negative cycle: B -> C has no return path to B.
 *
 * @return signed_graph with 4 vertices and 4 edges.
 * @section complexity Time and space: $O(1)$.
 */
[[nodiscard]] inline signed_graph make_c1315_graph() {
  signed_graph g(4);
  g[0].push_back({1, 1});  // s -> A, weight  1
  g[1].push_back({2, 4});  // A -> B, weight  4
  g[0].push_back({3, 2});  // s -> C, weight  2
  g[2].push_back({3, -6}); // B -> C, weight -6
  return g;
}

/**
 * @brief Builds a parameterised family G_k of counterexample graphs.
 *
 * G_k has k + 2 vertices: s = 0, chain[1..k], C = k + 1.
 *
 * @verbatim
 *   s --1--> 1 --1--> 2 --> ... --> k    (chain of k hops, weight 1 each)
 *   s --(k-1)--> C                        (direct shortcut, d_shortcut = k-1)
 *   k --(-(k+1))--> C                     (negative hop)
 * @endverbatim
 *
 * Invariants (for all k >= 2):
 *   - True d[C] via chain = k + (-(k+1)) = -1.
 *   - Dijkstra d[C] = k-1 (shortcut settled before chain completes).
 *   - No negative cycle.
 *
 * @param k Chain length; must be >= 2.
 * @return signed_graph with k + 2 vertices.
 * @section complexity Time: $O(k)$; auxiliary space: $O(k)$.
 */
[[nodiscard]] inline signed_graph make_c1315_family(std::size_t k) {
  const std::size_t c = k + 1; // index of vertex C
  signed_graph g(c + 1);

  // Chain s -> 1 -> 2 -> ... -> k (each edge weight 1).
  g[0].push_back({1, 1});
  for (std::size_t i = 1; i < k; ++i) {
    g[i].push_back({i + 1, 1});
  }

  // Direct shortcut s -> C: weight k-1 < k (the chain length),
  // so Dijkstra settles C before it finishes the chain.
  g[0].push_back({c, static_cast<int>(k) - 1});

  // Negative final hop: last chain vertex -> C.
  // weight = -(k+1), so true d[C] = k - (k+1) = -1.
  g[k].push_back({c, -static_cast<int>(k + 1)});

  return g;
}

} // namespace dsa

#endif // NEGATIVE_WEIGHT_SSSP_HPP
