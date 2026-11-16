/**
 * @file dijkstra_heap.hpp
 * @brief Indexed binary heap and Dijkstra example graph for C-13.14.
 */

#ifndef DIJKSTRA_HEAP_HPP
#define DIJKSTRA_HEAP_HPP

#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace dsa {

/**
 * @brief Weighted adjacency-list edge.
 * @section complexity
 * Storage: $O(1)$.
 */
struct weighted_edge {
  std::size_t to{};     ///< Destination vertex.
  std::size_t weight{}; ///< Nonnegative edge weight.
};

/**
 * @class indexed_min_heap
 * @brief Binary min-heap of vertex IDs keyed by an external distance array.
 *
 * @section complexity
 * Insert, extract-min, and decrease-key: $O(\log n)$ worst-case; storage:
 * $O(n)$.
 */
class indexed_min_heap {
public:
  /**
   * @brief Constructs an empty heap sized for the supplied key array.
   * @param keys External keys; the array must outlive this heap.
   * @return No value.
   * @section complexity
   * Time and auxiliary space: $O(n)$.
   */
  explicit indexed_min_heap(const std::vector<std::size_t> &keys)
      : keys_(keys), position_(keys.size(), absent) {}

  /**
   * @brief Checks whether the heap is empty.
   * @return True if it contains no vertices.
   * @section complexity
   * Time and auxiliary space: $O(1)$.
   */
  [[nodiscard]] bool empty() const noexcept { return heap_.empty(); }

  /**
   * @brief Checks whether a vertex is currently in the heap.
   * @param vertex Vertex identifier in the range of the key array.
   * @return True if the vertex is queued.
   * @section complexity
   * Time and auxiliary space: $O(1)$.
   */
  [[nodiscard]] bool contains(std::size_t vertex) const noexcept {
    return position_[vertex] != absent;
  }

  /**
   * @brief Inserts a vertex using its current key.
   * @param vertex Vertex identifier not already in the heap.
   * @return No value.
   * @section complexity
   * Time: $O(\log n)$ worst-case; auxiliary space: $O(1)$ amortized.
   */
  void insert(std::size_t vertex) {
    position_[vertex] = heap_.size();
    heap_.push_back(vertex);
    sift_up(heap_.size() - 1);
  }

  /**
   * @brief Removes and returns the vertex with the smallest key.
   * @return The minimum-key vertex; the heap must be nonempty.
   * @section complexity
   * Time: $O(\log n)$ worst-case; auxiliary space: $O(1)$.
   */
  std::size_t extract_min() {
    const std::size_t result = heap_.front();
    swap_positions(0, heap_.size() - 1);
    heap_.pop_back();
    position_[result] = absent;
    if (!heap_.empty()) {
      sift_down(0);
    }
    return result;
  }

  /**
   * @brief Restores heap order after a queued vertex's key decreases.
   * @param vertex Vertex whose key was decreased and is still queued.
   * @return No value.
   * @section complexity
   * Time: $O(\log n)$ worst-case; auxiliary space: $O(1)$.
   */
  void decrease_key(std::size_t vertex) { sift_up(position_[vertex]); }

private:
  static constexpr std::size_t absent =
      std::numeric_limits<std::size_t>::max();
  const std::vector<std::size_t> &keys_;
  std::vector<std::size_t> heap_;
  std::vector<std::size_t> position_;

  [[nodiscard]] bool less(std::size_t lhs, std::size_t rhs) const noexcept {
    return keys_[heap_[lhs]] < keys_[heap_[rhs]];
  }

  void swap_positions(std::size_t lhs, std::size_t rhs) noexcept {
    std::swap(heap_[lhs], heap_[rhs]);
    position_[heap_[lhs]] = lhs;
    position_[heap_[rhs]] = rhs;
  }

  void sift_up(std::size_t index) {
    while (index > 0) {
      const std::size_t parent = (index - 1) / 2;
      if (!less(index, parent)) {
        break;
      }
      swap_positions(index, parent);
      index = parent;
    }
  }

  void sift_down(std::size_t index) {
    while (true) {
      const std::size_t left = 2 * index + 1;
      const std::size_t right = left + 1;
      std::size_t smallest = index;
      if (left < heap_.size() && less(left, smallest)) {
        smallest = left;
      }
      if (right < heap_.size() && less(right, smallest)) {
        smallest = right;
      }
      if (smallest == index) {
        break;
      }
      swap_positions(index, smallest);
      index = smallest;
    }
  }
};

/**
 * @brief Dijkstra distances and the number of successful decrease-key calls.
 * @section complexity
 * Storage: $O(n)$.
 */
struct dijkstra_result {
  std::vector<std::size_t> distance; ///< Distance from the source per vertex.
  std::size_t decrease_key_count{};  ///< Successful queued-key decreases.
};

/**
 * @brief Runs Dijkstra's algorithm with an indexed binary min-heap.
 * @param graph Undirected adjacency list with nonnegative weights.
 * @param source Valid source vertex.
 * @return Shortest distances and successful decrease-key count.
 * @throws std::out_of_range if source or an edge destination is invalid.
 * @section complexity
 * Time: $O((n+m)\log n)$ worst-case; auxiliary space: $O(n)$.
 */
[[nodiscard]] inline dijkstra_result
dijkstra(const std::vector<std::vector<weighted_edge>> &graph,
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
    }
  }

  const std::size_t infinity = std::numeric_limits<std::size_t>::max();
  std::vector<std::size_t> distance(n, infinity);
  std::vector<bool> settled(n, false);
  indexed_min_heap heap(distance);
  std::size_t decrease_key_count = 0;

  distance[source] = 0;
  heap.insert(source);
  while (!heap.empty()) {
    const std::size_t u = heap.extract_min();
    settled[u] = true;
    for (const weighted_edge &edge : graph[u]) {
      if (settled[edge.to]) {
        continue;
      }
      if (edge.weight > infinity - distance[u]) {
        continue;
      }
      const std::size_t candidate = distance[u] + edge.weight;
      if (candidate >= distance[edge.to]) {
        continue;
      }
      const bool already_queued = heap.contains(edge.to);
      distance[edge.to] = candidate;
      if (already_queued) {
        heap.decrease_key(edge.to);
        ++decrease_key_count;
      } else {
        heap.insert(edge.to);
      }
    }
  }
  return {std::move(distance), decrease_key_count};
}

/**
 * @brief Builds the weighted complete graph used by C-13.14.
 * @param n Number of vertices.
 * @return Undirected graph with $n(n-1)/2$ edges.
 * @section complexity
 * Time: $O(n^2)$; auxiliary space: $O(n^2)$.
 */
[[nodiscard]] inline std::vector<std::vector<weighted_edge>>
make_c1314_example_graph(std::size_t n) {
  std::vector<std::vector<weighted_edge>> graph(n);
  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t j = i + 1; j < n; ++j) {
      const std::size_t weight = (j == i + 1) ? 1 : 3 * n - 2 * i;
      graph[i].push_back({j, weight});
      graph[j].push_back({i, weight});
    }
  }
  return graph;
}

} // namespace dsa

#endif // DIJKSTRA_HEAP_HPP
