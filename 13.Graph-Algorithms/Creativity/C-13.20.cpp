// File : C-13.20.cpp

/*
    C-13.20 Computer networks should avoid single points of failure, that is, network
    nodes that can disconnect the network if they fail. We say a connected
    graph G is biconnected if it contains no vertex whose removal would di-
    vide G into two or more connected components. Give an O(n + m)-time
    algorithm for adding at most n edges to a connected graph G, with n ≥ 3
    vertices and m >= n - 1 edges, to guarantee that G is biconnected.
*/

#include "biconnectivity.hpp"

#include <cassert>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <random>
#include <vector>

namespace {

/**
 * @brief Independent brute-force oracle to determine biconnectivity.
 *
 * For each vertex v in V, simulates the removal of v and verifies that the remaining
 * n - 1 vertices form a single connected component using BFS.
 *
 * @section complexity
 * Time: $O(n \cdot (n + m))$. Auxiliary space: $O(n)$.
 */
bool brute_force_is_biconnected(const dsa::undirected_graph &g) {
  const std::size_t n = g.num_vertices();
  if (n < 3) {
    return false;
  }
  if (!g.is_connected()) {
    return false;
  }

  for (std::size_t removed = 0; removed < n; ++removed) {
    std::vector<bool> visited(n, false);
    std::queue<std::size_t> q;

    // Pick start vertex different from 'removed'
    const std::size_t start = (removed == 0) ? 1 : 0;
    visited[start] = true;
    q.push(start);
    std::size_t reached = 0;

    while (!q.empty()) {
      const std::size_t u = q.front();
      q.pop();
      ++reached;

      for (std::size_t v : g.adjacent(u)) {
        if (v != removed && !visited[v]) {
          visited[v] = true;
          q.push(v);
        }
      }
    }

    if (reached < n - 1) {
      // Vertex 'removed' is an articulation point
      return false;
    }
  }

  return true;
}

/**
 * @brief Helper to verify that augmentation meets all problem requirements:
 * 1. Number of added edges <= n.
 * 2. None of the added edges are self-loops.
 * 3. None of the added edges existed in the original graph.
 * 4. The augmented graph is biconnected by both Hopcroft-Tarjan and Brute-Force oracle.
 */
void verify_augmentation(const dsa::undirected_graph &orig,
                         const std::vector<dsa::edge> &added) {
  const std::size_t n = orig.num_vertices();
  assert(added.size() <= n && "Added edges must not exceed n");

  dsa::undirected_graph aug = orig;
  for (const auto &e : added) {
    assert(e.u != e.v && "Added edge must not be a self-loop");
    assert(e.u < n && e.v < n && "Endpoints out of range");
    assert(!orig.has_edge(e.u, e.v) &&
           "Added edge must not already exist in original graph");
    aug.add_edge(e.u, e.v);
  }

  // Verify biconnectivity using both the O(n + m) algorithm and independent oracle
  assert(dsa::is_biconnected(aug) &&
         "Augmented graph must be biconnected (Hopcroft-Tarjan)");
  assert(brute_force_is_biconnected(aug) &&
         "Augmented graph must be biconnected (Brute-force oracle)");
}

/**
 * @brief Tests canonical path graph P_n (0-1-2-...-(n-1)).
 * Expected: Articulation points at 1, 2, ..., n-2.
 * Augmentation adds missing edge (n-1, 0), completing cycle C_n with only 1 added edge.
 */
void test_path_graph() {
  std::cout << "[RUN] test_path_graph\n";
  const std::size_t n = 6;
  dsa::undirected_graph g(n);
  for (std::size_t i = 0; i + 1 < n; ++i) {
    g.add_edge(i, i + 1);
  }

  assert(!dsa::is_biconnected(g));
  assert(!brute_force_is_biconnected(g));

  const auto diag = dsa::analyze_biconnectivity(g);
  assert(diag.cut_vertices.size() == n - 2);

  const auto added = dsa::biconnect_graph(g);
  // On a simple path, completing the cycle adds exactly 1 edge
  assert(added.size() == 1);
  assert(added[0] == dsa::edge(0, n - 1));

  verify_augmentation(g, added);
  std::cout << "[PASS] test_path_graph (added " << added.size() << " edge)\n";
}

/**
 * @brief Tests star graph K_{1, n-1} where vertex 0 is connected to all leaves.
 * Expected: 0 is the cut vertex; leaves are connected into a ring by augmentation.
 */
void test_star_graph() {
  std::cout << "[RUN] test_star_graph\n";
  const std::size_t n = 7;
  dsa::undirected_graph g(n);
  for (std::size_t i = 1; i < n; ++i) {
    g.add_edge(0, i);
  }

  assert(!dsa::is_biconnected(g));
  const auto diag = dsa::analyze_biconnectivity(g);
  assert(diag.cut_vertices.size() == 1 && diag.cut_vertices[0] == 0);

  const auto added = dsa::biconnect_graph(g);
  assert(added.size() <= n);

  verify_augmentation(g, added);
  std::cout << "[PASS] test_star_graph (added " << added.size()
            << " edges, wheel formed)\n";
}

/**
 * @brief Tests barbell graph: two 3-cycles joined by a central bridge.
 */
void test_barbell_graph() {
  std::cout << "[RUN] test_barbell_graph\n";
  // Left cycle: 0-1-2-0
  // Bridge: 2-3
  // Right cycle: 3-4-5-3
  dsa::undirected_graph g(6);
  g.add_edge(0, 1);
  g.add_edge(1, 2);
  g.add_edge(2, 0);

  g.add_edge(2, 3); // Bridge

  g.add_edge(3, 4);
  g.add_edge(4, 5);
  g.add_edge(5, 3);

  assert(!dsa::is_biconnected(g));
  const auto diag = dsa::analyze_biconnectivity(g);
  assert(diag.bridges.size() == 1 && diag.bridges[0] == dsa::edge(2, 3));

  const auto added = dsa::biconnect_graph(g);
  verify_augmentation(g, added);
  std::cout << "[PASS] test_barbell_graph (added " << added.size()
            << " edges)\n";
}

/**
 * @brief Tests graphs that are already biconnected (e.g. C_n and K_n).
 * Expected: 0 edges added.
 */
void test_already_biconnected() {
  std::cout << "[RUN] test_already_biconnected\n";

  // Cycle C_5
  dsa::undirected_graph c5(5);
  for (std::size_t i = 0; i < 5; ++i) {
    c5.add_edge(i, (i + 1) % 5);
  }
  assert(dsa::is_biconnected(c5));
  assert(brute_force_is_biconnected(c5));
  const auto added_c5 = dsa::biconnect_graph(c5);
  assert(added_c5.empty() && "Already biconnected graph must need 0 edges");

  // Complete graph K_4
  dsa::undirected_graph k4(4);
  for (std::size_t i = 0; i < 4; ++i) {
    for (std::size_t j = i + 1; j < 4; ++j) {
      k4.add_edge(i, j);
    }
  }
  assert(dsa::is_biconnected(k4));
  const auto added_k4 = dsa::biconnect_graph(k4);
  assert(added_k4.empty());

  std::cout << "[PASS] test_already_biconnected (0 edges added)\n";
}

/**
 * @brief Tests minimal boundary values (n = 3).
 */
void test_minimal_graph_n3() {
  std::cout << "[RUN] test_minimal_graph_n3\n";

  // Triangle K_3 (already biconnected)
  dsa::undirected_graph k3(3);
  k3.add_edge(0, 1);
  k3.add_edge(1, 2);
  k3.add_edge(2, 0);
  assert(dsa::is_biconnected(k3));
  assert(dsa::biconnect_graph(k3).empty());

  // Path of length 2: 0-1-2
  dsa::undirected_graph p3(3);
  p3.add_edge(0, 1);
  p3.add_edge(1, 2);
  assert(!dsa::is_biconnected(p3));
  const auto added_p3 = dsa::biconnect_graph(p3);
  assert(added_p3.size() == 1);
  verify_augmentation(p3, added_p3);

  std::cout << "[PASS] test_minimal_graph_n3\n";
}

/**
 * @brief Tests invalid inputs and exception handling.
 */
void test_invalid_inputs_and_exceptions() {
  std::cout << "[RUN] test_invalid_inputs_and_exceptions\n";

  // Graph with n < 3
  dsa::undirected_graph g2(2);
  g2.add_edge(0, 1);
  bool caught_small = false;
  try {
    [[maybe_unused]] auto res = dsa::biconnect_graph(g2);
  } catch (const std::invalid_argument &) {
    caught_small = true;
  }
  assert(caught_small);

  // Disconnected graph
  dsa::undirected_graph g_disc(4);
  g_disc.add_edge(0, 1);
  g_disc.add_edge(2, 3);
  bool caught_disc = false;
  try {
    [[maybe_unused]] auto res = dsa::biconnect_graph(g_disc);
  } catch (const std::invalid_argument &) {
    caught_disc = true;
  }
  assert(caught_disc);

  // Self-loop exception
  bool caught_self_loop = false;
  try {
    dsa::undirected_graph g(3);
    g.add_edge(1, 1);
  } catch (const std::invalid_argument &) {
    caught_self_loop = true;
  }
  assert(caught_self_loop);

  // Out of range vertex
  bool caught_oob = false;
  try {
    dsa::undirected_graph g(3);
    g.add_edge(0, 5);
  } catch (const std::out_of_range &) {
    caught_oob = true;
  }
  assert(caught_oob);

  std::cout << "[PASS] test_invalid_inputs_and_exceptions\n";
}

/**
 * @brief Randomized stress test comparing against the independent brute-force oracle.
 */
void test_randomized_stress() {
  std::cout << "[RUN] test_randomized_stress (50 random trees and connected "
               "graphs)\n";
  std::mt19937_64 rng(1320); // Fixed seed

  const std::size_t num_tests = 50;

  for (std::size_t t = 0; t < num_tests; ++t) {
    std::uniform_int_distribution<std::size_t> n_dist(3, 25);
    const std::size_t n = n_dist(rng);

    dsa::undirected_graph g(n);

    // Build random spanning tree to ensure connectivity
    for (std::size_t i = 1; i < n; ++i) {
      std::uniform_int_distribution<std::size_t> parent_dist(0, i - 1);
      g.add_edge(i, parent_dist(rng));
    }

    // Add random extra edges
    std::uniform_int_distribution<std::size_t> extra_dist(0, n);
    const std::size_t extra_edges = extra_dist(rng);
    std::uniform_int_distribution<std::size_t> v_dist(0, n - 1);

    for (std::size_t e = 0; e < extra_edges; ++e) {
      std::size_t u = v_dist(rng);
      std::size_t v = v_dist(rng);
      if (u != v && !g.has_edge(u, v)) {
        g.add_edge(u, v);
      }
    }

    // Check pre-condition agreement between algorithms
    const bool is_biconn_ht = dsa::is_biconnected(g);
    const bool is_biconn_bf = brute_force_is_biconnected(g);
    assert(is_biconn_ht == is_biconn_bf &&
           "Hopcroft-Tarjan and Brute-force oracle must agree");

    const auto added = dsa::biconnect_graph(g);
    verify_augmentation(g, added);
  }

  std::cout << "[PASS] test_randomized_stress (50 instances verified)\n";
}

/**
 * @brief Performance sanity check on a large graph confirming O(n + m) scaling.
 */
void test_performance_sanity() {
  std::cout << "[RUN] test_performance_sanity (N = 20,000 vertices, M = 60,000 "
               "edges)\n";

  const std::size_t n = 20000;
  const std::size_t extra_m = 40000;

  // Case 1: Pure deep path graph with 20,000 vertices
  dsa::undirected_graph line_g(n);
  for (std::size_t i = 0; i + 1 < n; ++i) {
    line_g.add_edge(i, i + 1);
  }
  assert(!dsa::is_biconnected(line_g));

  const auto t0 = std::chrono::steady_clock::now();
  const auto line_added = dsa::biconnect_graph(line_g);
  const auto t1 = std::chrono::steady_clock::now();
  const auto line_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

  assert(line_added.size() <= n);
  assert(line_added.size() == 1);
  dsa::undirected_graph aug_line = line_g;
  aug_line.add_edge(line_added[0].u, line_added[0].v);
  assert(dsa::is_biconnected(aug_line));

  std::cout << "  Deep Path (N = " << n << ") time: " << line_ms
            << " ms, added: " << line_added.size() << " edge\n";

  // Case 2: Large connected graph with random cross edges
  dsa::undirected_graph g(n);
  std::mt19937_64 rng(42);

  for (std::size_t i = 0; i + 1 < n; ++i) {
    g.add_edge(i, i + 1);
  }

  std::uniform_int_distribution<std::size_t> v_dist(0, n - 1);
  for (std::size_t i = 0; i < extra_m; ++i) {
    std::size_t u = v_dist(rng);
    std::size_t v = v_dist(rng);
    if (u != v && !g.has_edge(u, v)) {
      g.add_edge(u, v);
    }
  }

  const auto t2 = std::chrono::steady_clock::now();
  const auto added = dsa::biconnect_graph(g);
  const auto t3 = std::chrono::steady_clock::now();

  const auto elapsed_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count();

  assert(added.size() <= n);

  dsa::undirected_graph aug = g;
  for (const auto &e : added) {
    aug.add_edge(e.u, e.v);
  }
  assert(dsa::is_biconnected(aug));

  std::cout << "  Large Graph (N = " << n << ", M = " << g.num_edges()
            << ") time: " << elapsed_ms << " ms, added: " << added.size()
            << " edges\n";
  std::cout << "[PASS] test_performance_sanity\n";
}

} // namespace

int main() {
  std::cout << "Starting tests..." << std::endl;
  try {
    test_path_graph();
    std::cout << "Path graph passed." << std::endl;
    test_star_graph();
    std::cout << "Star graph passed." << std::endl;
    test_barbell_graph();
    std::cout << "Barbell graph passed." << std::endl;
    test_already_biconnected();
    std::cout << "Already biconnected passed." << std::endl;
    test_minimal_graph_n3();
    std::cout << "Minimal graph n3 passed." << std::endl;
    test_invalid_inputs_and_exceptions();
    std::cout << "Invalid inputs passed." << std::endl;
    test_randomized_stress();
    std::cout << "Randomized stress passed." << std::endl;
    test_performance_sanity();
    std::cout << "Performance sanity passed." << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Caught exception in main: " << e.what() << std::endl;
    return 1;
  }

  std::cout << "\nAll test suites passed successfully!" << std::endl;
  return 0;
}
