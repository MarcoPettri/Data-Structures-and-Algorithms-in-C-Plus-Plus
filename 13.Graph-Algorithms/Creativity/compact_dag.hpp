/**
 * @file compact_dag.hpp
 * @brief Implementation of Compact Directed Acyclic Graph (DAG)
 * detection (C-13.1).
 *
 * An n-vertex directed acyclic graph G is defined as "compact" if there exists
 * a bijective numbering pi: V -> {0, 1, ..., n - 1} such that G contains the
 * directed edge (u, v) if and only if pi(u) < pi(v) for all distinct u, v in V.
 *
 * In graph-theoretic terms, a compact DAG is isomorphic to a transitive
 * tournament, which possesses exactly n(n - 1) / 2 directed edges with unique
 * out-degrees {0, 1, ..., n - 1}.
 */

#ifndef COMPACT_DAG_HPP
#define COMPACT_DAG_HPP

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace dsa {

/**
 * @struct CompactDAGResult
 * @brief Encapsulates the verification outcome and the recovered topological
 * numbering.
 */
struct CompactDAGResult {
  /// True if the graph satisfies all criteria for a compact DAG; false
  /// otherwise.
  bool is_compact{false};

  /// Mapping from vertex ID to topological integer rank in [0, n - 1].
  /// vertex_to_rank[u] = i denotes that vertex u is assigned integer i.
  std::vector<std::size_t> vertex_to_rank{};

  /// Mapping from topological integer rank in [0, n - 1] to original vertex ID.
  /// rank_to_vertex[i] = u denotes that integer i corresponds to vertex u.
  std::vector<std::size_t> rank_to_vertex{};

  /// Diagnostic description or failure reason.
  std::string reason{};

  /**
   * @brief Contextual boolean conversion operator.
   * @return True if graph is compact, false otherwise.
   */
  [[nodiscard]] constexpr explicit operator bool() const noexcept {
    return is_compact;
  }
};

/**
 * @class DirectedGraph
 * @brief Production-grade directed graph ADT supporting adjacency lists and
 * matrix views.
 */
class DirectedGraph {
public:
  /**
   * @brief Constructs an empty directed graph with a specified number of
   * vertices.
   * @param num_vertices Number of vertices (0 to n - 1).
   *
   * @section complexity
   * - Time: $O(n)$ where $n$ is the number of vertices.
   * - Space: $O(n)$ auxiliary space.
   */
  explicit DirectedGraph(std::size_t num_vertices = 0)
      : num_vertices_(num_vertices), num_edges_(0), adj_(num_vertices) {}

  /**
   * @brief Adds a directed edge from vertex u to vertex v.
   * @param u Source vertex index in [0, num_vertices - 1].
   * @param v Destination vertex index in [0, num_vertices - 1].
   * @throws std::out_of_range if u or v is out of bounds.
   *
   * @section complexity
   * - Time: $O(1)$ amortized.
   * - Space: $O(1)$ auxiliary space.
   */
  void add_edge(std::size_t u, std::size_t v) {
    validate_vertex(u);
    validate_vertex(v);
    adj_[u].push_back(v);
    ++num_edges_;
  }

  /**
   * @brief Returns the total number of vertices in the graph.
   * @return Vertex count $n$.
   *
   * @section complexity
   * - Time: $O(1)$.
   * - Space: $O(1)$.
   */
  [[nodiscard]] constexpr std::size_t num_vertices() const noexcept {
    return num_vertices_;
  }

  /**
   * @brief Returns the total number of directed edges in the graph.
   * @return Edge count $m$.
   *
   * @section complexity
   * - Time: $O(1)$.
   * - Space: $O(1)$.
   */
  [[nodiscard]] constexpr std::size_t num_edges() const noexcept {
    return num_edges_;
  }

  /**
   * @brief Returns the out-degree of a specified vertex.
   * @param u Vertex index.
   * @return Number of outgoing edges from vertex u.
   *
   * @section complexity
   * - Time: $O(1)$.
   * - Space: $O(1)$.
   */
  [[nodiscard]] std::size_t out_degree(std::size_t u) const {
    validate_vertex(u);
    return adj_[u].size();
  }

  /**
   * @brief Returns an immutable reference to the list of outgoing neighbors of
   * vertex u.
   * @param u Vertex index.
   * @return Const reference to std::vector<std::size_t> containing targets of
   * outgoing edges.
   *
   * @section complexity
   * - Time: $O(1)$.
   * - Space: $O(1)$.
   */
  [[nodiscard]] const std::vector<std::size_t> &
  out_neighbors(std::size_t u) const {
    validate_vertex(u);
    return adj_[u];
  }

  /**
   * @brief Returns an immutable reference to the underlying full adjacency
   * list.
   * @return Const reference to the vector of outgoing adjacency lists.
   *
   * @section complexity
   * - Time: $O(1)$.
   * - Space: $O(1)$.
   */
  [[nodiscard]] const std::vector<std::vector<std::size_t>> &
  adjacency_list() const noexcept {
    return adj_;
  }

  /**
   * @brief Checks if a directed edge (u, v) exists.
   * @param u Source vertex index.
   * @param v Destination vertex index.
   * @return True if (u, v) is in E, false otherwise.
   *
   * @section complexity
   * - Time: $O(\text{out\_deg}(u))$ worst-case.
   * - Space: $O(1)$.
   */
  [[nodiscard]] bool has_edge(std::size_t u, std::size_t v) const {
    validate_vertex(u);
    validate_vertex(v);
    const auto &neighbors = adj_[u];
    return std::find(neighbors.begin(), neighbors.end(), v) != neighbors.end();
  }

  /**
   * @brief Factory method constructing a DirectedGraph from a list of directed
   * edges.
   * @param n Number of vertices.
   * @param edges Vector of pairs (u, v) representing directed edges.
   * @return Constructed DirectedGraph instance.
   *
   * @section complexity
   * - Time: $O(n + |E|)$.
   * - Space: $O(n + |E|)$.
   */
  [[nodiscard]] static DirectedGraph from_edge_list(
      std::size_t n,
      const std::vector<std::pair<std::size_t, std::size_t>> &edges) {
    DirectedGraph g(n);
    for (const auto &[u, v] : edges) {
      g.add_edge(u, v);
    }
    return g;
  }

  /**
   * @brief Factory method constructing a DirectedGraph from a boolean adjacency
   * matrix.
   * @param matrix 2D square matrix where matrix[u][v] == true denotes edge (u,
   * v).
   * @return Constructed DirectedGraph instance.
   * @throws std::invalid_argument if the matrix is not square.
   *
   * @section complexity
   * - Time: $O(n^2)$.
   * - Space: $O(n + |E|)$.
   */
  [[nodiscard]] static DirectedGraph
  from_adjacency_matrix(const std::vector<std::vector<bool>> &matrix) {
    const std::size_t n = matrix.size();
    DirectedGraph g(n);
    for (std::size_t u = 0; u < n; ++u) {
      if (matrix[u].size() != n) {
        throw std::invalid_argument("Adjacency matrix must be square (n x n).");
      }
      for (std::size_t v = 0; v < n; ++v) {
        if (matrix[u][v]) {
          g.add_edge(u, v);
        }
      }
    }
    return g;
  }

  /**
   * @brief Factory generating a canonical compact DAG on n vertices where edge
   * (u, v) exists iff u < v.
   * @param n Number of vertices.
   * @return A canonical compact DAG with vertex numbering identical to
   * identity.
   *
   * @section complexity
   * - Time: $O(n^2)$.
   * - Space: $O(n^2)$.
   */
  [[nodiscard]] static DirectedGraph make_canonical_compact_dag(std::size_t n) {
    DirectedGraph g(n);
    for (std::size_t u = 0; u < n; ++u) {
      for (std::size_t v = u + 1; v < n; ++v) {
        g.add_edge(u, v);
      }
    }
    return g;
  }

  /**
   * @brief Factory generating a compact DAG under an arbitrary vertex
   * permutation.
   * @param permutation Permutation of integers [0, n - 1] where permutation[u]
   * is the rank of u.
   * @return A compact DAG where edge (u, v) exists iff permutation[u] <
   * permutation[v].
   * @throws std::invalid_argument if permutation is not a valid bijection of
   * [0, n - 1].
   *
   * @section complexity
   * - Time: $O(n^2)$.
   * - Space: $O(n^2)$.
   */
  [[nodiscard]] static DirectedGraph make_compact_dag_from_permutation(
      const std::vector<std::size_t> &permutation) {
    const std::size_t n = permutation.size();
    // Validate permutation
    std::vector<bool> seen(n, false);
    for (std::size_t val : permutation) {
      if (val >= n || seen[val]) {
        throw std::invalid_argument(
            "Input is not a valid permutation of [0, n - 1].");
      }
      seen[val] = true;
    }

    DirectedGraph g(n);
    for (std::size_t u = 0; u < n; ++u) {
      for (std::size_t v = 0; v < n; ++v) {
        if (permutation[u] < permutation[v]) {
          g.add_edge(u, v);
        }
      }
    }
    return g;
  }

private:
  std::size_t num_vertices_{0};
  std::size_t num_edges_{0};
  std::vector<std::vector<std::size_t>> adj_{};

  void validate_vertex(std::size_t v) const {
    if (v >= num_vertices_) {
      std::ostringstream oss;
      oss << "Vertex index " << v << " out of range [0, " << num_vertices_
          << ").";
      throw std::out_of_range(oss.str());
    }
  }
};

/**
 * @class CompactDAGDetector
 * @brief Algorithmic engine for detecting whether a directed graph is a compact
 * DAG in O(n^2) time.
 *
 * Mathematical Foundations:
 * Let G = (V, E) be a directed graph with |V| = n.
 * G is compact iff there exists a bijection pi: V -> {0, 1, ..., n - 1} such
 * that (u, v) in E <=> pi(u) < pi(v) for all u, v in V.
 *
 * Necessary & Sufficient Properties:
 * 1. Edge Count: Exactly |E| = n(n - 1) / 2 edges.
 * 2. Degree Sequence: Each vertex u with rank pi(u) = i must have:
 *      out_deg(u) = (n - 1) - i
 *      in_deg(u)  = i
 *    Thus, the out-degrees {out_deg(u) : u in V} must form a permutation of {0,
 * 1, ..., n - 1}. This uniquely determines the candidate ranking: pi(u) = n - 1
 * - out_deg(u).
 * 3. Topological Edge Consistency:
 *    For every vertex u with candidate rank i = pi(u), all of its out_deg(u) =
 * n - 1 - i outgoing edges must lead to distinct target vertices v having pi(v)
 * > i. Since there are exactly (n - 1) - i integer values greater than i in [0,
 * n - 1], having n - 1 - i distinct forward edges implies u has directed edges
 * to EVERY vertex with rank > i, and NO edges to vertices with rank <= i. This
 * completely verifies compactness in O(n^2) worst-case time and O(n) auxiliary
 * space.
 */
class CompactDAGDetector {
public:
  /**
   * @brief Detects if the provided DirectedGraph is a compact DAG.
   * @param graph Directed graph instance.
   * @return CompactDAGResult containing boolean flag, bijective rankings, and
   * diagnostics.
   *
   * @section complexity
   * - Time: $O(n^2)$ worst-case time, where $n = |V|$.
   * - Space: $O(n)$ auxiliary memory (beyond the graph representation).
   */
  [[nodiscard]] static CompactDAGResult detect(const DirectedGraph &graph) {
    const std::size_t n = graph.num_vertices();

    // Base cases
    if (n == 0) {
      return {true, {}, {}, "Empty graph is vacuously compact."};
    }
    if (n == 1) {
      if (graph.num_edges() != 0) {
        return {false,
                {},
                {},
                "Single vertex graph with self-loops cannot be compact."};
      }
      return {true, {0}, {0}, "Single vertex graph with 0 edges is compact."};
    }

    // Step 1: Edge count check: |E| must be exactly n(n - 1) / 2
    const std::size_t expected_edges = (n * (n - 1)) / 2;
    if (graph.num_edges() != expected_edges) {
      std::ostringstream oss;
      oss << "Edge count mismatch: graph has " << graph.num_edges()
          << " edges, but compact DAG on " << n << " vertices requires exactly "
          << expected_edges << " edges.";
      return {false, {}, {}, oss.str()};
    }

    // Step 2: Out-degree permutation check & candidate rank assignment
    // In a compact DAG, out-degrees must be a permutation of {0, 1, ..., n -
    // 1}. Candidate rank: pi(u) = n - 1 - out_deg(u).
    std::vector<bool> seen_out_deg(n, false);
    std::vector<std::size_t> vertex_to_rank(n, 0);
    std::vector<std::size_t> rank_to_vertex(n, 0);

    for (std::size_t u = 0; u < n; ++u) {
      const std::size_t out_deg = graph.out_degree(u);
      if (out_deg >= n) {
        std::ostringstream oss;
        oss << "Vertex " << u << " has out-degree " << out_deg << " >= n (" << n
            << ").";
        return {false, {}, {}, oss.str()};
      }
      if (seen_out_deg[out_deg]) {
        std::ostringstream oss;
        oss << "Duplicate out-degree " << out_deg
            << " detected. Out-degrees must form a permutation of {0, ..., n - "
               "1}.";
        return {false, {}, {}, oss.str()};
      }
      seen_out_deg[out_deg] = true;

      const std::size_t rank = (n - 1) - out_deg;
      vertex_to_rank[u] = rank;
      rank_to_vertex[rank] = u;
    }

    // Step 3: Edge verification under candidate ranking
    // Check that every outgoing edge (u, v) satisfies pi(u) < pi(v) and that
    // there are no duplicate edges or self-loops.
    // We use a visited timestamp/epoch array to detect duplicates in O(1)
    // without reallocations.
    std::vector<std::size_t> visited_rank_epoch(n, 0);

    for (std::size_t u = 0; u < n; ++u) {
      const std::size_t u_rank = vertex_to_rank[u];
      const auto &neighbors = graph.out_neighbors(u);

      for (std::size_t v : neighbors) {
        if (v >= n) {
          return {false, {}, {}, "Target vertex index out of range."};
        }
        const std::size_t v_rank = vertex_to_rank[v];

        // Self-loop or backward/cross edge violating topological order
        if (v_rank <= u_rank) {
          std::ostringstream oss;
          oss << "Directed edge (" << u << ", " << v
              << ") violates topological order: "
              << "rank(" << u << ")=" << u_rank << " >= rank(" << v
              << ")=" << v_rank << ".";
          return {false, {}, {}, oss.str()};
        }

        // Duplicate edge check
        const std::size_t current_epoch = u + 1;
        if (visited_rank_epoch[v_rank] == current_epoch) {
          std::ostringstream oss;
          oss << "Duplicate edge detected from vertex " << u << " to " << v
              << ".";
          return {false, {}, {}, oss.str()};
        }
        visited_rank_epoch[v_rank] = current_epoch;
      }
    }

    // All conditions verified successfully
    return {true, std::move(vertex_to_rank), std::move(rank_to_vertex),
            "Graph is a compact DAG (transitive tournament)."};
  }

  /**
   * @brief Overload detecting compactness from raw adjacency lists.
   * @param adj Adjacency list representation where adj[u] contains
   * out-neighbors of u.
   * @return CompactDAGResult.
   *
   * @section complexity
   * - Time: $O(n^2)$ worst-case time.
   * - Space: $O(n)$ auxiliary space.
   */
  [[nodiscard]] static CompactDAGResult
  detect(const std::vector<std::vector<std::size_t>> &adj) {
    const std::size_t n = adj.size();
    if (n == 0) {
      return {true, {}, {}, "Empty graph is vacuously compact."};
    }
    if (n == 1) {
      if (!adj[0].empty()) {
        return {false,
                {},
                {},
                "Single vertex graph with self-loops cannot be compact."};
      }
      return {true, {0}, {0}, "Single vertex graph with 0 edges is compact."};
    }

    // Compute edge count in O(n)
    std::size_t edge_count = 0;
    const std::size_t expected_edges = (n * (n - 1)) / 2;
    for (std::size_t u = 0; u < n; ++u) {
      edge_count += adj[u].size();
      if (edge_count > expected_edges) {
        return {false,
                {},
                {},
                "Edge count exceeds maximum required for compact DAG."};
      }
    }
    if (edge_count != expected_edges) {
      return {false, {}, {}, "Edge count != n(n - 1) / 2."};
    }

    // Check out-degree permutation
    std::vector<bool> seen_out_deg(n, false);
    std::vector<std::size_t> vertex_to_rank(n, 0);
    std::vector<std::size_t> rank_to_vertex(n, 0);

    for (std::size_t u = 0; u < n; ++u) {
      const std::size_t deg = adj[u].size();
      if (deg >= n || seen_out_deg[deg]) {
        return {false,
                {},
                {},
                "Out-degrees do not form a permutation of [0, n - 1]."};
      }
      seen_out_deg[deg] = true;
      const std::size_t rank = (n - 1) - deg;
      vertex_to_rank[u] = rank;
      rank_to_vertex[rank] = u;
    }

    // Verify all edges are forward and distinct
    std::vector<std::size_t> visited_epoch(n, 0);
    for (std::size_t u = 0; u < n; ++u) {
      const std::size_t u_rank = vertex_to_rank[u];
      const std::size_t current_epoch = u + 1;
      for (std::size_t v : adj[u]) {
        if (v >= n) {
          return {false, {}, {}, "Target vertex out of range."};
        }
        const std::size_t v_rank = vertex_to_rank[v];
        if (v_rank <= u_rank || visited_epoch[v_rank] == current_epoch) {
          return {false,
                  {},
                  {},
                  "Edge violates topological ranking or is duplicated."};
        }
        visited_epoch[v_rank] = current_epoch;
      }
    }

    return {true, std::move(vertex_to_rank), std::move(rank_to_vertex),
            "Adjacency list represents a compact DAG."};
  }

  /**
   * @brief Overload detecting compactness from a boolean adjacency matrix.
   * @param matrix 2D square matrix where matrix[u][v] is true iff (u, v) in E.
   * @return CompactDAGResult.
   *
   * @section complexity
   * - Time: $O(n^2)$ worst-case time.
   * - Space: $O(n)$ auxiliary space.
   */
  [[nodiscard]] static CompactDAGResult
  detect(const std::vector<std::vector<bool>> &matrix) {
    const std::size_t n = matrix.size();
    if (n == 0) {
      return {true, {}, {}, "Empty graph is vacuously compact."};
    }
    for (std::size_t i = 0; i < n; ++i) {
      if (matrix[i].size() != n) {
        throw std::invalid_argument("Adjacency matrix must be square (n x n).");
      }
      if (matrix[i][i]) {
        return {false, {}, {}, "Self-loop detected on diagonal."};
      }
    }
    if (n == 1) {
      return {true, {0}, {0}, "Single vertex graph is compact."};
    }

    // Compute out-degrees and check tournament anti-symmetry:
    // For every pair (u, v) with u < v, exactly one of matrix[u][v] or
    // matrix[v][u] must be true.
    std::vector<std::size_t> out_deg(n, 0);
    for (std::size_t u = 0; u < n; ++u) {
      for (std::size_t v = u + 1; v < n; ++v) {
        const bool uv = matrix[u][v];
        const bool vu = matrix[v][u];
        if (uv == vu) {
          // Either both true (2-cycle) or both false (missing edge)
          return {
              false,
              {},
              {},
              "Graph is not a tournament (missing edge or 2-cycle detected)."};
        }
        if (uv) {
          ++out_deg[u];
        } else {
          ++out_deg[v];
        }
      }
    }

    // Verify out-degrees form a permutation of [0, n - 1]
    std::vector<bool> seen_deg(n, false);
    std::vector<std::size_t> vertex_to_rank(n, 0);
    std::vector<std::size_t> rank_to_vertex(n, 0);

    for (std::size_t u = 0; u < n; ++u) {
      const std::size_t deg = out_deg[u];
      if (deg >= n || seen_deg[deg]) {
        return {false,
                {},
                {},
                "Tournament has cycles (out-degrees do not form permutation)."};
      }
      seen_deg[deg] = true;
      const std::size_t rank = (n - 1) - deg;
      vertex_to_rank[u] = rank;
      rank_to_vertex[rank] = u;
    }

    // Verify consistency: matrix[u][v] == true <=> rank[u] < rank[v]
    for (std::size_t u = 0; u < n; ++u) {
      for (std::size_t v = 0; v < n; ++v) {
        if (u != v) {
          const bool expected = (vertex_to_rank[u] < vertex_to_rank[v]);
          if (matrix[u][v] != expected) {
            return {false,
                    {},
                    {},
                    "Edge orientation contradicts candidate ranking."};
          }
        }
      }
    }

    return {true, std::move(vertex_to_rank), std::move(rank_to_vertex),
            "Adjacency matrix represents a compact DAG."};
  }

  /**
   * @brief Overload detecting compactness from an edge list.
   * @param n Number of vertices.
   * @param edges Vector of directed edge pairs (u, v).
   * @return CompactDAGResult.
   *
   * @section complexity
   * - Time: $O(n^2)$ worst-case time.
   * - Space: $O(n + |E|)$ auxiliary space.
   */
  [[nodiscard]] static CompactDAGResult
  detect(std::size_t n,
         const std::vector<std::pair<std::size_t, std::size_t>> &edges) {
    const std::size_t expected_edges = (n * (n - 1)) / 2;
    if (edges.size() != expected_edges) {
      return {false, {}, {}, "Edge list size != n(n - 1) / 2."};
    }
    DirectedGraph g = DirectedGraph::from_edge_list(n, edges);
    return detect(g);
  }

  /**
   * @brief Verification oracle that directly validates a given numbering
   * against the graph.
   * @param graph The graph under test.
   * @param numbering Candidate mapping from vertex u to integer in [0, n - 1].
   * @return True if (u, v) in E <=> numbering[u] < numbering[v], false
   * otherwise.
   *
   * @section complexity
   * - Time: $O(n^3)$ worst-case, since it checks $O(n^2)$ vertex pairs and
   *   each `has_edge` call may scan $O(n)$ outgoing neighbors.
   * - Space: $O(n)$ auxiliary space.
   */
  [[nodiscard]] static bool
  verify_numbering(const DirectedGraph &graph,
                   const std::vector<std::size_t> &numbering) {
    const std::size_t n = graph.num_vertices();
    if (numbering.size() != n) {
      return false;
    }
    // Check bijection
    std::vector<bool> seen(n, false);
    for (std::size_t rank : numbering) {
      if (rank >= n || seen[rank]) {
        return false;
      }
      seen[rank] = true;
    }

    // Verify all pairs
    for (std::size_t u = 0; u < n; ++u) {
      for (std::size_t v = 0; v < n; ++v) {
        if (u == v) {
          if (graph.has_edge(u, v)) {
            return false;
          }
        } else {
          const bool edge_present = graph.has_edge(u, v);
          const bool should_be_present = (numbering[u] < numbering[v]);
          if (edge_present != should_be_present) {
            return false;
          }
        }
      }
    }
    return true;
  }
};

/**
 * @brief Convenience helper function to test if a DirectedGraph is compact.
 * @param graph Input directed graph.
 * @return CompactDAGResult.
 */
[[nodiscard]] inline CompactDAGResult
is_compact_dag(const DirectedGraph &graph) {
  return CompactDAGDetector::detect(graph);
}

} // namespace dsa

#endif // COMPACT_DAG_HPP
