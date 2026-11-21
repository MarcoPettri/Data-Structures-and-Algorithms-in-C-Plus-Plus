/**
 * @file dijkstra_integer.hpp
 * @brief Dijkstra's shortest-path algorithm for nonnegative integer weights.
 */
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

namespace dsa {

/**
 * @brief Directed weighted edge with an integer weight.
 * @section complexity
 * Storage: $O(1)$ per edge.
 */
struct weighted_edge {
  std::size_t to{};      ///< Destination vertex index.
  std::int64_t weight{}; ///< Nonnegative edge weight.
};

/** @brief Adjacency-list representation of a directed weighted graph. */
using weighted_graph = std::vector<std::vector<weighted_edge>>;

/**
 * @brief Shortest-path distances and predecessor links from one source.
 *
 * A missing distance means that the vertex is unreachable or that every path
 * to it has a length larger than the representable range of `uint64_t`.
 *
 * @section complexity
 * Storage: $O(n)$, where $n$ is the number of vertices.
 */
struct dijkstra_result {
  std::vector<std::optional<std::uint64_t>> distance; ///< Distance per vertex.
  std::vector<std::optional<std::size_t>> predecessor; ///< Previous vertex.

  /**
   * @brief Reconstructs a shortest path from the source to `target`.
   * @param target Vertex index to reach.
   * @return Vertex sequence including source and target, or empty if no
   *         representable path exists.
   * @throws std::out_of_range if `target` is outside the graph.
   * @throws std::logic_error if predecessor data is inconsistent.
   * @section complexity
   * Time and auxiliary space: $O(n)$, where $n$ is the number of vertices.
   */
  [[nodiscard]] std::vector<std::size_t> path_to(std::size_t target) const {
    if (predecessor.size() != distance.size()) {
      throw std::logic_error(
          "Dijkstra result vectors have inconsistent sizes.");
    }
    if (target >= distance.size()) {
      throw std::out_of_range("Dijkstra target is outside the graph.");
    }
    if (!distance[target]) {
      return {};
    }

    std::vector<std::size_t> path;
    path.reserve(distance.size());
    std::size_t current = target;
    for (std::size_t steps = 0; steps < distance.size(); ++steps) {
      if (current >= predecessor.size()) {
        throw std::logic_error("Dijkstra predecessor is outside the graph.");
      }
      path.push_back(current);
      if (!predecessor[current]) {
        std::reverse(path.begin(), path.end());
        return path;
      }
      current = *predecessor[current];
    }
    throw std::logic_error("Dijkstra predecessor chain contains a cycle.");
  }
};

/**
 * @brief Computes single-source shortest paths using Dijkstra's algorithm.
 *
 * The graph is directed; represent an undirected edge with two adjacency
 * entries. Vertices are indexed from zero. Edge weights must be nonnegative.
 * Distances that overflow `uint64_t` are omitted.
 *
 * @param graph Adjacency list for a graph with $n$ vertices and $m$ edges.
 * @param source Source vertex, with $0 \le source < n$.
 * @return Distances and predecessor links for all vertices.
 * @throws std::out_of_range if the source or an edge destination is invalid.
 * @throws std::invalid_argument if an edge has a negative weight.
 * @section complexity
 * Time: $O(n+(m+1)\log(1+m))$ worst case with lazy heap entries; auxiliary
 * space: $O(n+m)$. Here $n$ is the vertex count and $m$ is the edge count.
 */
[[nodiscard]] inline dijkstra_result dijkstra(const weighted_graph &graph,
                                              std::size_t source) {
  const std::size_t n = graph.size();
  if (source >= n) {
    throw std::out_of_range("Dijkstra source is outside the graph.");
  }

  for (const auto &edges : graph) {
    for (const weighted_edge &edge : edges) {
      if (edge.to >= n) {
        throw std::out_of_range("Edge destination is outside the graph.");
      }
      if (edge.weight < 0) {
        throw std::invalid_argument("Dijkstra requires nonnegative weights.");
      }
    }
  }

  dijkstra_result result{std::vector<std::optional<std::uint64_t>>(n),
                         std::vector<std::optional<std::size_t>>(n)};
  using queue_entry = std::pair<std::uint64_t, std::size_t>;
  std::priority_queue<queue_entry, std::vector<queue_entry>,
                      std::greater<queue_entry>>
      frontier;
  result.distance[source] = 0;
  frontier.emplace(0, source);

  while (!frontier.empty()) {
    const auto [current_distance, from] = frontier.top();
    frontier.pop();
    if (!result.distance[from] || current_distance != *result.distance[from]) {
      continue;
    }

    for (const weighted_edge &edge : graph[from]) {
      const auto weight = static_cast<std::uint64_t>(edge.weight);
      if (weight >
          std::numeric_limits<std::uint64_t>::max() - current_distance) {
        continue;
      }
      const std::uint64_t candidate = current_distance + weight;
      if (result.distance[edge.to] && candidate >= *result.distance[edge.to]) {
        continue;
      }
      result.distance[edge.to] = candidate;
      result.predecessor[edge.to] = from;
      frontier.emplace(candidate, edge.to);
    }
  }
  return result;
}

} // namespace dsa
