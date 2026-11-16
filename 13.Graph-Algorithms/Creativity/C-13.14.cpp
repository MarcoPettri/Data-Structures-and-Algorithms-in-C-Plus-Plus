// File : C-13.14.cpp

/*
    C-13.14 Give an example of an n-vertex simple graph G that causes Dijkstra’s
    algorithm to run in Ω(n^2 log n) time when it is implemented with a heap.
*/

#include "dijkstra_heap.hpp"

#include <cassert>
#include <cstddef>
#include <iostream>

namespace {

/**
 * @brief Checks the graph family and its Dijkstra operation count.
 * @return No value; assertions report any construction or algorithm failure.
 * @section complexity
 * The test runs for $n=2,\ldots,40$, using $O(n^2)$ work per size.
 */
void test_construction() {
  for (std::size_t n = 2; n <= 40; ++n) {
    const auto graph = dsa::make_c1314_example_graph(n);
    std::size_t directed_edge_count = 0;
    for (const auto &edges : graph) {
      directed_edge_count += edges.size();
    }
    assert(directed_edge_count == n * (n - 1));

    const dsa::dijkstra_result result = dsa::dijkstra(graph, 0);
    for (std::size_t vertex = 0; vertex < n; ++vertex) {
      assert(result.distance[vertex] == vertex);
    }
    const std::size_t expected_decreases = (n - 1) * (n - 2) / 2;
    assert(result.decrease_key_count == expected_decreases);
  }
}

} // namespace

int main() {
  test_construction();

  constexpr std::size_t n = 10;
  const auto graph = dsa::make_c1314_example_graph(n);
  const dsa::dijkstra_result result = dsa::dijkstra(graph, 0);
  std::cout << "n = " << n << ", edges = " << n * (n - 1) / 2
            << ", successful decrease-key operations = "
            << result.decrease_key_count << '\n';
}
