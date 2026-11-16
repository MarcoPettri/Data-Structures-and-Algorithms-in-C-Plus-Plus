// File : C-13.1.cpp

/*
    C-13.1 Say that an n-vertex directed acyclic graph G is compact if there is some
    way of numbering the vertices of G with the integers from 0 to n − 1 such
    that G contains the edge (i, j) if and only if i < j, for all i, j in [0, n − 1].
    Give an O(n^2)-time algorithm for detecting if G is compact.

*/

#include "compact_dag.hpp"

#include <cassert>
#include <chrono>
#include <iostream>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

namespace {

/**
 * @brief Independent brute-force oracle for small n.
 * Checks all n! permutations to determine if any satisfies (u, v) in E <=> pi(u) < pi(v).
 */
bool brute_force_compact_oracle(const dsa::DirectedGraph &graph) {
  const std::size_t n = graph.num_vertices();
  if (n <= 1) {
    return graph.num_edges() == 0;
  }
  if (graph.num_edges() != (n * (n - 1)) / 2) {
    return false;
  }

  std::vector<std::size_t> p(n);
  std::iota(p.begin(), p.end(), 0);

  do {
    bool valid = true;
    for (std::size_t u = 0; u < n && valid; ++u) {
      for (std::size_t v = 0; v < n && valid; ++v) {
        if (u == v) {
          if (graph.has_edge(u, v)) {
            valid = false;
          }
        } else {
          const bool edge_exists = graph.has_edge(u, v);
          const bool expected = (p[u] < p[v]);
          if (edge_exists != expected) {
            valid = false;
          }
        }
      }
    }
    if (valid) {
      return true;
    }
  } while (std::next_permutation(p.begin(), p.end()));

  return false;
}

void test_edge_cases() {
  std::cout << "[*] Running edge case tests...\n";

  // Edge Case 1: Empty graph (n = 0)
  {
    dsa::DirectedGraph g(0);
    auto res = dsa::is_compact_dag(g);
    assert(res);
    assert(res.is_compact);
    assert(res.vertex_to_rank.empty());
  }

  // Edge Case 2: Single vertex (n = 1), 0 edges
  {
    dsa::DirectedGraph g(1);
    auto res = dsa::is_compact_dag(g);
    assert(res);
    assert(res.vertex_to_rank.size() == 1);
    assert(res.vertex_to_rank[0] == 0);
  }

  // Edge Case 3: Single vertex with self-loop (n = 1, 1 edge)
  {
    dsa::DirectedGraph g(1);
    g.add_edge(0, 0);
    auto res = dsa::is_compact_dag(g);
    assert(!res);
  }

  // Edge Case 4: Two vertices, compact (0 -> 1)
  {
    dsa::DirectedGraph g(2);
    g.add_edge(0, 1);
    auto res = dsa::is_compact_dag(g);
    assert(res);
    assert(res.vertex_to_rank[0] == 0);
    assert(res.vertex_to_rank[1] == 1);
    assert(dsa::CompactDAGDetector::verify_numbering(g, res.vertex_to_rank));
  }

  // Edge Case 5: Two vertices, compact reversed (1 -> 0)
  {
    dsa::DirectedGraph g(2);
    g.add_edge(1, 0);
    auto res = dsa::is_compact_dag(g);
    assert(res);
    assert(res.vertex_to_rank[1] == 0);
    assert(res.vertex_to_rank[0] == 1);
    assert(dsa::CompactDAGDetector::verify_numbering(g, res.vertex_to_rank));
  }

  // Edge Case 6: Two vertices, empty (0 edges, expected 1)
  {
    dsa::DirectedGraph g(2);
    auto res = dsa::is_compact_dag(g);
    assert(!res);
  }

  // Edge Case 7: Two vertices, 2-cycle (0 -> 1 and 1 -> 0)
  {
    dsa::DirectedGraph g(2);
    g.add_edge(0, 1);
    g.add_edge(1, 0);
    auto res = dsa::is_compact_dag(g);
    assert(!res);
  }

  std::cout << "    [+] Edge cases passed successfully.\n";
}

void test_boundary_and_structural_conditions() {
  std::cout << "[*] Running boundary and structural condition tests...\n";

  // Test 1: n = 3, directed 3-cycle (0 -> 1 -> 2 -> 0)
  // Has 3 edges (3 * 2 / 2 = 3), but all out-degrees are 1!
  {
    dsa::DirectedGraph g(3);
    g.add_edge(0, 1);
    g.add_edge(1, 2);
    g.add_edge(2, 0);
    auto res = dsa::is_compact_dag(g);
    assert(!res);
    assert(!brute_force_compact_oracle(g));
  }

  // Test 2: n = 3, transitive tournament (compact)
  // 0 -> 1, 1 -> 2, 0 -> 2
  {
    dsa::DirectedGraph g(3);
    g.add_edge(0, 1);
    g.add_edge(1, 2);
    g.add_edge(0, 2);
    auto res = dsa::is_compact_dag(g);
    assert(res);
    assert(brute_force_compact_oracle(g));
    assert(dsa::CompactDAGDetector::verify_numbering(g, res.vertex_to_rank));
  }

  // Test 3: Missing one edge (n = 4, expected 6 edges, given 5)
  {
    auto g = dsa::DirectedGraph::make_canonical_compact_dag(4);
    // g has 6 edges. Create g_missing with 5 edges
    dsa::DirectedGraph g_missing(4);
    g_missing.add_edge(0, 1);
    g_missing.add_edge(0, 2);
    g_missing.add_edge(0, 3);
    g_missing.add_edge(1, 2);
    g_missing.add_edge(1, 3);
    // Missing (2, 3)
    auto res = dsa::is_compact_dag(g_missing);
    assert(!res);
  }

  // Test 4: Extra edge (duplicate edge making |E| = 7 instead of 6)
  {
    auto g = dsa::DirectedGraph::make_canonical_compact_dag(4);
    g.add_edge(0, 1); // duplicate edge
    auto res = dsa::is_compact_dag(g);
    assert(!res);
  }

  // Test 5: Self-loop alongside other edges
  {
    dsa::DirectedGraph g(3);
    g.add_edge(0, 0); // self-loop
    g.add_edge(0, 1);
    g.add_edge(1, 2);
    auto res = dsa::is_compact_dag(g);
    assert(!res);
  }

  // Test 6: Duplicate edge while keeping edge count equal to n(n-1)/2
  // n = 4 (expected 6 edges). 0->1, 0->1 (dup), 0->2, 0->3, 1->2, 1->3
  // Out-degrees: vertex 0 has 4 edges!
  {
    dsa::DirectedGraph g(4);
    g.add_edge(0, 1);
    g.add_edge(0, 1);
    g.add_edge(0, 2);
    g.add_edge(0, 3);
    g.add_edge(1, 2);
    g.add_edge(1, 3);
    auto res = dsa::is_compact_dag(g);
    assert(!res);
  }

  std::cout << "    [+] Boundary and structural conditions passed.\n";
}

void test_subtle_counterexample() {
  std::cout << "[*] Running subtle counterexample test (degree sequence "
               "deception)...\n";

  // Critical Counterexample:
  // Consider n = 4 vertices. A compact graph requires 6 edges and out-degrees {3, 2, 1, 0}.
  // We can construct a graph with EXACTLY 6 edges and out-degrees {3, 2, 1, 0}, but
  // containing a 2-cycle between 0 and 1!
  // Edges:
  //   0 -> 1, 2, 3 (out-degree 3)
  //   1 -> 0, 3    (out-degree 2: notice 1 -> 0 creates a 2-cycle, missing 1 -> 2!)
  //   2 -> 3       (out-degree 1)
  //   3 -> none    (out-degree 0)
  // Out-degrees: 3, 2, 1, 0 (perfect permutation of 0..3).
  // Total edges: 6 = 4 * 3 / 2.
  // Is it compact? NO! It contains a 2-cycle (0, 1) and missing edge between (1, 2).
  // Our algorithm MUST reject this graph in Step 4.

  dsa::DirectedGraph g(4);
  g.add_edge(0, 1);
  g.add_edge(0, 2);
  g.add_edge(0, 3);
  g.add_edge(1, 0); // Backward edge violating topological rank
  g.add_edge(1, 3);
  g.add_edge(2, 3);

  assert(g.num_edges() == 6);
  assert(g.out_degree(0) == 3);
  assert(g.out_degree(1) == 2);
  assert(g.out_degree(2) == 1);
  assert(g.out_degree(3) == 0);

  auto res = dsa::is_compact_dag(g);
  assert(!res);
  assert(!brute_force_compact_oracle(g));

  std::cout
      << "    [+] Subtle counterexample correctly identified and rejected: "
      << res.reason << "\n";
}

void test_permutations_and_isomorphisms() {
  std::cout << "[*] Running arbitrary permutation and isomorphism tests...\n";

  std::mt19937_64 rng(1337);

  // Test across various vertex counts
  for (std::size_t n = 2; n <= 25; ++n) {
    std::vector<std::size_t> p(n);
    std::iota(p.begin(), p.end(), 0);

    // Shuffle to create an arbitrary bijective ranking
    std::shuffle(p.begin(), p.end(), rng);

    dsa::DirectedGraph g =
        dsa::DirectedGraph::make_compact_dag_from_permutation(p);
    auto res = dsa::is_compact_dag(g);

    assert(res);
    assert(res.is_compact);
    assert(res.vertex_to_rank.size() == n);
    assert(res.rank_to_vertex.size() == n);

    // Verify reconstructed ranking matches the true permutation
    for (std::size_t u = 0; u < n; ++u) {
      assert(res.vertex_to_rank[u] == p[u]);
      assert(res.rank_to_vertex[p[u]] == u);
    }

    // Verify against oracle
    assert(dsa::CompactDAGDetector::verify_numbering(g, res.vertex_to_rank));

    if (n <= 5) {
      assert(brute_force_compact_oracle(g));
    }

    // Perturb the graph by reversing an edge of distance 2 (creating a 3-cycle: u -> w -> v -> u)
    // to ensure negative detection
    if (n >= 3) {
      std::vector<std::pair<std::size_t, std::size_t>> perturbed_edges;
      bool reversed_cycle = false;
      for (std::size_t u = 0; u < n; ++u) {
        for (std::size_t v = 0; v < n; ++v) {
          if (p[u] < p[v]) {
            if (!reversed_cycle && p[u] + 2 == p[v]) {
              perturbed_edges.emplace_back(v, u); // reversed to create 3-cycle!
              reversed_cycle = true;
            } else {
              perturbed_edges.emplace_back(u, v);
            }
          }
        }
      }
      if (reversed_cycle) {
        auto perturbed_g =
            dsa::DirectedGraph::from_edge_list(n, perturbed_edges);
        auto perturbed_res = dsa::is_compact_dag(perturbed_g);
        assert(!perturbed_res);
      }
    }
  }

  std::cout << "    [+] Permutation and isomorphism tests passed.\n";
}

void test_alternative_representations() {
  std::cout << "[*] Running adjacency matrix and edge list overload tests...\n";

  // Test Adjacency Matrix
  {
    // n = 3 compact DAG: 0 -> 1, 0 -> 2, 1 -> 2
    std::vector<std::vector<bool>> matrix = {
        {false, true, true}, {false, false, true}, {false, false, false}};

    auto res = dsa::CompactDAGDetector::detect(matrix);
    assert(res);
    assert(res.vertex_to_rank[0] == 0);
    assert(res.vertex_to_rank[1] == 1);
    assert(res.vertex_to_rank[2] == 2);

    // Negative: add 2-cycle
    matrix[1][0] = true;
    auto res_cycle = dsa::CompactDAGDetector::detect(matrix);
    assert(!res_cycle);

    // Negative: self-loop
    matrix[1][0] = false;
    matrix[0][0] = true;
    auto res_loop = dsa::CompactDAGDetector::detect(matrix);
    assert(!res_loop);
  }

  // Test Raw Adjacency List
  {
    std::vector<std::vector<std::size_t>> adj = {
        {1, 2}, // vertex 0 -> 1, 2
        {2}, // vertex 1 -> 2
        {} // vertex 2 -> none
    };
    auto res = dsa::CompactDAGDetector::detect(adj);
    assert(res);
    assert(res.vertex_to_rank[0] == 0);
    assert(res.vertex_to_rank[1] == 1);
    assert(res.vertex_to_rank[2] == 2);
  }

  // Test Raw Edge List
  {
    std::vector<std::pair<std::size_t, std::size_t>> edges = {
        {2, 0},
        {2, 1},
        {1, 0}}; // vertex 2 has rank 0, 1 has rank 1, 0 has rank 2
    auto res = dsa::CompactDAGDetector::detect(3, edges);
    assert(res);
    assert(res.vertex_to_rank[2] == 0);
    assert(res.vertex_to_rank[1] == 1);
    assert(res.vertex_to_rank[0] == 2);
  }

  std::cout << "    [+] Alternative representation overloads passed.\n";
}

void test_performance_and_stress() {
  std::cout << "[*] Running performance and stress tests...\n";

  // Benchmark n = 500 (m = 124,750 edges)
  {
    const std::size_t n = 500;
    std::vector<std::size_t> p(n);
    std::iota(p.begin(), p.end(), 0);
    std::mt19937_64 rng(42);
    std::shuffle(p.begin(), p.end(), rng);

    auto start_build = std::chrono::high_resolution_clock::now();
    dsa::DirectedGraph g =
        dsa::DirectedGraph::make_compact_dag_from_permutation(p);
    auto end_build = std::chrono::high_resolution_clock::now();

    auto start_det = std::chrono::high_resolution_clock::now();
    auto res = dsa::is_compact_dag(g);
    auto end_det = std::chrono::high_resolution_clock::now();

    assert(res);
    assert(res.is_compact);

    const auto build_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              end_build - start_build)
                              .count();
    const auto det_ms = std::chrono::duration_cast<std::chrono::microseconds>(
                            end_det - start_det)
                            .count();

    std::cout << "    [+] n = " << n << " (" << g.num_edges() << " edges): "
              << "Graph build: " << build_ms << " ms, "
              << "Detection: " << det_ms << " us (" << (det_ms / 1000.0)
              << " ms)\n";
  }

  // Benchmark n = 1000 (m = 499,500 edges)
  {
    const std::size_t n = 1000;
    std::vector<std::size_t> p(n);
    std::iota(p.begin(), p.end(), 0);
    std::mt19937_64 rng(999);
    std::shuffle(p.begin(), p.end(), rng);

    auto start_build = std::chrono::high_resolution_clock::now();
    dsa::DirectedGraph g =
        dsa::DirectedGraph::make_compact_dag_from_permutation(p);
    auto end_build = std::chrono::high_resolution_clock::now();

    auto start_det = std::chrono::high_resolution_clock::now();
    auto res = dsa::is_compact_dag(g);
    auto end_det = std::chrono::high_resolution_clock::now();

    assert(res);
    assert(res.is_compact);

    const auto build_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              end_build - start_build)
                              .count();
    const auto det_ms = std::chrono::duration_cast<std::chrono::microseconds>(
                            end_det - start_det)
                            .count();

    std::cout << "    [+] n = " << n << " (" << g.num_edges() << " edges): "
              << "Graph build: " << build_ms << " ms, "
              << "Detection: " << det_ms << " us (" << (det_ms / 1000.0)
              << " ms)\n";
  }

  std::cout << "    [+] Stress and performance tests completed successfully.\n";
}

} // namespace

int main() {
  std::cout << "========================================================\n";
  std::cout << "  C-13.1: Compact Directed Acyclic Graph (DAG) Detector \n";
  std::cout << "========================================================\n\n";

  test_edge_cases();
  test_boundary_and_structural_conditions();
  test_subtle_counterexample();
  test_permutations_and_isomorphisms();
  test_alternative_representations();
  test_performance_and_stress();

  std::cout << "\n========================================================\n";
  std::cout << "  All verification suites passed successfully!          \n";
  std::cout << "========================================================\n";

  return 0;
}
