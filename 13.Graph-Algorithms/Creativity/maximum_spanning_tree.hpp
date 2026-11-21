/**
 * @file maximum_spanning_tree.hpp
 * @brief Maximum Spanning Tree (MaxST) algorithms for network channel selection (C-13.21).
 *
 * This header provides:
 *   - weighted_edge<Weight>: Undirected edge with arithmetic weight (bandwidth).
 *   - station_network<Weight>: Adjacency-list graph model for NASA stations and channels.
 *   - spanning_tree_result<Weight>: Container holding total bandwidth, tree edges,
 *     and connectivity status.
 *   - disjoint_set: Union-Find structure with union-by-rank and path compression.
 *   - maximum_spanning_tree_kruskal: Greedy Kruskal algorithm with reverse sorting.
 *   - maximum_spanning_tree_prim: Greedy Prim algorithm with a max-priority queue.
 *   - minimum_spanning_tree_kruskal: Dual MST algorithm for comparison and duality checks.
 */

#ifndef MAXIMUM_SPANNING_TREE_HPP
#define MAXIMUM_SPANNING_TREE_HPP

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <numeric>
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
 * @brief An undirected weighted communication channel between two stations.
 * @tparam Weight Arithmetic numeric type representing bandwidth.
 *
 * @section complexity
 * Storage: $O(1)$ auxiliary space.
 */
template <typename Weight = double> struct weighted_edge {
  static_assert(std::is_arithmetic_v<Weight>,
                "Weight must be a numeric arithmetic type.");
  std::size_t u{};    ///< First station index.
  std::size_t v{};    ///< Second station index.
  Weight bandwidth{}; ///< Channel bandwidth.
  std::size_t id{};   ///< Unique channel identifier.

  constexpr weighted_edge() noexcept = default;

  /**
   * @brief Constructs an undirected channel.
   * @param a First station.
   * @param b Second station.
   * @param bw Channel bandwidth.
   * @param edge_id Optional identifier.
   */
  constexpr weighted_edge(std::size_t a, std::size_t b, Weight bw,
                          std::size_t edge_id = 0) noexcept
      : u(a < b ? a : b), v(a < b ? b : a), bandwidth(bw), id(edge_id) {}

  constexpr bool operator==(const weighted_edge &other) const noexcept {
    return u == other.u && v == other.v && bandwidth == other.bandwidth;
  }

  constexpr bool operator!=(const weighted_edge &other) const noexcept {
    return !(*this == other);
  }
};

/**
 * @brief Result of a spanning tree computation.
 * @tparam Weight Arithmetic numeric type.
 *
 * @section complexity
 * Storage: $O(n)$ where $n$ is the number of stations in the tree.
 */
template <typename Weight = double> struct spanning_tree_result {
  /// Whether all stations in the network are linked into a single tree.
  bool is_connected{false};

  /// Total bandwidth: sum of the individual bandwidths of the selected channels.
  Weight total_bandwidth{Weight{0}};

  /// The collection of channels forming the spanning tree ($n - 1$ edges if connected).
  std::vector<weighted_edge<Weight>> tree_edges{};

  /**
   * @brief Returns the number of channels selected.
   * @return Channel count.
   *
   * @section complexity
   * Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] std::size_t size() const noexcept { return tree_edges.size(); }
};

// ============================================================================
// Graph Class: Station Network
// ============================================================================

/**
 * @class station_network
 * @brief Adjacency-list graph model for NASA communication stations and channels.
 *
 * Stations are 0-indexed vertices $\{0, 1, \dots, n - 1\}$.
 * Channels are weighted undirected edges marked by bandwidth $\ge 0$.
 *
 * @tparam Weight Arithmetic numeric type.
 */
template <typename Weight = double> class station_network {
public:
  static_assert(std::is_arithmetic_v<Weight>,
                "Weight must be a numeric arithmetic type.");

  struct neighbor {
    std::size_t to{};
    Weight bandwidth{};
    std::size_t edge_id{};
  };

  /**
   * @brief Constructs a network with n stations and no initial channels.
   * @param num_stations Total number of stations $n$.
   *
   * @section complexity
   * Time: $O(n)$ worst-case. Auxiliary space: $O(n)$.
   */
  explicit station_network(std::size_t num_stations)
      : adj_(num_stations), num_stations_(num_stations) {}

  /**
   * @brief Adds a communication channel between stations u and v.
   * @param u First station index ($0 \le u < n$).
   * @param v Second station index ($0 \le v < n$).
   * @param bandwidth Available bandwidth capacity (must be $\ge 0$).
   * @throws std::out_of_range if u or v is out of bounds.
   * @throws std::invalid_argument if u == v (self-loop) or bandwidth < 0.
   *
   * @section complexity
   * Time: $O(1)$ amortized worst-case. Auxiliary space: $O(1)$ amortized.
   */
  void add_channel(std::size_t u, std::size_t v, Weight bandwidth) {
    validate_station(u, "add_channel: station u out of range");
    validate_station(v, "add_channel: station v out of range");
    if (u == v) {
      throw std::invalid_argument(
          "add_channel: self-channels are not permitted");
    }
    if (bandwidth < Weight{0}) {
      throw std::invalid_argument(
          "add_channel: bandwidth must be non-negative");
    }

    const std::size_t channel_id = channels_.size();
    adj_[u].push_back(neighbor{v, bandwidth, channel_id});
    adj_[v].push_back(neighbor{u, bandwidth, channel_id});
    channels_.push_back(weighted_edge<Weight>(u, v, bandwidth, channel_id));
  }

  /**
   * @brief Returns the total number of stations $n$.
   * @return Number of stations.
   *
   * @section complexity
   * Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] std::size_t num_stations() const noexcept {
    return num_stations_;
  }

  /**
   * @brief Returns the total number of communication channels $m$.
   * @return Number of channels.
   *
   * @section complexity
   * Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] std::size_t num_channels() const noexcept {
    return channels_.size();
  }

  /**
   * @brief Returns all incident channels from station u.
   * @param u Station index ($0 \le u < n$).
   * @return Const reference to neighbor list.
   * @throws std::out_of_range if u is invalid.
   *
   * @section complexity
   * Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] const std::vector<neighbor> &adjacent(std::size_t u) const {
    validate_station(u, "adjacent: station out of range");
    return adj_[u];
  }

  /**
   * @brief Returns all communication channels in the network.
   * @return Const reference to vector of channels.
   *
   * @section complexity
   * Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] const std::vector<weighted_edge<Weight>> &
  all_channels() const noexcept {
    return channels_;
  }

  /**
   * @brief Tests whether the network is fully connected.
   * @return True if all stations are connected or n <= 1, false otherwise.
   *
   * @section complexity
   * Time: $O(n + m)$ worst-case, where $n = |V|$ and $m = |E|$.
   * Auxiliary space: $O(n)$ for BFS traversal.
   */
  [[nodiscard]] bool is_connected() const {
    if (num_stations_ <= 1) {
      return true;
    }
    std::vector<bool> visited(num_stations_, false);
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
    return count == num_stations_;
  }

private:
  void validate_station(std::size_t u, const char *msg) const {
    if (u >= num_stations_) {
      throw std::out_of_range(msg);
    }
  }

  std::vector<std::vector<neighbor>> adj_{};
  std::vector<weighted_edge<Weight>> channels_{};
  std::size_t num_stations_{0};
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
   * @brief Finds the representative root of element x with two-pass path compression.
   * @param x Element index ($0 \le x < n$).
   * @return Representative root of the set containing x.
   * @throws std::out_of_range if x is invalid.
   *
   * @section complexity
   * Time: $O(\alpha(n))$ amortized. Auxiliary space: $O(1)$.
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
   * Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] std::size_t count() const noexcept { return count_; }

private:
  std::vector<std::size_t> parent_{};
  std::vector<std::size_t> rank_{};
  std::size_t count_{0};
};

// ============================================================================
// Maximum Spanning Tree: Kruskal's Algorithm
// ============================================================================

/**
 * @brief Computes the Maximum Spanning Tree using Kruskal's greedy algorithm.
 *
 * Sorts all channels in descending order of bandwidth and greedily incorporates
 * channels that connect previously disconnected components using a Disjoint-Set Union
 * data structure.
 *
 * @tparam Weight Arithmetic numeric type representing channel bandwidth.
 * @param network The NASA station network graph.
 * @return spanning_tree_result with total bandwidth, the n - 1 channels, and connectivity status.
 *
 * @section complexity
 * Time: $O(m \log m) = O(m \log n)$ worst-case, where $n = |V|$ is the number of stations
 *       and $m = |E|$ is the number of channels. Edge sorting dominates; DSU operations
 *       take $O(m \alpha(n))$. Early exits once $n - 1$ channels are chosen.
 * Auxiliary space: $O(n + m)$ for sorted edges, DSU parent/rank tables, and tree edges.
 *
 * @note Exception safety: Strong guarantee; input network is unmodified.
 */
template <typename Weight = double>
[[nodiscard]] spanning_tree_result<Weight>
maximum_spanning_tree_kruskal(const station_network<Weight> &network) {
  const std::size_t n = network.num_stations();
  spanning_tree_result<Weight> result;

  if (n <= 1) {
    result.is_connected = true;
    return result;
  }

  // Copy and sort channels in descending order of bandwidth
  std::vector<weighted_edge<Weight>> sorted_channels = network.all_channels();
  std::sort(
      sorted_channels.begin(), sorted_channels.end(),
      [](const weighted_edge<Weight> &lhs, const weighted_edge<Weight> &rhs) {
        return lhs.bandwidth > rhs.bandwidth;
      });

  disjoint_set dsu(n);
  result.tree_edges.reserve(n - 1);

  for (const auto &channel : sorted_channels) {
    if (dsu.union_sets(channel.u, channel.v)) {
      result.tree_edges.push_back(channel);
      result.total_bandwidth += channel.bandwidth;

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
// Maximum Spanning Tree: Prim's Algorithm
// ============================================================================

/**
 * @brief Computes the Maximum Spanning Tree using Prim's greedy algorithm.
 *
 * Grows a single spanning tree from a start station by repeatedly adding the
 * highest-bandwidth channel linking a station inside the tree to a station outside.
 * Uses a max-priority queue storing incident candidate channels.
 *
 * @tparam Weight Arithmetic numeric type representing channel bandwidth.
 * @param network The NASA station network graph.
 * @param start Starting station index (default: 0).
 * @return spanning_tree_result with total bandwidth, chosen channels, and connectivity status.
 * @throws std::out_of_range if start is invalid and n > 0.
 *
 * @section complexity
 * Time: $O(m \log n)$ worst-case using a binary max-heap.
 * Auxiliary space: $O(n + m)$ for the priority queue, visited flags, and tree edges.
 *
 * @note Exception safety: Strong guarantee; input network is unmodified.
 */
template <typename Weight = double>
[[nodiscard]] spanning_tree_result<Weight>
maximum_spanning_tree_prim(const station_network<Weight> &network,
                           std::size_t start = 0) {
  const std::size_t n = network.num_stations();
  if (n == 0) {
    spanning_tree_result<Weight> empty_res;
    empty_res.is_connected = true;
    return empty_res;
  }
  if (start >= n) {
    throw std::out_of_range(
        "maximum_spanning_tree_prim: start station out of range");
  }

  spanning_tree_result<Weight> result;
  if (n == 1) {
    result.is_connected = true;
    return result;
  }

  std::vector<bool> in_tree(n, false);
  result.tree_edges.reserve(n - 1);

  // Comparator for max-heap: highest bandwidth at the top
  struct pq_comparator {
    bool operator()(const weighted_edge<Weight> &lhs,
                    const weighted_edge<Weight> &rhs) const noexcept {
      return lhs.bandwidth < rhs.bandwidth;
    }
  };

  std::priority_queue<weighted_edge<Weight>, std::vector<weighted_edge<Weight>>,
                      pq_comparator>
      pq;

  in_tree[start] = true;
  for (const auto &nbr : network.adjacent(start)) {
    pq.push(weighted_edge<Weight>(start, nbr.to, nbr.bandwidth, nbr.edge_id));
  }

  while (!pq.empty() && result.tree_edges.size() < n - 1) {
    const weighted_edge<Weight> best_channel = pq.top();
    pq.pop();

    const std::size_t u = best_channel.u;
    const std::size_t v = best_channel.v;

    // Identify which endpoint is newly incorporated
    const std::size_t new_station = (!in_tree[u]) ? u : ((!in_tree[v]) ? v : n);

    if (new_station == n) {
      // Both endpoints already inside tree: cycle formed, skip
      continue;
    }

    in_tree[new_station] = true;
    result.tree_edges.push_back(best_channel);
    result.total_bandwidth += best_channel.bandwidth;

    for (const auto &nbr : network.adjacent(new_station)) {
      if (!in_tree[nbr.to]) {
        pq.push(weighted_edge<Weight>(new_station, nbr.to, nbr.bandwidth,
                                      nbr.edge_id));
      }
    }
  }

  result.is_connected = (result.tree_edges.size() == n - 1);
  return result;
}

// ============================================================================
// Dual: Minimum Spanning Tree (for Duality Verification)
// ============================================================================

/**
 * @brief Computes the Minimum Spanning Tree using Kruskal's algorithm (for duality checking).
 *
 * @tparam Weight Arithmetic numeric type.
 * @param network The NASA station network graph.
 * @return spanning_tree_result with total minimum bandwidth and tree channels.
 *
 * @section complexity
 * Time: $O(m \log n)$. Auxiliary space: $O(n + m)$.
 */
template <typename Weight = double>
[[nodiscard]] spanning_tree_result<Weight>
minimum_spanning_tree_kruskal(const station_network<Weight> &network) {
  const std::size_t n = network.num_stations();
  spanning_tree_result<Weight> result;

  if (n <= 1) {
    result.is_connected = true;
    return result;
  }

  std::vector<weighted_edge<Weight>> sorted_channels = network.all_channels();
  std::sort(
      sorted_channels.begin(), sorted_channels.end(),
      [](const weighted_edge<Weight> &lhs, const weighted_edge<Weight> &rhs) {
        return lhs.bandwidth < rhs.bandwidth;
      });

  disjoint_set dsu(n);
  result.tree_edges.reserve(n - 1);

  for (const auto &channel : sorted_channels) {
    if (dsu.union_sets(channel.u, channel.v)) {
      result.tree_edges.push_back(channel);
      result.total_bandwidth += channel.bandwidth;
      if (result.tree_edges.size() == n - 1) {
        break;
      }
    }
  }

  result.is_connected = (result.tree_edges.size() == n - 1);
  return result;
}

} // namespace dsa

#endif // MAXIMUM_SPANNING_TREE_HPP
