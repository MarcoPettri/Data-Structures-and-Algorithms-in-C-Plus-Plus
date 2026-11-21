// File : C-13.24.cpp

/*
    C-13.24 Let G be a weighted digraph with n vertices. Design a variation of Floyd-
    Warshall’s algorithm for computing the lengths of the shortest paths from
    each vertex to every other vertex in O(n^3) time.
*/

#include "floyd_warshall.hpp"

#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <random>
#include <vector>

namespace {

/**
 * @brief Verifies that a reconstructed path is structurally valid in the digraph
 *        and that the sum of its edge weights matches the reported shortest distance.
 */
template <typename Weight>
void verify_path_validity(const dsa::weighted_digraph<Weight> &g, std::size_t u,
                          std::size_t v, const dsa::apsp_result<Weight> &res) {
  if (u == v) {
    assert(res.dist[u][v].has_value() && *res.dist[u][v] == Weight{0});
    const auto path = res.get_path(u, v);
    assert(path.size() == 1 && path[0] == u);
    return;
  }

  if (!res.dist[u][v].has_value()) {
    assert(res.get_path(u, v).empty());
    return;
  }

  const auto path = res.get_path(u, v);
  assert(!path.empty());
  assert(path.front() == u);
  assert(path.back() == v);

  Weight sum_weight{0};
  for (std::size_t i = 0; i + 1 < path.size(); ++i) {
    const std::size_t from = path[i];
    const std::size_t to = path[i + 1];

    bool edge_found = false;
    Weight min_w = std::numeric_limits<Weight>::max();

    for (const auto &edge : g.outgoing(from)) {
      if (edge.to == to) {
        edge_found = true;
        min_w = std::min(min_w, edge.weight);
      }
    }

    assert(edge_found && "Path step uses non-existent edge!");
    sum_weight += min_w;
  }

  assert(std::abs(sum_weight - *res.dist[u][v]) < 1e-9 &&
         "Sum of path weights does not match reported distance!");
}

/**
 * @brief Tests canonical digraph with non-negative weights and multi-hop shortcuts.
 */
void test_canonical_digraph() {
  std::cout << "[RUN] test_canonical_digraph" << std::endl;

  // 4 vertices:
  // 0 -> 1 (5), 1 -> 2 (3), 2 -> 3 (1)
  // 0 -> 3 (10) [direct edge is longer than 5 + 3 + 1 = 9]
  // 0 -> 2 (15)
  dsa::weighted_digraph<int> g(4);
  g.add_edge(0, 1, 5);
  g.add_edge(1, 2, 3);
  g.add_edge(2, 3, 1);
  g.add_edge(0, 3, 10);
  g.add_edge(0, 2, 15);

  const auto res = dsa::floyd_warshall(g);
  const auto oracle = dsa::bellman_ford_all_pairs(g);

  assert(!res.has_negative_cycle);
  assert(res.dist[0][0] == 0);
  assert(res.dist[0][1] == 5);
  assert(res.dist[0][2] == 8);
  assert(res.dist[0][3] == 9); // Multi-hop shortcut chosen over 10

  // Unreachable backwards
  assert(!res.dist[3][0].has_value());
  assert(!res.dist[2][1].has_value());

  // Compare with oracle
  for (std::size_t i = 0; i < 4; ++i) {
    for (std::size_t j = 0; j < 4; ++j) {
      assert(res.dist[i][j] == oracle.dist[i][j]);
      verify_path_validity(g, i, j, res);
    }
  }

  std::cout << "[PASS] test_canonical_digraph" << std::endl;
}

/**
 * @brief Tests digraph with negative-weight edges but NO negative cycles.
 * (Classic case where Dijkstra fails, but Floyd-Warshall succeeds!)
 */
void test_negative_edges_no_cycle() {
  std::cout << "[RUN] test_negative_edges_no_cycle" << std::endl;

  // 4 vertices (C-13.15 counterexample graph):
  // 0 -> 1 (1), 1 -> 2 (4), 0 -> 3 (2), 2 -> 3 (-6)
  // Path 0 -> 1 -> 2 -> 3 has length 1 + 4 - 6 = -1, beating direct edge 0 -> 3 (2)
  dsa::weighted_digraph<int> g(4);
  g.add_edge(0, 1, 1);
  g.add_edge(1, 2, 4);
  g.add_edge(0, 3, 2);
  g.add_edge(2, 3, -6);

  const auto res = dsa::floyd_warshall(g);
  const auto oracle = dsa::bellman_ford_all_pairs(g);

  assert(!res.has_negative_cycle);
  assert(res.dist[0][3] == -1); // True shortest distance
  assert(res.dist[0][1] == 1);
  assert(res.dist[0][2] == 5);

  for (std::size_t i = 0; i < 4; ++i) {
    for (std::size_t j = 0; j < 4; ++j) {
      assert(res.dist[i][j] == oracle.dist[i][j]);
      verify_path_validity(g, i, j, res);
    }
  }

  std::cout << "[PASS] test_negative_edges_no_cycle" << std::endl;
}

/**
 * @brief Tests detection of negative-weight cycles and corrupted paths.
 */
void test_negative_weight_cycle() {
  std::cout << "[RUN] test_negative_weight_cycle" << std::endl;

  // Cycle 1 -> 2 -> 1 with weights 2, -5 (sum = -3 < 0)
  // Vertex 0 can reach 1. Vertex 3 is reached from 2.
  // Paths 0 -> 3 and 1 -> 2 can be made arbitrarily small (-infinity).
  dsa::weighted_digraph<int> g(4);
  g.add_edge(0, 1, 1);
  g.add_edge(1, 2, 2);
  g.add_edge(2, 1, -5); // Closes negative cycle
  g.add_edge(2, 3, 4);

  const auto res = dsa::floyd_warshall(g);
  assert(res.has_negative_cycle);

  // Distances affected by negative cycle must be marked
  assert(res.neg_cycle_path[1][2]);
  assert(res.neg_cycle_path[0][3]);
  assert(!res.dist[0][3].has_value()); // -infinity

  std::cout << "[PASS] test_negative_weight_cycle" << std::endl;
}

/**
 * @brief Tests edge cases: empty digraph, single vertex, disconnected digraphs.
 */
void test_boundary_cases() {
  std::cout << "[RUN] test_boundary_cases" << std::endl;

  // Empty digraph
  dsa::weighted_digraph<double> g0(0);
  const auto res0 = dsa::floyd_warshall(g0);
  assert(res0.dist.empty());

  // Single vertex without edges
  dsa::weighted_digraph<double> g1(1);
  const auto res1 = dsa::floyd_warshall(g1);
  assert(res1.dist.size() == 1);
  assert(res1.dist[0][0] == 0.0);
  verify_path_validity(g1, 0, 0, res1);

  // Single vertex with negative self-loop
  dsa::weighted_digraph<double> g1_neg(1);
  g1_neg.add_edge(0, 0, -2.0);
  const auto res1_neg = dsa::floyd_warshall(g1_neg);
  assert(res1_neg.has_negative_cycle);

  // Disconnected digraph
  dsa::weighted_digraph<int> g_disc(4);
  g_disc.add_edge(0, 1, 10);
  g_disc.add_edge(2, 3, 20);
  const auto res_disc = dsa::floyd_warshall(g_disc);
  assert(res_disc.dist[0][1] == 10);
  assert(res_disc.dist[2][3] == 20);
  assert(!res_disc.dist[0][2].has_value());
  assert(!res_disc.dist[1][3].has_value());

  std::cout << "[PASS] test_boundary_cases" << std::endl;
}

/**
 * @brief Tests parallel directed edges with different weights.
 */
void test_parallel_edges() {
  std::cout << "[RUN] test_parallel_edges" << std::endl;

  dsa::weighted_digraph<int> g(2);
  g.add_edge(0, 1, 100);
  g.add_edge(0, 1, 30); // Lower weight parallel edge
  g.add_edge(0, 1, 50);

  const auto res = dsa::floyd_warshall(g);
  assert(res.dist[0][1] == 30);
  verify_path_validity(g, 0, 1, res);

  std::cout << "[PASS] test_parallel_edges" << std::endl;
}

/**
 * @brief Tests invalid inputs and exception handling.
 */
void test_invalid_inputs_and_exceptions() {
  std::cout << "[RUN] test_invalid_inputs_and_exceptions" << std::endl;

  dsa::weighted_digraph<double> g(3);

  bool caught_add = false;
  try {
    g.add_edge(0, 5, 10.0);
  } catch (const std::out_of_range &) {
    caught_add = true;
  }
  assert(caught_add);

  bool caught_outgoing = false;
  try {
    [[maybe_unused]] const auto &edges = g.outgoing(10);
  } catch (const std::out_of_range &) {
    caught_outgoing = true;
  }
  assert(caught_outgoing);

  std::cout << "[PASS] test_invalid_inputs_and_exceptions" << std::endl;
}

/**
 * @brief Randomized stress test comparing Floyd-Warshall against Bellman-Ford All-Pairs.
 */
void test_randomized_stress() {
  std::cout << "[RUN] test_randomized_stress (50 random digraphs)" << std::endl;
  std::mt19937_64 rng(1324); // Fixed seed

  const std::size_t num_tests = 50;

  for (std::size_t t = 0; t < num_tests; ++t) {
    std::uniform_int_distribution<std::size_t> n_dist(2, 10);
    const std::size_t n = n_dist(rng);

    dsa::weighted_digraph<int> g(n);

    std::uniform_int_distribution<std::size_t> m_dist(n, n * 3);
    const std::size_t m = m_dist(rng);

    std::uniform_int_distribution<std::size_t> v_dist(0, n - 1);
    // Non-negative weights to ensure well-defined shortest paths
    std::uniform_int_distribution<int> w_dist(0, 50);

    for (std::size_t i = 0; i < m; ++i) {
      const std::size_t u = v_dist(rng);
      const std::size_t v = v_dist(rng);
      g.add_edge(u, v, w_dist(rng));
    }

    const auto res = dsa::floyd_warshall(g);
    const auto oracle = dsa::bellman_ford_all_pairs(g);

    assert(res.has_negative_cycle == oracle.has_negative_cycle);

    for (std::size_t i = 0; i < n; ++i) {
      for (std::size_t j = 0; j < n; ++j) {
        assert(res.dist[i][j] == oracle.dist[i][j]);
        if (res.dist[i][j].has_value()) {
          verify_path_validity(g, i, j, res);
        }
      }
    }
  }

  std::cout << "[PASS] test_randomized_stress (50 random digraphs verified)"
            << std::endl;
}

/**
 * @brief Performance sanity check confirming O(n^3) scaling on n = 300.
 */
void test_performance_sanity() {
  std::cout
      << "[RUN] test_performance_sanity (N = 300 vertices, M = 3,000 edges)"
      << std::endl;

  const std::size_t n = 300;
  const std::size_t m = 3000;

  dsa::weighted_digraph<double> g(n);
  std::mt19937_64 rng(42);
  std::uniform_int_distribution<std::size_t> v_dist(0, n - 1);
  std::uniform_real_distribution<double> w_dist(1.0, 100.0);

  // Guarantee reachability by a directed cycle
  for (std::size_t i = 0; i < n; ++i) {
    g.add_edge(i, (i + 1) % n, w_dist(rng));
  }

  for (std::size_t i = n; i < m; ++i) {
    g.add_edge(v_dist(rng), v_dist(rng), w_dist(rng));
  }

  const auto t0 = std::chrono::steady_clock::now();
  const auto res = dsa::floyd_warshall(g);
  const auto t1 = std::chrono::steady_clock::now();

  const auto elapsed_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

  assert(!res.has_negative_cycle);
  // Verify that all pairs are reachable
  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t j = 0; j < n; ++j) {
      assert(res.dist[i][j].has_value());
    }
  }

  verify_path_validity(g, 0, n - 1, res);

  std::cout << "  Floyd-Warshall (N = 300) time: " << elapsed_ms << " ms\n";
  std::cout << "  Sample distance [0 -> " << n - 1
            << "]: " << *res.dist[0][n - 1] << "\n";
  std::cout << "[PASS] test_performance_sanity" << std::endl;
}

} // namespace

int main() {
  std::cout << "========================================================\n";
  std::cout << " C-13.24: Floyd-Warshall Shortest Path Digraph Variation\n";
  std::cout << "========================================================\n"
            << std::endl;

  test_canonical_digraph();
  test_negative_edges_no_cycle();
  test_negative_weight_cycle();
  test_boundary_cases();
  test_parallel_edges();
  test_invalid_inputs_and_exceptions();
  test_randomized_stress();
  test_performance_sanity();

  std::cout << "\nAll test suites passed successfully!" << std::endl;
  return 0;
}
