/**
 * @file prim_integer.hpp
 * @brief Prim-Jarník minimum spanning forest for integer edge weights.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <queue>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace dsa {

/**
 * @brief Undirected edge with an integer weight.
 * @section complexity
 * Storage: $O(1)$ per edge.
 */
struct prim_edge {
  std::size_t u{};       ///< First endpoint.
  std::size_t v{};       ///< Second endpoint.
  std::int64_t weight{}; ///< Signed edge weight.
};

/**
 * @brief Result of Prim-Jarník's minimum spanning forest computation.
 * @section complexity
 * Storage: $O(n)$, where $n$ is the vertex count.
 */
struct prim_result {
  bool is_connected{false};              ///< Whether the graph is connected.
  std::int64_t total_weight{0};           ///< Sum of selected edge weights.
  std::vector<prim_edge> tree_edges{};   ///< Forest edges, at most $n-1$.
};

namespace detail {

/** @brief One directed adjacency entry retaining its undirected edge weight. */
struct prim_adjacency_entry {
  std::size_t to{};
  std::int64_t weight{};
};

/**
 * @brief Adds signed weights with overflow detection.
 * @param sum Current total.
 * @param weight Next weight.
 * @return Exact sum when representable.
 * @throws std::overflow_error if the exact sum does not fit in `int64_t`.
 * @section complexity
 * Time and auxiliary space: $O(1)$.
 */
[[nodiscard]] inline std::int64_t prim_checked_add(std::int64_t sum,
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
 * @brief Sums selected weights with opposite-sign cancellation.
 * @param edges Selected forest edges.
 * @return Exact total when representable.
 * @throws std::overflow_error if the exact total is outside the `int64_t` range.
 * @section complexity
 * Time and auxiliary space: $O(k)$, where $k$ is the selected edge count.
 */
[[nodiscard]] inline std::int64_t
prim_total_weight(const std::vector<prim_edge> &edges) {
  std::vector<std::int64_t> positive;
  std::vector<std::int64_t> negative;
  positive.reserve(edges.size());
  negative.reserve(edges.size());
  for (const prim_edge &edge : edges) {
    (edge.weight < 0 ? negative : positive).push_back(edge.weight);
  }

  std::size_t positive_index = 0;
  std::size_t negative_index = 0;
  std::int64_t total = 0;
  while (positive_index < positive.size() || negative_index < negative.size()) {
    if (total > 0 && negative_index < negative.size()) {
      total = prim_checked_add(total, negative[negative_index++]);
    } else if (total < 0 && positive_index < positive.size()) {
      total = prim_checked_add(total, positive[positive_index++]);
    } else if (positive_index < positive.size()) {
      total = prim_checked_add(total, positive[positive_index++]);
    } else {
      total = prim_checked_add(total, negative[negative_index++]);
    }
  }
  return total;
}

} // namespace detail

/**
 * @brief Computes a minimum spanning forest using Prim-Jarník's algorithm.
 *
 * The input is an undirected multigraph with vertices indexed in $[0,n)$.
 * Negative integer weights are allowed. Disconnected input produces a
 * minimum spanning forest and sets `is_connected` to false. Empty and
 * singleton graphs are considered connected by convention.
 *
 * @param vertex_count Number of vertices $n$.
 * @param edges Undirected edge list with $m$ entries.
 * @return A minimum spanning forest and its total weight.
 * @throws std::out_of_range if an edge endpoint is outside $[0,n)$.
 * @throws std::overflow_error if the exact forest weight does not fit in
 *         `int64_t`.
 * @section complexity
 * Time: $O(n+(m+1)\log(1+m))$ worst case using lazy priority-queue entries;
 * auxiliary space: $O(n+m)$. Here $n$ is the vertex count and $m$ is the
 * edge count.
 */
[[nodiscard]] inline prim_result
prim_jarnik_mst(std::size_t vertex_count, const std::vector<prim_edge> &edges) {
  std::vector<std::vector<detail::prim_adjacency_entry>> adjacency(
      vertex_count);
  for (const prim_edge &edge : edges) {
    if (edge.u >= vertex_count || edge.v >= vertex_count) {
      throw std::out_of_range(
          "Prim-Jarnik edge endpoint is outside the graph.");
    }
    adjacency[edge.u].push_back({edge.v, edge.weight});
    adjacency[edge.v].push_back({edge.u, edge.weight});
  }

  prim_result result;
  if (vertex_count == 0) {
    result.is_connected = true;
    return result;
  }

  using frontier_entry = std::tuple<std::int64_t, std::size_t, std::size_t>;
  std::priority_queue<frontier_entry, std::vector<frontier_entry>,
                      std::greater<frontier_entry>>
      frontier;
  std::vector<bool> visited(vertex_count, false);
  std::size_t components = 0;

  for (std::size_t root = 0; root < vertex_count; ++root) {
    if (visited[root]) {
      continue;
    }
    ++components;
    visited[root] = true;
    for (const auto &edge : adjacency[root]) {
      if (!visited[edge.to]) {
        frontier.emplace(edge.weight, root, edge.to);
      }
    }

    while (!frontier.empty()) {
      const auto [weight, from, to] = frontier.top();
      frontier.pop();
      if (visited[to]) {
        continue;
      }
      visited[to] = true;
      result.tree_edges.push_back({from, to, weight});
      for (const auto &edge : adjacency[to]) {
        if (!visited[edge.to]) {
          frontier.emplace(edge.weight, to, edge.to);
        }
      }
    }
  }

  result.total_weight = detail::prim_total_weight(result.tree_edges);
  result.is_connected = components <= 1;
  return result;
}

} // namespace dsa
