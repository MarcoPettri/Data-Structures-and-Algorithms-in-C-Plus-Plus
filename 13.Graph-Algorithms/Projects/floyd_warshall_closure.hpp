/**
 * @file floyd_warshall_closure.hpp
 * @brief Floyd-Warshall transitive closure for directed graphs.
 */
#pragma once

#include <cstddef>
#include <stdexcept>
#include <vector>

namespace dsa {

/**
 * @brief Computes reflexive transitive closure from an adjacency matrix.
 *
 * The returned matrix contains `true` at $(i,j)$ exactly when a directed path
 * from vertex $i$ to vertex $j$ exists. Paths of length zero are included, so
 * every diagonal entry in the result is true. The input is not modified.
 *
 * @param adjacency Square adjacency matrix for a directed graph with $n$
 *        vertices. A true entry denotes a directed edge.
 * @return Reflexive reachability matrix of size $n \times n$.
 * @throws std::invalid_argument if the matrix is not square.
 * @section complexity
 * Time: $O(n^3)$ worst-case; auxiliary space: $O(n^2)$ for the result.
 * Here $n$ is the number of vertices.
 */
[[nodiscard]] inline std::vector<std::vector<bool>>
floyd_warshall_closure(const std::vector<std::vector<bool>> &adjacency) {
  const std::size_t n = adjacency.size();
  for (const auto &row : adjacency) {
    if (row.size() != n) {
      throw std::invalid_argument(
          "Floyd-Warshall adjacency matrix must be square.");
    }
  }

  std::vector<std::vector<bool>> reachable = adjacency;
  for (std::size_t vertex = 0; vertex < n; ++vertex) {
    reachable[vertex][vertex] = true;
  }

  // After iteration k, reachable[i][j] includes paths whose intermediate
  // vertices are drawn only from [0, k].
  for (std::size_t k = 0; k < n; ++k) {
    for (std::size_t i = 0; i < n; ++i) {
      if (!reachable[i][k]) {
        continue;
      }
      for (std::size_t j = 0; j < n; ++j) {
        reachable[i][j] = reachable[i][j] || reachable[k][j];
      }
    }
  }
  return reachable;
}

} // namespace dsa
