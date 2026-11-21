// File : C-13.26.cpp

/*
    C-13.26 A graph G is bipartite if its vertices can be partitioned into two sets X and
    Y such that every edge in G has one end vertex in X and the other in Y .
    Design and analyze an efficient algorithm for determining if an undirected
    graph G is bipartite (without knowing the sets X and Y in advance).
*/

#include "bipartite.hpp"

#include <cassert>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <random>
#include <vector>

namespace {

/**
 * @brief Helper to verify that the bipartite partition (X, Y) is valid:
 * 1. X and Y form a complete partition of V (X union Y = V, X intersect Y = empty).
 * 2. Every edge in G has one endpoint in X and the other in Y (no monochromatic edges).
 */
void verify_bipartite_partition(const dsa::undirected_graph &g,
                                const dsa::bipartite_result &res) {
  const std::size_t n = g.num_vertices();
  assert(res.is_bipartite);
  assert(res.set_x.size() + res.set_y.size() == n);

  std::vector<int> side(n, -1);
  for (std::size_t u : res.set_x) {
    assert(side[u] == -1 && "Vertex present multiple times in partition");
    side[u] = 0;
  }
  for (std::size_t v : res.set_y) {
    assert(side[v] == -1 && "Vertex present in both X and Y");
    side[v] = 1;
  }

  // Verify that every edge crosses between X and Y
  for (std::size_t u = 0; u < n; ++u) {
    for (std::size_t v : g.adjacent(u)) {
      if (u == v) {
        assert(false && "Bipartite graph cannot contain self-loops!");
      }
      assert(side[u] != side[v] &&
             "Edge connects two vertices in the same partition set!");
    }
  }
}

/**
 * @brief Helper to verify that an odd cycle certificate is valid:
 * 1. Cycle length (number of edges) is odd and >= 3 (or 1 for self-loop).
 * 2. Every edge along the cycle physically exists in G.
 */
void verify_odd_cycle_certificate(const dsa::undirected_graph &g,
                                  const std::vector<std::size_t> &cycle) {
  assert(!cycle.empty());
  assert(cycle.front() == cycle.back() &&
         "Cycle must start and end at same vertex");
  const std::size_t num_edges = cycle.size() - 1;
  assert(num_edges % 2 == 1 && "Cycle certificate must have odd length!");

  for (std::size_t i = 0; i + 1 < cycle.size(); ++i) {
    const std::size_t u = cycle[i];
    const std::size_t v = cycle[i + 1];
    assert(g.has_edge(u, v) && "Cycle edge does not exist in graph!");
  }
}

/**
 * @brief Independent brute-force oracle for small graphs (n <= 12).
 * Exhaustively checks all 2^n possible 2-colorings of V.
 */
bool brute_force_bipartite_oracle(const dsa::undirected_graph &g) {
  const std::size_t n = g.num_vertices();
  if (n == 0) {
    return true;
  }

  // Check self-loops
  for (std::size_t u = 0; u < n; ++u) {
    for (std::size_t v : g.adjacent(u)) {
      if (u == v) {
        return false;
      }
    }
  }

  const std::size_t max_mask = (std::size_t{1} << n);
  for (std::size_t mask = 0; mask < max_mask; ++mask) {
    bool valid = true;
    for (std::size_t u = 0; u < n && valid; ++u) {
      const int color_u = (mask >> u) & 1;
      for (std::size_t v : g.adjacent(u)) {
        const int color_v = (mask >> v) & 1;
        if (color_u == color_v) {
          valid = false;
          break;
        }
      }
    }
    if (valid) {
      return true;
    }
  }
  return false;
}

/**
 * @brief Tests canonical bipartite graphs: complete bipartite K_{2, 3}, trees, and even cycles.
 */
void test_canonical_bipartite_graphs() {
  std::cout << "[RUN] test_canonical_bipartite_graphs" << std::endl;

  // 1. Complete bipartite graph K_{2, 3}
  // Part X = {0, 1}, Part Y = {2, 3, 4}
  dsa::undirected_graph k23(5);
  for (std::size_t u = 0; u < 2; ++u) {
    for (std::size_t v = 2; v < 5; ++v) {
      k23.add_edge(u, v);
    }
  }
  const auto res_k23 = dsa::check_bipartite(k23);
  assert(res_k23.is_bipartite);
  verify_bipartite_partition(k23, res_k23);

  // 2. Tree (any tree is bipartite)
  dsa::undirected_graph tree(7);
  tree.add_edge(0, 1);
  tree.add_edge(0, 2);
  tree.add_edge(1, 3);
  tree.add_edge(1, 4);
  tree.add_edge(2, 5);
  tree.add_edge(2, 6);
  const auto res_tree = dsa::check_bipartite(tree);
  assert(res_tree.is_bipartite);
  verify_bipartite_partition(tree, res_tree);

  // 3. Even cycle C_6
  dsa::undirected_graph c6(6);
  for (std::size_t i = 0; i < 6; ++i) {
    c6.add_edge(i, (i + 1) % 6);
  }
  const auto res_c6 = dsa::check_bipartite(c6);
  assert(res_c6.is_bipartite);
  verify_bipartite_partition(c6, res_c6);

  std::cout << "[PASS] test_canonical_bipartite_graphs" << std::endl;
}

/**
 * @brief Tests canonical non-bipartite graphs: odd cycles and complete graphs.
 */
void test_canonical_non_bipartite_graphs() {
  std::cout << "[RUN] test_canonical_non_bipartite_graphs" << std::endl;

  // 1. Triangle C_3
  dsa::undirected_graph c3(3);
  c3.add_edge(0, 1);
  c3.add_edge(1, 2);
  c3.add_edge(2, 0);
  const auto res_c3 = dsa::check_bipartite(c3);
  assert(!res_c3.is_bipartite);
  verify_odd_cycle_certificate(c3, res_c3.odd_cycle);

  // 2. 5-cycle C_5
  dsa::undirected_graph c5(5);
  for (std::size_t i = 0; i < 5; ++i) {
    c5.add_edge(i, (i + 1) % 5);
  }
  const auto res_c5 = dsa::check_bipartite(c5);
  assert(!res_c5.is_bipartite);
  verify_odd_cycle_certificate(c5, res_c5.odd_cycle);

  // 3. Complete graph K_4
  dsa::undirected_graph k4(4);
  for (std::size_t i = 0; i < 4; ++i) {
    for (std::size_t j = i + 1; j < 4; ++j) {
      k4.add_edge(i, j);
    }
  }
  const auto res_k4 = dsa::check_bipartite(k4);
  assert(!res_k4.is_bipartite);
  verify_odd_cycle_certificate(k4, res_k4.odd_cycle);

  std::cout << "[PASS] test_canonical_non_bipartite_graphs" << std::endl;
}

/**
 * @brief Tests boundary and disconnected cases.
 */
void test_boundary_and_disconnected() {
  std::cout << "[RUN] test_boundary_and_disconnected" << std::endl;

  // Empty graph
  dsa::undirected_graph g0(0);
  assert(dsa::is_bipartite(g0));

  // Single vertex
  dsa::undirected_graph g1(1);
  assert(dsa::is_bipartite(g1));
  const auto res_g1 = dsa::check_bipartite(g1);
  verify_bipartite_partition(g1, res_g1);

  // Single edge
  dsa::undirected_graph g2(2);
  g2.add_edge(0, 1);
  assert(dsa::is_bipartite(g2));
  verify_bipartite_partition(g2, dsa::check_bipartite(g2));

  // Self-loop (odd cycle of length 1)
  dsa::undirected_graph g_loop(2);
  g_loop.add_edge(0, 0);
  assert(!dsa::is_bipartite(g_loop));

  // Disconnected: both components bipartite -> graph is bipartite
  dsa::undirected_graph g_disc_bip(6);
  // Component 1: C_4 (0-1-2-3-0)
  g_disc_bip.add_edge(0, 1);
  g_disc_bip.add_edge(1, 2);
  g_disc_bip.add_edge(2, 3);
  g_disc_bip.add_edge(3, 0);
  // Component 2: path 4-5
  g_disc_bip.add_edge(4, 5);
  const auto res_disc_bip = dsa::check_bipartite(g_disc_bip);
  assert(res_disc_bip.is_bipartite);
  verify_bipartite_partition(g_disc_bip, res_disc_bip);

  // Disconnected: one component contains an odd cycle -> graph is NOT bipartite
  dsa::undirected_graph g_disc_odd(6);
  // Component 1: path 0-1
  g_disc_odd.add_edge(0, 1);
  // Component 2: triangle 2-3-4-2
  g_disc_odd.add_edge(2, 3);
  g_disc_odd.add_edge(3, 4);
  g_disc_odd.add_edge(4, 2);
  const auto res_disc_odd = dsa::check_bipartite(g_disc_odd);
  assert(!res_disc_odd.is_bipartite);
  verify_odd_cycle_certificate(g_disc_odd, res_disc_odd.odd_cycle);

  std::cout << "[PASS] test_boundary_and_disconnected" << std::endl;
}

/**
 * @brief Tests invalid inputs and exception handling.
 */
void test_invalid_inputs_and_exceptions() {
  std::cout << "[RUN] test_invalid_inputs_and_exceptions" << std::endl;

  dsa::undirected_graph g(3);

  bool caught_add = false;
  try {
    g.add_edge(0, 5);
  } catch (const std::out_of_range &) {
    caught_add = true;
  }
  assert(caught_add);

  bool caught_adjacent = false;
  try {
    [[maybe_unused]] const auto &nbrs = g.adjacent(10);
  } catch (const std::out_of_range &) {
    caught_adjacent = true;
  }
  assert(caught_adjacent);

  std::cout << "[PASS] test_invalid_inputs_and_exceptions" << std::endl;
}

/**
 * @brief Randomized stress test comparing against the independent brute-force oracle.
 */
void test_randomized_stress() {
  std::cout << "[RUN] test_randomized_stress (50 random graphs)" << std::endl;
  std::mt19937_64 rng(1326); // Fixed seed

  const std::size_t num_tests = 50;

  for (std::size_t t = 0; t < num_tests; ++t) {
    std::uniform_int_distribution<std::size_t> n_dist(2, 10);
    const std::size_t n = n_dist(rng);

    dsa::undirected_graph g(n);

    std::uniform_int_distribution<std::size_t> m_dist(1, n * 2);
    const std::size_t m = m_dist(rng);
    std::uniform_int_distribution<std::size_t> v_dist(0, n - 1);

    for (std::size_t i = 0; i < m; ++i) {
      const std::size_t u = v_dist(rng);
      const std::size_t v = v_dist(rng);
      if (u != v && !g.has_edge(u, v)) {
        g.add_edge(u, v);
      }
    }

    const auto res = dsa::check_bipartite(g);
    const bool oracle_bip = brute_force_bipartite_oracle(g);

    assert(res.is_bipartite == oracle_bip);

    if (res.is_bipartite) {
      verify_bipartite_partition(g, res);
    } else {
      verify_odd_cycle_certificate(g, res.odd_cycle);
    }
  }

  std::cout << "[PASS] test_randomized_stress (50 random graphs verified)"
            << std::endl;
}

/**
 * @brief Performance sanity check confirming O(n + m) scaling on large graph.
 */
void test_performance_sanity() {
  std::cout
      << "[RUN] test_performance_sanity (N = 20,000 vertices, M = 60,000 edges)"
      << std::endl;

  const std::size_t n = 20000;
  const std::size_t m = 60000;

  // Build large bipartite graph: partitions X = {0 .. 9999}, Y = {10000 .. 19999}
  dsa::undirected_graph g(n);
  std::mt19937_64 rng(42);
  std::uniform_int_distribution<std::size_t> x_dist(0, n / 2 - 1);
  std::uniform_int_distribution<std::size_t> y_dist(n / 2, n - 1);

  // Backbone chain connecting partitions
  for (std::size_t i = 0; i < n / 2; ++i) {
    g.add_edge(i, n / 2 + i);
    if (i + 1 < n / 2) {
      g.add_edge(n / 2 + i, i + 1);
    }
  }

  for (std::size_t i = n; i < m; ++i) {
    const std::size_t u = x_dist(rng);
    const std::size_t v = y_dist(rng);
    g.add_edge(u, v);
  }

  const auto t0 = std::chrono::steady_clock::now();
  const auto res = dsa::check_bipartite(g);
  const auto t1 = std::chrono::steady_clock::now();

  const auto elapsed_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

  assert(res.is_bipartite);
  verify_bipartite_partition(g, res);

  std::cout << "  check_bipartite (N = " << n << ", M = " << g.num_edges()
            << ") time: " << elapsed_ms << " ms\n";
  std::cout << "[PASS] test_performance_sanity" << std::endl;
}

} // namespace

int main() {
  std::cout << "========================================================\n";
  std::cout << " C-13.26: Bipartite Graph Detection and Odd-Cycle Cert\n";
  std::cout << "========================================================\n"
            << std::endl;

  test_canonical_bipartite_graphs();
  test_canonical_non_bipartite_graphs();
  test_boundary_and_disconnected();
  test_invalid_inputs_and_exceptions();
  test_randomized_stress();
  test_performance_sanity();

  std::cout << "\nAll test suites passed successfully!" << std::endl;
  return 0;
}
