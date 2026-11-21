// File : C-13.15.cpp

/*
    C-13.15 Give an example of a weighted directed graph G with negative-weight
    edges but no negative-weight cycle, such that Dijkstra's algorithm incor-
    rectly computes the shortest-path distances from some start vertex v.
*/

#include "negative_weight_sssp.hpp"

#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

namespace {

/**
 * @brief Verifies the definitive 4-vertex counterexample.
 *
 * Graph: s->A(1), A->B(4), s->C(2), B->C(-6).
 * Expected:
 *   - Bellman-Ford: d = [0, 1, 5, -1]  (correct).
 *   - Dijkstra:     d = [0, 1, 5,  2]  (WRONG for C — settled too early).
 *
 * @section complexity Time: $O(n \cdot m)$ dominated by Bellman-Ford call.
 */
void test_definitive_counterexample() {
  using namespace dsa;

  const signed_graph g = make_c1315_graph();

  // --- Oracle: Bellman-Ford ---
  const sssp_result bf = bellman_ford(g, 0);
  assert(!bf.negative_cycle && "Graph must not contain a negative cycle.");
  assert(bf.dist[0].has_value() && *bf.dist[0] == 0);
  assert(bf.dist[1].has_value() && *bf.dist[1] == 1);
  assert(bf.dist[2].has_value() && *bf.dist[2] == 5);
  assert(bf.dist[3].has_value() && *bf.dist[3] == -1); // true d[C]

  // --- Under test: Dijkstra ---
  const sssp_result dj = dijkstra(g, 0);
  assert(dj.dist[0].has_value() && *dj.dist[0] == 0);
  assert(dj.dist[1].has_value() && *dj.dist[1] == 1);
  assert(dj.dist[2].has_value() && *dj.dist[2] == 5);

  // Core assertion: Dijkstra settles C with d[C]=2 before B is extracted.
  // After B is extracted (d[B]=5) the relaxation 5+(-6)=-1 would improve
  // d[C], but C is already settled and is NEVER revisited.
  assert(dj.dist[3].has_value() && *dj.dist[3] == 2 &&
         "Dijkstra must report d[C]=2 (the known incorrect distance).");

  std::cout << "[PASS] test_definitive_counterexample\n";
  std::cout << "  Bellman-Ford d[C] = " << *bf.dist[3] << "  (correct)\n";
  std::cout << "  Dijkstra     d[C] = " << *dj.dist[3]
            << "  (WRONG — greedy settled C before the negative edge)\n";
}

/**
 * @brief Verifies Bellman-Ford correctly detects a negative-weight cycle.
 *
 * Graph: s->A(1), A->B(-1), B->A(-1) — cycle A<->B with total weight -2.
 * @section complexity Time: $O(n \cdot m)$.
 */
void test_negative_cycle_detection() {
  using namespace dsa;
  signed_graph g(3);
  g[0].push_back({1, 1});
  g[1].push_back({2, -1});
  g[2].push_back({1, -1}); // back-edge closes the negative cycle

  const sssp_result bf = bellman_ford(g, 0);
  assert(bf.negative_cycle &&
         "Bellman-Ford must detect the negative-weight cycle.");

  std::cout << "[PASS] test_negative_cycle_detection\n";
}

/**
 * @brief Dijkstra is correct when the negative edge is not on any shortest
 *        path from the source.
 *
 * Graph: s->A(5), s->B(3), B->X(-1).
 * The negative edge B->X does not influence any path starting from s to A,
 * so both algorithms agree.
 * @section complexity Time: $O(n \cdot m)$.
 */
void test_negative_edge_not_on_shortest_path() {
  using namespace dsa;
  signed_graph g(4); // s=0, A=1, B=2, X=3
  g[0].push_back({1, 5});
  g[0].push_back({2, 3});
  g[2].push_back({3, -1});

  const sssp_result bf = bellman_ford(g, 0);
  const sssp_result dj = dijkstra(g, 0);

  assert(!bf.negative_cycle);
  assert(*bf.dist[0] == 0 && *dj.dist[0] == 0);
  assert(*bf.dist[1] == 5 && *dj.dist[1] == 5);
  assert(*bf.dist[2] == 3 && *dj.dist[2] == 3);
  assert(*bf.dist[3] == 2 && *dj.dist[3] == 2);

  std::cout << "[PASS] test_negative_edge_not_on_shortest_path\n";
}

/**
 * @brief Single-vertex graph: both algorithms return d[s]=0 with no cycle.
 * @section complexity Time: $O(1)$.
 */
void test_single_vertex() {
  using namespace dsa;
  signed_graph g(1);
  const sssp_result bf = bellman_ford(g, 0);
  const sssp_result dj = dijkstra(g, 0);
  assert(!bf.negative_cycle);
  assert(*bf.dist[0] == 0 && *dj.dist[0] == 0);
  std::cout << "[PASS] test_single_vertex\n";
}

/**
 * @brief Unreachable vertices are reported as std::nullopt by both algorithms.
 * @section complexity Time: $O(n \cdot m)$.
 */
void test_unreachable_vertex() {
  using namespace dsa;
  signed_graph g(3); // s=0, A=1, isolated=2
  g[0].push_back({1, 7});

  const sssp_result bf = bellman_ford(g, 0);
  const sssp_result dj = dijkstra(g, 0);

  assert(!bf.negative_cycle);
  assert(*bf.dist[0] == 0 && *dj.dist[0] == 0);
  assert(*bf.dist[1] == 7 && *dj.dist[1] == 7);
  assert(!bf.dist[2].has_value() && "vertex 2 unreachable (BF)");
  assert(!dj.dist[2].has_value() && "vertex 2 unreachable (Dijkstra)");
  std::cout << "[PASS] test_unreachable_vertex\n";
}

/**
 * @brief Invalid source index throws std::out_of_range from both algorithms.
 * @section complexity Time: $O(1)$.
 */
void test_invalid_source() {
  using namespace dsa;
  signed_graph g(2);
  bool caught_bf = false;
  bool caught_dj = false;
  try {
    [[maybe_unused]] auto r = bellman_ford(g, 5);
  } catch (const std::out_of_range &) {
    caught_bf = true;
  }
  try {
    [[maybe_unused]] auto r = dijkstra(g, 5);
  } catch (const std::out_of_range &) {
    caught_dj = true;
  }
  assert(caught_bf && "bellman_ford must throw on invalid source.");
  assert(caught_dj && "dijkstra must throw on invalid source.");
  std::cout << "[PASS] test_invalid_source\n";
}

/**
 * @brief Stress test: parameterised family G_k for k = 2 .. 20.
 *
 * For each k, verifies:
 *   - Bellman-Ford gives d[C] = -1 (correct), no negative cycle.
 *   - Dijkstra   gives d[C] = k-1 (provably wrong), confirming the failure
 *     at every scale.
 *
 * @section complexity Per iteration: $O(k^2)$ (Bellman-Ford on k-edge path).
 *   Total: $O(K^3)$ for K = 20.
 */
void test_parameterised_family() {
  using namespace dsa;
  for (std::size_t k = 2; k <= 20; ++k) {
    const signed_graph g = make_c1315_family(k);
    const std::size_t c = k + 1; // index of vertex C

    const sssp_result bf = bellman_ford(g, 0);
    const sssp_result dj = dijkstra(g, 0);

    assert(!bf.negative_cycle);
    assert(bf.dist[c].has_value() && *bf.dist[c] == -1);

    const int expected_dj_wrong = static_cast<int>(k) - 1;
    assert(dj.dist[c].has_value() && *dj.dist[c] == expected_dj_wrong &&
           "Dijkstra must report the known wrong distance for C.");
  }
  std::cout << "[PASS] test_parameterised_family (k = 2 .. 20)\n";
}

/**
 * @brief Performance sanity: Bellman-Ford on a dense DAG (n=300, m=O(n^2))
 *        finishes in a few seconds and returns correct distances.
 *
 * Confirms the $O(n \cdot m) = O(n^3)$ complexity claim for dense graphs.
 * @section complexity $O(n^3)$ where $n = 300$.
 */
void test_performance_sanity() {
  using namespace dsa;
  constexpr std::size_t n = 300;
  signed_graph g(n);
  // Forward DAG: edge (u, v) for all u < v, weight = v - u.
  // Shortest path s -> v is the direct edge of weight v.
  for (std::size_t u = 0; u < n; ++u) {
    for (std::size_t v = u + 1; v < n; ++v) {
      g[u].push_back({v, static_cast<int>(v - u)});
    }
  }

  const sssp_result bf = bellman_ford(g, 0);
  assert(!bf.negative_cycle);
  for (std::size_t v = 0; v < n; ++v) {
    assert(bf.dist[v].has_value() && *bf.dist[v] == static_cast<int>(v));
  }
  std::cout << "[PASS] test_performance_sanity (n = " << n << ")\n";
}

} // namespace

// ============================================================
//  main
// ============================================================

int main() {
  std::cout << "=== C-13.15: Dijkstra fails on negative-weight edges ===\n\n";

  test_single_vertex();
  test_unreachable_vertex();
  test_invalid_source();
  test_negative_cycle_detection();
  test_negative_edge_not_on_shortest_path();
  test_definitive_counterexample();
  test_parameterised_family();
  test_performance_sanity();

  std::cout << "\nAll tests passed.\n";

  // ---- Human-readable counterexample summary ----
  std::cout << "\n=== Counterexample Summary ===\n";
  std::cout << "Graph G (4 vertices, 4 directed edges):\n";
  std::cout << "  s=0 --1--> A=1\n";
  std::cout << "  A=1 --4--> B=2\n";
  std::cout << "  s=0 --2--> C=3   <-- cheap shortcut: Dijkstra settles C "
               "too early\n";
  std::cout << "  B=2 --(-6)--> C=3 <-- negative edge: B not yet settled "
               "when C is!\n";
  std::cout << "\nShortest-path distances from s=0:\n";

  using namespace dsa;
  const signed_graph g = make_c1315_graph();
  const sssp_result bf = bellman_ford(g, 0);
  const sssp_result dj = dijkstra(g, 0);

  const std::vector<std::string> names = {"s", "A", "B", "C"};
  std::cout << "  Vertex | True dist (BF) | Dijkstra dist | Correct?\n";
  std::cout << "  -------+----------------+---------------+---------\n";
  for (std::size_t v = 0; v < g.size(); ++v) {
    const std::string bf_s =
        bf.dist[v].has_value() ? std::to_string(*bf.dist[v]) : "inf";
    const std::string dj_s =
        dj.dist[v].has_value() ? std::to_string(*dj.dist[v]) : "inf";
    const bool correct =
        (bf.dist[v].has_value() == dj.dist[v].has_value()) &&
        (!bf.dist[v].has_value() || *bf.dist[v] == *dj.dist[v]);
    std::cout << "    " << names[v] << "    |      " << bf_s
              << "          |     " << dj_s << "         |  "
              << (correct ? "YES" : "NO  <-- WRONG") << "\n";
  }

  return 0;
}
