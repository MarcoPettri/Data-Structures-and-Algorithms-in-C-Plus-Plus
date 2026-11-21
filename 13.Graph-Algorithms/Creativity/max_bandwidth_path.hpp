/**
 * @file max_bandwidth_path.hpp
 * @brief Maximum Bandwidth (Widest) Path algorithms for telephone networks (C-13.19).
 *
 * Problem Statement (C-13.19):
 * Consider a diagram of a telephone network, which is a graph G whose vertices
 * represent switching centers and whose edges represent communication lines
 * joining pairs of centers. Edges are marked by their bandwidth, and the
 * bandwidth of a path is the bandwidth of its lowest bandwidth edge.
 * Give an algorithm that, given a diagram and two switching centers a and b,
 * outputs the maximum bandwidth of a path between a and b.
 *
 * This header provides:
 *   - telephone_network<Weight>: Adjacency-list graph model for switching centers
 *     and communication lines (supporting both undirected and directed lines).
 *   - disjoint_set: Disjoint-Set Union (DSU) data structure with path compression
 *     and union-by-rank.
 *   - max_bandwidth_dijkstra: Modified Dijkstra (Widest Path) greedy algorithm
 *     using a max-priority queue. Works on both directed and undirected graphs.
 *   - max_bandwidth_kruskal: Maximum Spanning Forest approach with early exit,
 *     optimal for undirected telephone networks.
 *   - max_bandwidth_binary_search: Binary search on bandwidth thresholds with
 *     BFS reachability; acts as an independent verification oracle.
 */

#ifndef MAX_BANDWIDTH_PATH_HPP
#define MAX_BANDWIDTH_PATH_HPP

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <limits>
#include <numeric>
#include <optional>
#include <queue>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace dsa {

// ============================================================================
// Data Structures: Edge and Result Types
// ============================================================================

/**
 * @brief An outgoing communication line in an adjacency list.
 * @tparam Weight Arithmetic type representing line bandwidth (e.g., double, int64_t).
 *
 * @section complexity
 * Storage: $O(1)$ auxiliary space.
 */
template <typename Weight = double> struct line_edge {
  static_assert(std::is_arithmetic_v<Weight>,
                "Weight must be a numeric arithmetic type.");
  std::size_t to{}; ///< Destination switching center.
  Weight bandwidth{}; ///< Bandwidth capacity of the communication line.
};

/**
 * @brief An undirected communication line between two switching centers.
 * @tparam Weight Arithmetic type representing line bandwidth.
 *
 * @section complexity
 * Storage: $O(1)$ auxiliary space.
 */
template <typename Weight = double> struct undirected_line {
  static_assert(std::is_arithmetic_v<Weight>,
                "Weight must be a numeric arithmetic type.");
  std::size_t u{};    ///< First endpoint switching center.
  std::size_t v{};    ///< Second endpoint switching center.
  Weight bandwidth{}; ///< Bandwidth capacity of the communication line.
};

/**
 * @brief Result of a maximum bandwidth path query.
 * @tparam Weight Arithmetic type representing line bandwidth.
 *
 * @section complexity
 * Storage: $O(k)$ auxiliary space where $k$ is the number of vertices in the path.
 */
template <typename Weight = double> struct bandwidth_result {
  /// Maximum bottleneck bandwidth of a path between source and destination.
  /// std::nullopt indicates that no path exists (centers are disconnected).
  std::optional<Weight> max_bandwidth{std::nullopt};

  /// Sequence of switching centers along the maximum bandwidth path,
  /// starting with the source and ending with the destination.
  /// Empty if no path exists.
  std::vector<std::size_t> path{};

  /**
   * @brief Checks if a path exists between the query switching centers.
   * @return True if reachable, false otherwise.
   *
   * @section complexity
   * Time: $O(1)$ worst-case. Space: $O(1)$ auxiliary.
   */
  [[nodiscard]] constexpr bool has_path() const noexcept {
    return max_bandwidth.has_value();
  }

  /**
   * @brief Returns the number of communication hops in the path.
   * @return Number of edges traversed, or 0 if empty or single vertex.
   *
   * @section complexity
   * Time: $O(1)$ worst-case. Space: $O(1)$ auxiliary.
   */
  [[nodiscard]] std::size_t hop_count() const noexcept {
    return path.size() > 1 ? path.size() - 1 : 0;
  }
};

// ============================================================================
// Telephone Network Graph Class
// ============================================================================

/**
 * @class telephone_network
 * @brief Graph representation of a telephone network diagram.
 *
 * Switching centers are 0-indexed vertices $\{0, 1, \dots, n - 1\}$.
 * Communication lines are weighted edges marked by their bandwidth.
 *
 * @tparam Weight Arithmetic type representing bandwidth (e.g. double, uint64_t).
 */
template <typename Weight = double> class telephone_network {
public:
  static_assert(std::is_arithmetic_v<Weight>,
                "Weight must be a numeric arithmetic type.");

  /**
   * @brief Constructs a network with a fixed number of switching centers.
   * @param num_centers Total number of switching centers $n$.
   *
   * @section complexity
   * Time: $O(n)$ worst-case. Auxiliary space: $O(n)$ for adjacency lists.
   */
  explicit telephone_network(std::size_t num_centers)
      : adj_(num_centers), num_centers_(num_centers) {}

  /**
   * @brief Adds a communication line between centers u and v.
   * @param u First switching center index ($0 \le u < n$).
   * @param v Second switching center index ($0 \le v < n$).
   * @param bandwidth Bandwidth of the line (must be $\ge 0$).
   * @param bidirectional True if line carries traffic both ways; false for directed.
   * @throws std::out_of_range if u or v is out of bounds.
   * @throws std::invalid_argument if bandwidth < 0.
   *
   * @section complexity
   * Time: $O(1)$ amortized worst-case. Auxiliary space: $O(1)$ amortized.
   */
  void add_line(std::size_t u, std::size_t v, Weight bandwidth,
                bool bidirectional = true) {
    validate_center(u, "add_line: vertex u out of range");
    validate_center(v, "add_line: vertex v out of range");
    if (bandwidth < Weight{0}) {
      throw std::invalid_argument("add_line: bandwidth must be non-negative");
    }

    adj_[u].push_back(line_edge<Weight>{v, bandwidth});
    if (bidirectional) {
      if (u != v) {
        adj_[v].push_back(line_edge<Weight>{u, bandwidth});
      }
      lines_.push_back(undirected_line<Weight>{u, v, bandwidth});
    } else {
      lines_.push_back(undirected_line<Weight>{u, v, bandwidth});
    }
  }

  /**
   * @brief Returns the total number of switching centers $n$.
   * @return Number of centers.
   *
   * @section complexity
   * Time: $O(1)$ worst-case. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] std::size_t num_centers() const noexcept {
    return num_centers_;
  }

  /**
   * @brief Returns the total number of communication lines added.
   * @return Number of lines $m$.
   *
   * @section complexity
   * Time: $O(1)$ worst-case. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] std::size_t num_lines() const noexcept { return lines_.size(); }

  /**
   * @brief Accesses outgoing lines from switching center u.
   * @param u Switching center index ($0 \le u < n$).
   * @return Const reference to vector of outgoing lines.
   * @throws std::out_of_range if u is invalid.
   *
   * @section complexity
   * Time: $O(1)$ worst-case. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] const std::vector<line_edge<Weight>> &
  adjacent(std::size_t u) const {
    validate_center(u, "adjacent: vertex u out of range");
    return adj_[u];
  }

  /**
   * @brief Returns the collection of all registered communication lines.
   * @return Const reference to vector of lines.
   *
   * @section complexity
   * Time: $O(1)$ worst-case. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] const std::vector<undirected_line<Weight>> &
  all_lines() const noexcept {
    return lines_;
  }

private:
  void validate_center(std::size_t u, const char *msg) const {
    if (u >= num_centers_) {
      throw std::out_of_range(msg);
    }
  }

  std::vector<std::vector<line_edge<Weight>>> adj_{};
  std::vector<undirected_line<Weight>> lines_{};
  std::size_t num_centers_{0};
};

// ============================================================================
// Disjoint-Set Union (DSU) Structure
// ============================================================================

/**
 * @class disjoint_set
 * @brief Disjoint-Set Union (Union-Find) with path compression and union-by-rank.
 *
 * @section complexity
 * Operations find() and union_sets() run in amortized $O(\alpha(n))$ time,
 * where $\alpha$ is the inverse Ackermann function ($\alpha(n) \le 4$ for all practical $n$).
 * Auxiliary space: $O(n)$ for parent and rank arrays.
 */
class disjoint_set {
public:
  /**
   * @brief Constructs a disjoint set containing n independent elements.
   * @param n Number of elements.
   *
   * @section complexity
   * Time: $O(n)$ worst-case. Auxiliary space: $O(n)$.
   */
  explicit disjoint_set(std::size_t n) : parent_(n), rank_(n, 0), count_(n) {
    std::iota(parent_.begin(), parent_.end(), std::size_t{0});
  }

  /**
   * @brief Finds the representative root of element x with path compression.
   * @param x Element index ($0 \le x < n$).
   * @return Representative root of the set containing x.
   * @throws std::out_of_range if x is invalid.
   *
   * @section complexity
   * Time: $O(\alpha(n))$ amortized. Auxiliary space: $O(1)$ auxiliary.
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
   * @brief Merges the sets containing elements x and y using union-by-rank.
   * @param x First element.
   * @param y Second element.
   * @return True if sets were disjoint and merged; false if already in the same set.
   * @throws std::out_of_range if x or y is invalid.
   *
   * @section complexity
   * Time: $O(\alpha(n))$ amortized. Auxiliary space: $O(1)$.
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
   * Time: $O(\alpha(n))$ amortized. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] bool connected(std::size_t x, std::size_t y) {
    return find(x) == find(y);
  }

  /**
   * @brief Returns the current number of disjoint sets.
   * @return Component count.
   *
   * @section complexity
   * Time: $O(1)$ worst-case. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] std::size_t count() const noexcept { return count_; }

  /**
   * @brief Returns the total capacity of elements.
   * @return Total elements $n$.
   *
   * @section complexity
   * Time: $O(1)$ worst-case. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] std::size_t size() const noexcept { return parent_.size(); }

private:
  std::vector<std::size_t> parent_{};
  std::vector<std::size_t> rank_{};
  std::size_t count_{0};
};

// ============================================================================
// Maximum Bandwidth Path: Dijkstra-style Greedy Algorithm
// ============================================================================

/**
 * @brief Computes the maximum bandwidth path from center a to b using a modified Dijkstra.
 *
 * Maintains the maximum bottleneck bandwidth to each switching center using a
 * max-priority queue. At each step, the unvisited center with the highest bottleneck
 * is extracted. When the destination center b is settled, its bottleneck is optimal.
 * Works on both directed and undirected graphs.
 *
 * @tparam Weight Arithmetic type representing line bandwidth.
 * @param network The telephone network diagram.
 * @param src Source switching center a ($0 \le a < n$).
 * @param dst Destination switching center b ($0 \le b < n$).
 * @return bandwidth_result containing the maximum bottleneck bandwidth and the vertex path.
 * @throws std::out_of_range if src or dst is invalid.
 *
 * @section complexity
 * Time: $O((n + m) \log n)$ worst-case with binary max-heap, where $n = |V|$ is the number
 *       of switching centers and $m = |E|$ is the number of communication lines.
 * Auxiliary space: $O(n + m)$ for priority queue and predecessor tracking.
 *
 * @note Exception safety: Strong guarantee; input network is unchanged.
 */
template <typename Weight = double>
[[nodiscard]] bandwidth_result<Weight>
max_bandwidth_dijkstra(const telephone_network<Weight> &network,
                       std::size_t src, std::size_t dst) {
  const std::size_t n = network.num_centers();
  if (src >= n || dst >= n) {
    throw std::out_of_range(
        "max_bandwidth_dijkstra: center index out of range");
  }

  bandwidth_result<Weight> res;

  // Trivial zero-hop path
  if (src == dst) {
    if constexpr (std::numeric_limits<Weight>::has_infinity) {
      res.max_bandwidth = std::numeric_limits<Weight>::infinity();
    } else {
      res.max_bandwidth = std::numeric_limits<Weight>::max();
    }
    res.path = {src};
    return res;
  }

  // bottleneck[v] tracks the maximum known bottleneck bandwidth of any path from src to v.
  std::vector<Weight> bottleneck(n, Weight{0});
  std::vector<bool> settled(n, false);
  std::vector<std::size_t> parent(n, std::numeric_limits<std::size_t>::max());

  // Max-priority queue storing pairs of (bottleneck_capacity, vertex)
  using queue_entry = std::pair<Weight, std::size_t>;
  std::priority_queue<queue_entry> pq;

  // Initialize source with unbounded bandwidth
  const Weight inf_bw = std::numeric_limits<Weight>::has_infinity
                            ? std::numeric_limits<Weight>::infinity()
                            : std::numeric_limits<Weight>::max();
  bottleneck[src] = inf_bw;
  pq.push({inf_bw, src});

  while (!pq.empty()) {
    const auto [curr_bw, u] = pq.top();
    pq.pop();

    if (settled[u]) {
      continue;
    }
    settled[u] = true;

    // Early termination: destination reached with optimal bottleneck
    if (u == dst) {
      break;
    }

    for (const auto &line : network.adjacent(u)) {
      const std::size_t v = line.to;
      const Weight cand_bw = std::min(curr_bw, line.bandwidth);

      if (cand_bw > bottleneck[v]) {
        bottleneck[v] = cand_bw;
        parent[v] = u;
        pq.push({cand_bw, v});
      }
    }
  }

  if (!settled[dst] || bottleneck[dst] == Weight{0}) {
    // b is not reachable from a
    return res;
  }

  res.max_bandwidth = bottleneck[dst];

  // Reconstruct path from dst to src using parent pointers
  std::vector<std::size_t> rev_path;
  for (std::size_t curr = dst; curr != std::numeric_limits<std::size_t>::max();
       curr = parent[curr]) {
    rev_path.push_back(curr);
    if (curr == src) {
      break;
    }
  }
  std::reverse(rev_path.begin(), rev_path.end());
  res.path = std::move(rev_path);

  return res;
}

// ============================================================================
// Maximum Bandwidth Path: Kruskal / Maximum Spanning Forest Algorithm
// ============================================================================

/**
 * @brief Computes the maximum bandwidth between centers a and b using Kruskal's MSF algorithm.
 *
 * For undirected telephone networks, sorts all communication lines in descending order
 * of bandwidth and unions their endpoints using Disjoint-Set Union. The very first
 * line whose union connects switching centers a and b is guaranteed to be the bottleneck
 * of the maximum bandwidth path.
 *
 * To reconstruct the exact path, edges added to the forest are stored, and a DFS is run
 * in the resulting tree between a and b.
 *
 * @tparam Weight Arithmetic type representing line bandwidth.
 * @param network The telephone network diagram (treated as undirected).
 * @param src Source switching center a ($0 \le a < n$).
 * @param dst Destination switching center b ($0 \le b < n$).
 * @return bandwidth_result with maximum bottleneck and vertex path.
 * @throws std::out_of_range if src or dst is invalid.
 *
 * @section complexity
 * Time: $O(m \log m + m \alpha(n))$ worst-case, where $m = |E|$ is the number of lines,
 *       and $\alpha$ is the inverse Ackermann function. Can terminate earlier if a and b
 *       are connected by high-bandwidth lines.
 * Auxiliary space: $O(n + m)$ for sorting, DSU, and forest representation.
 *
 * @note Exception safety: Strong guarantee; network is unchanged.
 */
template <typename Weight = double>
[[nodiscard]] bandwidth_result<Weight>
max_bandwidth_kruskal(const telephone_network<Weight> &network, std::size_t src,
                      std::size_t dst) {
  const std::size_t n = network.num_centers();
  if (src >= n || dst >= n) {
    throw std::out_of_range("max_bandwidth_kruskal: center index out of range");
  }

  bandwidth_result<Weight> res;

  // Trivial zero-hop path
  if (src == dst) {
    if constexpr (std::numeric_limits<Weight>::has_infinity) {
      res.max_bandwidth = std::numeric_limits<Weight>::infinity();
    } else {
      res.max_bandwidth = std::numeric_limits<Weight>::max();
    }
    res.path = {src};
    return res;
  }

  // Collect and sort all communication lines in descending order of bandwidth
  std::vector<undirected_line<Weight>> sorted_lines = network.all_lines();
  std::sort(sorted_lines.begin(), sorted_lines.end(),
            [](const undirected_line<Weight> &lhs,
               const undirected_line<Weight> &rhs) {
              return lhs.bandwidth > rhs.bandwidth;
            });

  disjoint_set dsu(n);
  std::vector<std::vector<std::size_t>> forest_adj(n);
  std::optional<Weight> found_bottleneck{std::nullopt};

  for (const auto &line : sorted_lines) {
    if (dsu.union_sets(line.u, line.v)) {
      forest_adj[line.u].push_back(line.v);
      forest_adj[line.v].push_back(line.u);

      if (dsu.connected(src, dst)) {
        found_bottleneck = line.bandwidth;
        break; // Early termination: a and b just became connected!
      }
    }
  }

  if (!found_bottleneck.has_value()) {
    return res; // Unreachable
  }

  res.max_bandwidth = found_bottleneck;

  // DFS in the maximum spanning forest to find the unique path between src and dst
  std::vector<std::size_t> path;
  std::vector<bool> visited(n, false);

  auto dfs = [&](auto &self, std::size_t curr) -> bool {
    visited[curr] = true;
    path.push_back(curr);
    if (curr == dst) {
      return true;
    }
    for (std::size_t nxt : forest_adj[curr]) {
      if (!visited[nxt]) {
        if (self(self, nxt)) {
          return true;
        }
      }
    }
    path.pop_back();
    return false;
  };

  dfs(dfs, src);
  res.path = std::move(path);

  return res;
}

// ============================================================================
// Maximum Bandwidth Path: Binary Search Verification Oracle
// ============================================================================

/**
 * @brief Independent verification oracle using binary search over bandwidth thresholds.
 *
 * Collects distinct bandwidth values across all lines, sorts them, and uses binary search
 * to find the maximal threshold $W$ such that destination b is reachable from source a
 * in the subgraph $G' = (V, \{e \in E \mid bandwidth(e) \ge W\})$ via Breadth-First Search.
 *
 * @tparam Weight Arithmetic type representing line bandwidth.
 * @param network The telephone network diagram.
 * @param src Source switching center a.
 * @param dst Destination switching center b.
 * @return std::optional<Weight> containing the maximum bottleneck, or nullopt if unreachable.
 * @throws std::out_of_range if src or dst is invalid.
 *
 * @section complexity
 * Time: $O((n + m) \log m)$ worst-case, where $n = |V|$ and $m = |E|$.
 * Auxiliary space: $O(n + m)$ for threshold collection and BFS queue.
 */
template <typename Weight = double>
[[nodiscard]] std::optional<Weight>
max_bandwidth_binary_search(const telephone_network<Weight> &network,
                            std::size_t src, std::size_t dst) {
  const std::size_t n = network.num_centers();
  if (src >= n || dst >= n) {
    throw std::out_of_range("max_bandwidth_binary_search: center out of range");
  }

  if (src == dst) {
    if constexpr (std::numeric_limits<Weight>::has_infinity) {
      return std::numeric_limits<Weight>::infinity();
    } else {
      return std::numeric_limits<Weight>::max();
    }
  }

  // Gather distinct edge bandwidths
  std::vector<Weight> thresholds;
  thresholds.reserve(network.all_lines().size());
  for (const auto &line : network.all_lines()) {
    thresholds.push_back(line.bandwidth);
  }
  std::sort(thresholds.begin(), thresholds.end());
  thresholds.erase(std::unique(thresholds.begin(), thresholds.end()),
                   thresholds.end());

  if (thresholds.empty()) {
    return std::nullopt;
  }

  // Helper lambda: BFS reachability check with threshold
  auto is_reachable = [&](Weight min_bw) -> bool {
    std::vector<bool> vis(n, false);
    std::queue<std::size_t> q;
    vis[src] = true;
    q.push(src);

    while (!q.empty()) {
      std::size_t u = q.front();
      q.pop();
      if (u == dst) {
        return true;
      }
      for (const auto &line : network.adjacent(u)) {
        if (!vis[line.to] && line.bandwidth >= min_bw) {
          vis[line.to] = true;
          q.push(line.to);
        }
      }
    }
    return false;
  };

  // Check if even the smallest bandwidth line enables a path
  if (!is_reachable(thresholds.front())) {
    return std::nullopt;
  }

  std::size_t low = 0;
  std::size_t high = thresholds.size() - 1;
  std::size_t best_idx = 0;

  while (low <= high) {
    std::size_t mid = low + (high - low) / 2;
    if (is_reachable(thresholds[mid])) {
      best_idx = mid;
      low = mid + 1; // Try higher bandwidth
    } else {
      if (mid == 0) {
        break;
      }
      high = mid - 1; // Try lower bandwidth
    }
  }

  return thresholds[best_idx];
}

} // namespace dsa

#endif // MAX_BANDWIDTH_PATH_HPP
