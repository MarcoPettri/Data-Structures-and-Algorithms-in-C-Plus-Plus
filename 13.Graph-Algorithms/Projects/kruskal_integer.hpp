/**
 * @file kruskal_integer.hpp
 * @brief Kruskal's minimum spanning forest algorithm for integer weights.
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace dsa {

/**
 * @brief Undirected edge with a signed integer weight.
 * @section complexity
 * Storage: $O(1)$ per edge.
 */
struct kruskal_edge {
  std::size_t u{};       ///< First endpoint.
  std::size_t v{};       ///< Second endpoint.
  std::int64_t weight{}; ///< Edge weight; negative values are allowed.
};

/**
 * @brief Result of a minimum spanning forest computation.
 * @section complexity
 * Storage: $O(n)$, where $n$ is the number of vertices.
 */
struct kruskal_result {
  bool is_connected{false}; ///< Whether all vertices are spanned.
  std::int64_t total_weight{0}; ///< Sum of selected edge weights.
  std::vector<kruskal_edge> tree_edges{}; ///< Forest edges, at most $n-1$.
};

namespace detail {

/** @brief Disjoint-set union with path compression and union by size. */
class disjoint_set {
public:
  /**
   * @brief Creates $n$ singleton sets.
   * @param size Number of elements.
   * @section complexity
   * Time and auxiliary space: $O(n)$.
   */
  explicit disjoint_set(std::size_t size) : parent_(size), size_(size, 1) {
    for (std::size_t i = 0; i < size; ++i) {
      parent_[i] = i;
    }
  }

  /**
   * @brief Finds an element's representative and compresses its path.
   * @param value Element index.
   * @return Representative index.
   * @section complexity
   * Amortized time: $O(\alpha(n))$, where $n$ is the set count; auxiliary
   * space: $O(1)$.
   */
  [[nodiscard]] std::size_t find(std::size_t value) {
    std::size_t root = value;
    while (root != parent_[root]) {
      root = parent_[root];
    }
    while (value != root) {
      const std::size_t next = parent_[value];
      parent_[value] = root;
      value = next;
    }
    return root;
  }

  /**
   * @brief Joins two distinct sets when possible.
   * @param lhs First element index.
   * @param rhs Second element index.
   * @return True if two sets were joined.
   * @section complexity
   * Amortized time: $O(\alpha(n))$, where $n$ is the set count; auxiliary
   * space: $O(1)$.
   */
  [[nodiscard]] bool unite(std::size_t lhs, std::size_t rhs) {
    lhs = find(lhs);
    rhs = find(rhs);
    if (lhs == rhs) {
      return false;
    }
    if (size_[lhs] < size_[rhs]) {
      std::swap(lhs, rhs);
    }
    parent_[rhs] = lhs;
    size_[lhs] += size_[rhs];
    return true;
  }

private:
  std::vector<std::size_t> parent_;
  std::vector<std::size_t> size_;
};

/**
 * @brief Stable least-significant-byte radix sort for signed 64-bit weights.
 * @param edges Edges to sort; the vector is consumed by value.
 * @return Edges in nondecreasing signed weight order.
 * @section complexity
 * Time: $O(m+256\cdot 8)$; auxiliary space: $O(m+256)$, where $m$ is the
 * number of edges. The eight passes correspond to the fixed 64-bit key width.
 */
[[nodiscard]] inline std::vector<kruskal_edge>
radix_sort_edges(std::vector<kruskal_edge> edges) {
  if (edges.size() < 2) {
    return edges;
  }

  constexpr std::uint64_t sign_bit = std::uint64_t{1} << 63;
  std::vector<kruskal_edge> buffer(edges.size());
  for (unsigned shift = 0; shift < 64; shift += 8) {
    std::array<std::size_t, 256> offsets{};
    for (const kruskal_edge &edge : edges) {
      const auto key = static_cast<std::uint64_t>(edge.weight) ^ sign_bit;
      ++offsets[(key >> shift) & 0xFFU];
    }

    std::size_t prefix = 0;
    for (std::size_t &count : offsets) {
      const std::size_t bucket_size = count;
      count = prefix;
      prefix += bucket_size;
    }
    for (const kruskal_edge &edge : edges) {
      const auto key = static_cast<std::uint64_t>(edge.weight) ^ sign_bit;
      buffer[offsets[(key >> shift) & 0xFFU]++] = edge;
    }
    edges.swap(buffer);
  }
  return edges;
}

/**
 * @brief Adds two signed weights, reporting an unrepresentable sum.
 * @param sum Accumulated signed weight.
 * @param weight Next signed weight.
 * @return Exact sum when it fits in `int64_t`.
 * @throws std::overflow_error if the sum is outside the `int64_t` range.
 * @section complexity
 * Time and auxiliary space: $O(1)$.
 */
[[nodiscard]] inline std::int64_t checked_weight_sum(std::int64_t sum,
                                                     std::int64_t weight) {
  constexpr auto min = std::numeric_limits<std::int64_t>::min();
  constexpr auto max = std::numeric_limits<std::int64_t>::max();
  if ((weight > 0 && sum > max - weight) ||
      (weight < 0 && sum < min - weight)) {
    throw std::overflow_error("MST total weight exceeds int64_t range.");
  }
  return sum + weight;
}

/**
 * @brief Computes the representable total using opposite-sign cancellation.
 * @param edges Selected forest edges.
 * @return Exact total weight.
 * @throws std::overflow_error if the exact total is outside the `int64_t` range.
 * @section complexity
 * Time and auxiliary space: $O(k)$, where $k$ is the selected edge count.
 */
[[nodiscard]] inline std::int64_t
checked_total_weight(const std::vector<kruskal_edge> &edges) {
  std::vector<std::int64_t> positive;
  std::vector<std::int64_t> negative;
  positive.reserve(edges.size());
  negative.reserve(edges.size());
  for (const kruskal_edge &edge : edges) {
    (edge.weight < 0 ? negative : positive).push_back(edge.weight);
  }

  // Pair opposite signs first. This avoids a false overflow when a sorted
  // prefix is out of range even though the exact final sum is representable.
  std::size_t positive_index = 0;
  std::size_t negative_index = 0;
  std::int64_t total = 0;
  while (positive_index < positive.size() || negative_index < negative.size()) {
    if (total > 0 && negative_index < negative.size()) {
      total = checked_weight_sum(total, negative[negative_index++]);
    } else if (total < 0 && positive_index < positive.size()) {
      total = checked_weight_sum(total, positive[positive_index++]);
    } else if (positive_index < positive.size()) {
      total = checked_weight_sum(total, positive[positive_index++]);
    } else {
      total = checked_weight_sum(total, negative[negative_index++]);
    }
  }
  return total;
}

} // namespace detail

/**
 * @brief Finds a minimum spanning forest using Kruskal's algorithm.
 *
 * The input is an undirected multigraph. Parallel edges and self-loops are
 * accepted; self-loops are discarded naturally by the disjoint-set check.
 * Signed integer edge weights are supported. A disconnected graph returns a
 * minimum spanning forest with `is_connected == false`.
 *
 * @param vertex_count Number of vertices $n$, indexed in $[0,n)$.
 * @param edges Undirected edge list containing $m$ edges.
 * @return A minimum spanning forest and its total weight.
 * @throws std::out_of_range if an endpoint is outside $[0,n)$.
 * @throws std::overflow_error if the forest weight cannot fit in `int64_t`.
 * @section complexity
 * Time: $O(n+m+256\cdot 8+m\alpha(n))$ amortized; auxiliary space:
 * $O(n+m)$. Here $n$ is the vertex count, $m$ is the edge count, and
 * $\alpha$ is the inverse Ackermann function from disjoint-set operations.
 */
[[nodiscard]] inline kruskal_result
kruskal_mst(std::size_t vertex_count, const std::vector<kruskal_edge> &edges) {
  for (const kruskal_edge &edge : edges) {
    if (edge.u >= vertex_count || edge.v >= vertex_count) {
      throw std::out_of_range("Kruskal edge endpoint is outside the graph.");
    }
  }

  kruskal_result result;
  if (vertex_count == 0) {
    result.is_connected = true;
    return result;
  }
  if (vertex_count == 1) {
    result.is_connected = true;
    return result;
  }

  auto sorted_edges = detail::radix_sort_edges(edges);
  detail::disjoint_set components(vertex_count);
  result.tree_edges.reserve(vertex_count - 1);
  for (const kruskal_edge &edge : sorted_edges) {
    if (!components.unite(edge.u, edge.v)) {
      continue;
    }
    result.tree_edges.push_back(edge);
    if (result.tree_edges.size() == vertex_count - 1) {
      break;
    }
  }
  result.total_weight = detail::checked_total_weight(result.tree_edges);
  result.is_connected = result.tree_edges.size() == vertex_count - 1;
  return result;
}

} // namespace dsa
