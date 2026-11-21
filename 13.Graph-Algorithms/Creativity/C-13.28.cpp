// File : C-13.28.cpp

/*
     Let G be a graph with n vertices and m edges such that all the edge weights
    in G are integers in the range [1, n]. Give an algorithm for finding a mini-
    mum spanning tree for G in O(m log* n) time.
*/

#include "integer_weight_mst.hpp"

#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <optional>
#include <random>
#include <vector>

namespace {

/**
 * @brief Helper to verify that a computed spanning tree is structurally valid:
 * 1. Contains exactly n - 1 edges (for n >= 1).
 * 2. Forms an acyclic connected graph linking all n stations.
 * 3. Every edge belongs to the original network with matching weight.
 * 4. Sum of edge weights matches reported total_weight.
 */
void verify_mst_validity(const dsa::integer_graph &g,
                         const dsa::mst_result &res) {
  const std::size_t n = g.num_vertices();
  if (n <= 1) {
    assert(res.is_connected);
    assert(res.tree_edges.empty());
    assert(res.total_weight == 0);
    return;
  }

  if (!res.is_connected) {
    assert(res.tree_edges.size() < n - 1);
    return;
  }

  assert(res.tree_edges.size() == n - 1 &&
         "Spanning tree on n vertices must have exactly n - 1 edges");

  dsa::disjoint_set dsu(n);
  std::uint64_t sum_w = 0;

  for (const auto &edge : res.tree_edges) {
    assert(edge.u < n && edge.v < n);
    assert(edge.u != edge.v);

    // Verify channel exists in network with same weight
    bool found = false;
    for (const auto &nbr : g.adjacent(edge.u)) {
      if (nbr.to == edge.v && nbr.weight == edge.weight) {
        found = true;
        break;
      }
    }
    assert(found && "Selected edge does not exist in the graph!");

    // Verify acyclic property using DSU
    const bool merged = dsu.union_sets(edge.u, edge.v);
    assert(merged && "Spanning tree contains a cycle!");

    sum_w += edge.weight;
  }

  assert(dsu.count() == 1 && "Spanning tree does not link all vertices!");
  assert(sum_w == res.total_weight &&
         "Reported total weight does not match sum of edge weights!");
}

/**
 * @brief Independent brute-force oracle for small n.
 * Tests all combinations of n - 1 edges to find the true minimum weight spanning tree.
 */
std::optional<std::uint64_t>
brute_force_mst_oracle(const dsa::integer_graph &g) {
  const std::size_t n = g.num_vertices();
  if (n <= 1) {
    return 0;
  }

  const auto &edges = g.all_edges();
  const std::size_t m = edges.size();
  if (m < n - 1) {
    return std::nullopt;
  }

  std::optional<std::uint64_t> min_weight = std::nullopt;

  std::vector<std::size_t> indices(n - 1);
  std::iota(indices.begin(), indices.end(), 0);

  auto check_combination = [&]() {
    dsa::disjoint_set dsu(n);
    std::uint64_t cur_sum = 0;
    bool valid_tree = true;

    for (std::size_t idx : indices) {
      const auto &e = edges[idx];
      if (!dsu.union_sets(e.u, e.v)) {
        valid_tree = false;
        break;
      }
      cur_sum += e.weight;
    }

    if (valid_tree && dsu.count() == 1) {
      if (!min_weight.has_value() || cur_sum < *min_weight) {
        min_weight = cur_sum;
      }
    }
  };

  auto generate_combinations = [&](auto &self, std::size_t depth,
                                   std::size_t start) -> void {
    if (depth == n - 1) {
      check_combination();
      return;
    }
    for (std::size_t i = start; i < m; ++i) {
      indices[depth] = i;
      self(self, depth + 1, i + 1);
    }
  };

  generate_combinations(generate_combinations, 0, 0);
  return min_weight;
}

/**
 * @brief Tests the iterated logarithm function log*(n).
 */
void test_log_star() {
  std::cout << "[RUN] test_log_star" << std::endl;

  assert(dsa::log_star(1) == 0);
  assert(dsa::log_star(2) == 1);
  assert(dsa::log_star(3) == 2);
  assert(dsa::log_star(4) == 2);
  assert(dsa::log_star(5) == 3);
  assert(dsa::log_star(16) == 3);
  assert(dsa::log_star(17) == 4);
  assert(dsa::log_star(65536) == 4);
  assert(dsa::log_star(65537) == 5);

  std::cout << "[PASS] test_log_star" << std::endl;
}

/**
 * @brief Tests canonical graph with edge weights strictly in [1, n].
 */
void test_canonical_integer_graph() {
  std::cout << "[RUN] test_canonical_integer_graph" << std::endl;

  // 5 vertices, weights in [1, 5]
  dsa::integer_graph g(5);
  g.add_edge(0, 1, 1);
  g.add_edge(1, 2, 2);
  g.add_edge(2, 3, 3);
  g.add_edge(3, 4, 4);
  g.add_edge(0, 4, 5);
  g.add_edge(0, 2, 4);
  g.add_edge(1, 3, 3);

  const auto res = dsa::integer_weight_mst(g);
  const auto ref = dsa::comparison_kruskal_mst(g);
  const auto oracle = brute_force_mst_oracle(g);

  assert(res.is_connected && ref.is_connected);
  assert(oracle.has_value());

  assert(res.total_weight == 10); // 1 + 2 + 3 + 4 = 10
  assert(ref.total_weight == 10);
  assert(*oracle == 10);

  verify_mst_validity(g, res);
  verify_mst_validity(g, ref);

  std::cout << "[PASS] test_canonical_integer_graph (Total weight = 10)"
            << std::endl;
}

/**
 * @brief Tests boundary and small graph cases.
 */
void test_boundary_cases() {
  std::cout << "[RUN] test_boundary_cases" << std::endl;

  // n = 0
  dsa::integer_graph g0(0);
  const auto res0 = dsa::integer_weight_mst(g0);
  assert(res0.is_connected && res0.tree_edges.empty());

  // n = 1
  dsa::integer_graph g1(1);
  const auto res1 = dsa::integer_weight_mst(g1);
  assert(res1.is_connected && res1.tree_edges.empty());

  // n = 2, weight in [1, 2]
  dsa::integer_graph g2(2);
  g2.add_edge(0, 1, 2);
  const auto res2 = dsa::integer_weight_mst(g2);
  assert(res2.is_connected && res2.total_weight == 2);
  verify_mst_validity(g2, res2);

  // Disconnected graph
  dsa::integer_graph g_disc(4);
  g_disc.add_edge(0, 1, 1);
  g_disc.add_edge(2, 3, 2);
  const auto res_disc = dsa::integer_weight_mst(g_disc);
  assert(!res_disc.is_connected);

  // Parallel edges: 0-1 with weights 1 and 3 (algorithm must pick 1)
  dsa::integer_graph g_par(3);
  g_par.add_edge(0, 1, 3);
  g_par.add_edge(0, 1, 1); // Shorter parallel edge
  g_par.add_edge(1, 2, 2);
  const auto res_par = dsa::integer_weight_mst(g_par);
  assert(res_par.is_connected && res_par.total_weight == 3); // 1 + 2
  verify_mst_validity(g_par, res_par);

  std::cout << "[PASS] test_boundary_cases" << std::endl;
}

/**
 * @brief Tests invalid inputs and exception handling.
 */
void test_invalid_inputs_and_exceptions() {
  std::cout << "[RUN] test_invalid_inputs_and_exceptions" << std::endl;

  dsa::integer_graph g(3);

  // Weight 0 not allowed
  bool caught_zero = false;
  try {
    g.add_edge(0, 1, 0);
  } catch (const std::invalid_argument &) {
    caught_zero = true;
  }
  assert(caught_zero);

  // Self loop not allowed
  bool caught_self = false;
  try {
    g.add_edge(1, 1, 2);
  } catch (const std::invalid_argument &) {
    caught_self = true;
  }
  assert(caught_self);

  // Out of bounds vertex
  bool caught_oob = false;
  try {
    g.add_edge(0, 5, 1);
  } catch (const std::out_of_range &) {
    caught_oob = true;
  }
  assert(caught_oob);

  std::cout << "[PASS] test_invalid_inputs_and_exceptions" << std::endl;
}

/**
 * @brief Randomized stress test with edge weights in [1, n], comparing against
 *        reference Kruskal and brute-force oracle.
 */
void test_randomized_stress() {
  std::cout << "[RUN] test_randomized_stress (50 random graphs)" << std::endl;
  std::mt19937_64 rng(1328); // Fixed seed

  const std::size_t num_tests = 50;

  for (std::size_t t = 0; t < num_tests; ++t) {
    std::uniform_int_distribution<std::size_t> n_dist(3, 6);
    const std::size_t n = n_dist(rng);

    dsa::integer_graph g(n);

    // Weights uniformly distributed in [1, n]
    std::uniform_int_distribution<std::uint64_t> w_dist(1, n);

    // Build backbone chain to guarantee connectivity
    for (std::size_t i = 1; i < n; ++i) {
      std::uniform_int_distribution<std::size_t> p_dist(0, i - 1);
      g.add_edge(i, p_dist(rng), w_dist(rng));
    }

    // Add random extra edges
    std::uniform_int_distribution<std::size_t> extra_dist(1, n * 2);
    const std::size_t extra_m = extra_dist(rng);
    std::uniform_int_distribution<std::size_t> v_dist(0, n - 1);

    for (std::size_t e = 0; e < extra_m; ++e) {
      const std::size_t u = v_dist(rng);
      const std::size_t v = v_dist(rng);
      if (u != v) {
        g.add_edge(u, v, w_dist(rng));
      }
    }

    const auto res = dsa::integer_weight_mst(g);
    const auto ref = dsa::comparison_kruskal_mst(g);
    const auto oracle = brute_force_mst_oracle(g);

    assert(res.is_connected && ref.is_connected);
    assert(oracle.has_value());
    assert(res.total_weight == ref.total_weight);
    assert(res.total_weight == *oracle);

    verify_mst_validity(g, res);
    verify_mst_validity(g, ref);
  }

  std::cout << "[PASS] test_randomized_stress (50 random graphs verified)"
            << std::endl;
}

/**
 * @brief Performance sanity check confirming O(m log* n) efficiency on a large graph.
 */
void test_performance_sanity() {
  std::cout
      << "[RUN] test_performance_sanity (N = 10,000 vertices, M = 50,000 edges)"
      << std::endl;

  const std::size_t n = 10000;
  const std::size_t m = 50000;

  dsa::integer_graph g(n);
  std::mt19937_64 rng(42);
  std::uniform_int_distribution<std::uint64_t> w_dist(1, n);
  std::uniform_int_distribution<std::size_t> v_dist(0, n - 1);

  // Spanning chain
  for (std::size_t i = 0; i + 1 < n; ++i) {
    g.add_edge(i, i + 1, w_dist(rng));
  }

  // Extra random edges
  for (std::size_t i = n - 1; i < m; ++i) {
    const std::size_t u = v_dist(rng);
    const std::size_t v = v_dist(rng);
    if (u != v) {
      g.add_edge(u, v, w_dist(rng));
    }
  }

  const auto t0 = std::chrono::steady_clock::now();
  const auto res = dsa::integer_weight_mst(g);
  const auto t1 = std::chrono::steady_clock::now();
  const auto ref = dsa::comparison_kruskal_mst(g);
  const auto t2 = std::chrono::steady_clock::now();

  const auto linear_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
  const auto comp_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

  assert(res.is_connected && ref.is_connected);
  assert(res.total_weight == ref.total_weight);
  verify_mst_validity(g, res);

  std::cout << "  Integer Weight MST (Counting Sort) time: " << linear_ms
            << " ms\n";
  std::cout << "  Comparison Kruskal MST (std::sort) time: " << comp_ms
            << " ms\n";
  std::cout << "  MST Total Weight: " << res.total_weight << "\n";
  std::cout << "  log*(N = " << n << ") = " << dsa::log_star(n) << "\n";
  std::cout << "[PASS] test_performance_sanity" << std::endl;
}

} // namespace

int main() {
  std::cout << "========================================================\n";
  std::cout << " C-13.28: Integer-Weight MST in O(m log* n) Time\n";
  std::cout << "========================================================\n"
            << std::endl;

  test_log_star();
  test_canonical_integer_graph();
  test_boundary_cases();
  test_invalid_inputs_and_exceptions();
  test_randomized_stress();
  test_performance_sanity();

  std::cout << "\nAll test suites passed successfully!" << std::endl;
  return 0;
}
