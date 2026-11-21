/**
 * @file integer_weight_mst.hpp
 * @brief Minimum Spanning Tree for graphs with integer weights in [1, n] in O(m log* n) time (C-13.28).
 *
 * Problem Statement (C-13.28):
 * Let G be a graph with n vertices and m edges such that all the edge weights
 * in G are integers in the range [1, n]. Give an algorithm for finding a mini-
 * mum spanning tree for G in O(m log* n) time.
 *
 * Features provided:
 *   - integer_edge: Undirected edge with integral weight.
 *   - integer_graph: Adjacency-list graph model supporting bounded integer weights.
 *   - disjoint_set: Union-Find with path compression and union-by-rank.
 *   - counting_sort_edges: O(m + W) linear-time edge sort for bounded integer keys.
 *   - integer_weight_mst: The O(m log* n) Kruskal algorithm with counting sort.
 *   - comparison_kruskal_mst: Standard O(m log m) Kruskal for verification.
 *   - log_star: Iterated logarithm calculator log*(n).
 */

#ifndef INTEGER_WEIGHT_MST_HPP
#define INTEGER_WEIGHT_MST_HPP

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

namespace dsa {

// ============================================================================
// Math Helper: Iterated Logarithm log*(n)
// ============================================================================

/**
 * @brief Computes the iterated logarithm log*(n) (the number of times log2 must be
 *        applied before the result is <= 1).
 * @param n Positive integer value.
 * @return log*(n).
 *
 * @section complexity
 * Time: $O(\log^* n)$ operations. Auxiliary space: $O(1)$.
 */
[[nodiscard]] constexpr std::size_t log_star(std::uint64_t n) noexcept {
  std::size_t count = 0;
  double val = static_cast<double>(n);
  while (val > 1.0) {
    val = std::log2(val);
    ++count;
  }
  return count;
}

// ============================================================================
// Edge and Graph Data Structures
// ============================================================================

/**
 * @brief An undirected weighted edge with integral weight.
 *
 * Invariant: u <= v at all times.
 *
 * @section complexity
 * Storage: $O(1)$ auxiliary space.
 */
struct integer_edge {
  std::size_t u{};         ///< First endpoint (u <= v).
  std::size_t v{};         ///< Second endpoint.
  std::uint64_t weight{};  ///< Integer weight in [1, n].
  std::size_t id{};        ///< Unique edge index.

  constexpr integer_edge() noexcept = default;

  constexpr integer_edge(std::size_t a, std::size_t b, std::uint64_t w,
                         std::size_t edge_id = 0) noexcept
      : u(a < b ? a : b), v(a < b ? b : a), weight(w), id(edge_id) {}

  constexpr bool operator==(const integer_edge &other) const noexcept {
    return u == other.u && v == other.v && weight == other.weight;
  }

  constexpr bool operator!=(const integer_edge &other) const noexcept {
    return !(*this == other);
  }
};

/**
 * @brief Result of a minimum spanning tree computation.
 *
 * @section complexity
 * Storage: $O(n)$ where $n = |V|$.
 */
struct mst_result {
  /// True if the graph is connected and all vertices are spanned.
  bool is_connected{false};

  /// Total weight of the minimum spanning tree.
  std::uint64_t total_weight{0};

  /// The collection of n - 1 edges forming the tree.
  std::vector<integer_edge> tree_edges{};

  /**
   * @brief Returns the number of edges in the spanning tree.
   * @return Edge count.
   *
   * @section complexity
   * Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] std::size_t size() const noexcept { return tree_edges.size(); }
};

/**
 * @class integer_graph
 * @brief Graph representation for undirected networks with bounded integer weights.
 */
class integer_graph {
public:
  struct neighbor {
    std::size_t to{};
    std::uint64_t weight{};
    std::size_t edge_id{};
  };

  /**
   * @brief Constructs an empty graph with n vertices.
   * @param num_vertices Vertex count $n$.
   *
   * @section complexity
   * Time: $O(n)$. Auxiliary space: $O(n)$.
   */
  explicit integer_graph(std::size_t num_vertices)
      : adj_(num_vertices), num_vertices_(num_vertices) {}

  /**
   * @brief Adds an undirected edge between u and v with integer weight in [1, max_w].
   * @param u First endpoint index ($0 \le u < n$).
   * @param v Second endpoint index ($0 \le v < n$).
   * @param weight Edge weight. Must be >= 1.
   * @throws std::out_of_range if u or v is out of bounds.
   * @throws std::invalid_argument if weight == 0 or u == v (self-loop).
   *
   * @section complexity
   * Time: $O(1)$ amortized. Auxiliary space: $O(1)$ amortized.
   */
  void add_edge(std::size_t u, std::size_t v, std::uint64_t weight) {
    validate_vertex(u, "add_edge: u out of range");
    validate_vertex(v, "add_edge: v out of range");
    if (u == v) {
      throw std::invalid_argument("add_edge: self-loops are not permitted");
    }
    if (weight == 0) {
      throw std::invalid_argument("add_edge: edge weights must be >= 1");
    }

    const std::size_t edge_id = edges_.size();
    adj_[u].push_back(neighbor{v, weight, edge_id});
    adj_[v].push_back(neighbor{u, weight, edge_id});
    edges_.push_back(integer_edge(u, v, weight, edge_id));
    if (weight > max_weight_) {
      max_weight_ = weight;
    }
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
  [[nodiscard]] std::size_t num_edges() const noexcept { return edges_.size(); }

  /**
   * @brief Returns the maximum edge weight present in the graph.
   * @return Maximum weight $W$.
   *
   * @section complexity
   * Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] std::uint64_t max_weight() const noexcept {
    return max_weight_;
  }

  /**
   * @brief Returns incident edges from vertex u.
   * @param u Vertex index.
   * @return Const reference to neighbor list.
   * @throws std::out_of_range if u is invalid.
   *
   * @section complexity
   * Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] const std::vector<neighbor> &adjacent(std::size_t u) const {
    validate_vertex(u, "adjacent: vertex out of range");
    return adj_[u];
  }

  /**
   * @brief Returns all edges in the graph.
   * @return Const reference to vector of edges.
   *
   * @section complexity
   * Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] const std::vector<integer_edge> &all_edges() const noexcept {
    return edges_;
  }

  /**
   * @brief Tests connectivity of the graph using Breadth-First Search.
   * @return True if connected or n <= 1, false otherwise.
   *
   * @section complexity
   * Time: $O(n + m)$. Auxiliary space: $O(n)$.
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

      for (const auto &nbr : adj_[u]) {
        if (!visited[nbr.to]) {
          visited[nbr.to] = true;
          q.push(nbr.to);
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

  std::vector<std::vector<neighbor>> adj_{};
  std::vector<integer_edge> edges_{};
  std::uint64_t max_weight_{0};
  std::size_t num_vertices_{0};
};

// ============================================================================
// Disjoint-Set Union (DSU) Structure
// ============================================================================

/**
 * @class disjoint_set
 * @brief Disjoint-Set Union (Union-Find) with path compression and union-by-rank.
 *
 * @section complexity
 * Any sequence of $m$ operations on $n$ elements takes $O(m \log^* n)$ time
 * (Hopcroft & Ullman 1973) and $O(m \alpha(n))$ time (Tarjan 1975).
 * Auxiliary space: $O(n)$.
 */
class disjoint_set {
public:
  /**
   * @brief Constructs a disjoint set containing n independent elements.
   * @param n Number of elements.
   *
   * @section complexity
   * Time: $O(n)$. Auxiliary space: $O(n)$.
   */
  explicit disjoint_set(std::size_t n) : parent_(n), rank_(n, 0), count_(n) {
    std::iota(parent_.begin(), parent_.end(), std::size_t{0});
  }

  /**
   * @brief Finds the representative root of element x with two-pass path compression.
   * @param x Element index ($0 \le x < n$).
   * @return Representative root.
   * @throws std::out_of_range if x is invalid.
   *
   * @section complexity
   * Time: $O(\alpha(n)) \subseteq O(\log^* n)$ amortized. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] std::size_t find(std::size_t x) {
    if (x >= parent_.size()) {
      throw std::out_of_range("disjoint_set::find: index out of range");
    }
    std::size_t root = x;
    while (root != parent_[root]) {
      root = parent_[root];
    }
    // Path compression
    std::size_t curr = x;
    while (curr != root) {
      std::size_t next = parent_[curr];
      parent_[curr] = root;
      curr = next;
    }
    return root;
  }

  /**
   * @brief Merges the sets containing x and y using union-by-rank.
   * @param x First element.
   * @param y Second element.
   * @return True if sets were merged, false if already in the same set.
   * @throws std::out_of_range if x or y is invalid.
   *
   * @section complexity
   * Time: $O(\alpha(n)) \subseteq O(\log^* n)$ amortized. Auxiliary space: $O(1)$.
   */
  bool union_sets(std::size_t x, std::size_t y) {
    std::size_t root_x = find(x);
    std::size_t root_y = find(y);
    if (root_x == root_y) {
      return false;
    }

    if (rank_[root_x] < rank_[root_y]) {
      std::swap(root_x, root_y);
    }
    parent_[root_y] = root_x;
    if (rank_[root_x] == rank_[root_y]) {
      ++rank_[root_x];
    }
    --count_;
    return true;
  }

  /**
   * @brief Checks whether x and y belong to the same set.
   * @param x First element.
   * @param y Second element.
   * @return True if connected, false otherwise.
   *
   * @section complexity
   * Time: $O(\alpha(n)) \subseteq O(\log^* n)$ amortized. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] bool connected(std::size_t x, std::size_t y) {
    return find(x) == find(y);
  }

  /**
   * @brief Returns the number of disjoint components.
   * @return Component count.
   *
   * @section complexity
   * Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] std::size_t count() const noexcept { return count_; }

private:
  std::vector<std::size_t> parent_{};
  std::vector<std::size_t> rank_{};
  std::size_t count_{0};
};

// ============================================================================
// Counting Sort for Integer Edges
// ============================================================================

/**
 * @brief Sorts m edges with integer weights in [1, max_w] in O(m + max_w) time.
 *
 * Uses stable bucket counting sort.
 * When max_w = O(n) and the graph is connected (m >= n - 1), this runs in strictly
 * linear O(m) time, bypassing the Omega(m log m) comparison lower bound.
 *
 * @param edges Vector of edges to sort.
 * @param max_w Upper bound on edge weights (typically n).
 * @return Sorted vector of edges in ascending order of weight.
 *
 * @section complexity
 * Time: $O(m + W)$ worst-case, where $m = |E|$ and $W = \text{max\_w}$.
 * Auxiliary space: $O(m + W)$ for count arrays and output buffer.
 */
[[nodiscard]] inline std::vector<integer_edge>
counting_sort_edges(const std::vector<integer_edge> &edges, std::size_t max_w) {
  const std::size_t m = edges.size();
  if (m <= 1) {
    return edges;
  }

  // Buckets from weight 0 to max_w (weights are 1-indexed)
  std::vector<std::size_t> count(max_w + 1, 0);
  for (const auto &e : edges) {
    assert(e.weight <= max_w && "Edge weight exceeds max_w");
    ++count[e.weight];
  }

  // Prefix sums for stable positioning
  for (std::size_t i = 1; i <= max_w; ++i) {
    count[i] += count[i - 1];
  }

  std::vector<integer_edge> sorted_edges(m);
  // Iterate in reverse to maintain stability
  for (auto it = edges.rbegin(); it != edges.rend(); ++it) {
    const std::size_t w = it->weight;
    sorted_edges[--count[w]] = *it;
  }

  return sorted_edges;
}

// ============================================================================
// Minimum Spanning Tree: O(m log* n) Algorithm (C-13.28)
// ============================================================================

/**
 * @brief Computes Minimum Spanning Tree in O(m log* n) time for weights in [1, n] (C-13.28).
 *
 * Steps:
 * 1. Sorts all m edges using Counting Sort in O(m + n) = O(m) time.
 * 2. Processes edges greedily using DSU with union-by-rank and path compression,
 *    taking O(m alpha(n)) <= O(m log* n) time.
 * 3. Early exits once n - 1 edges are selected.
 *
 * @param g The integer-weighted graph.
 * @return mst_result containing total weight, tree edges, and connectivity status.
 *
 * @section complexity
 * Time: $O(m + n + m \alpha(n)) = O(m \alpha(n)) \subseteq O(m \log^* n)$ worst-case,
 *       where $n = |V|$ and $m = |E|$.
 * Auxiliary space: $O(n + m)$ for sorted edge list and DSU structures.
 *
 * @note Exception safety: Strong guarantee; graph g is unmodified.
 */
[[nodiscard]] inline mst_result integer_weight_mst(const integer_graph &g) {
  const std::size_t n = g.num_vertices();
  mst_result result;

  if (n <= 1) {
    result.is_connected = true;
    return result;
  }

  // Max weight is at least n, or the maximum weight present in the graph
  const std::size_t max_w =
      std::max(n, static_cast<std::size_t>(g.max_weight()));

  // Step 1: Linear-time Counting Sort in O(m + n) time
  const std::vector<integer_edge> sorted_edges =
      counting_sort_edges(g.all_edges(), max_w);

  // Step 2: DSU Kruskal pass in O(m alpha(n)) <= O(m log* n) time
  disjoint_set dsu(n);
  result.tree_edges.reserve(n - 1);

  for (const auto &edge : sorted_edges) {
    if (dsu.union_sets(edge.u, edge.v)) {
      result.tree_edges.push_back(edge);
      result.total_weight += edge.weight;

      // Early exit: Spanning tree complete once n - 1 edges are selected
      if (result.tree_edges.size() == n - 1) {
        break;
      }
    }
  }

  result.is_connected = (result.tree_edges.size() == n - 1);
  return result;
}

// ============================================================================
// Reference Comparison-Based Kruskal MST (for Verification)
// ============================================================================

/**
 * @brief Standard O(m log m) comparison-based Kruskal MST used for verification.
 * @param g The integer-weighted graph.
 * @return mst_result.
 *
 * @section complexity
 * Time: $O(m \log m)$. Auxiliary space: $O(n + m)$.
 */
[[nodiscard]] inline mst_result comparison_kruskal_mst(const integer_graph &g) {
  const std::size_t n = g.num_vertices();
  mst_result result;

  if (n <= 1) {
    result.is_connected = true;
    return result;
  }

  std::vector<integer_edge> sorted_edges = g.all_edges();
  std::sort(sorted_edges.begin(), sorted_edges.end(),
            [](const integer_edge &lhs, const integer_edge &rhs) {
              return lhs.weight < rhs.weight;
            });

  disjoint_set dsu(n);
  result.tree_edges.reserve(n - 1);

  for (const auto &edge : sorted_edges) {
    if (dsu.union_sets(edge.u, edge.v)) {
      result.tree_edges.push_back(edge);
      result.total_weight += edge.weight;
      if (result.tree_edges.size() == n - 1) {
        break;
      }
    }
  }

  result.is_connected = (result.tree_edges.size() == n - 1);
  return result;
}

} // namespace dsa

#endif // INTEGER_WEIGHT_MST_HPP
