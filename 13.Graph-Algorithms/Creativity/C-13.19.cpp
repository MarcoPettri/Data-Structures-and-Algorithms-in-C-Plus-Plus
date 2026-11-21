// File : C-13.19.cpp

/*
    C-13.19 Consider a diagram of a telephone network, which is a graph G whose
    vertices represent switching centers and whose edges represent communi-
    cation lines joining pairs of centers. Edges are marked by their bandwidth,
    and the bandwidth of a path is the bandwidth of its lowest bandwidth edge.
    Give an algorithm that, given a diagram and two switching centers a and
    b, outputs the maximum bandwidth of a path between a and b.
*/

#include "max_bandwidth_path.hpp"

#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace {

/**
 * @brief Helper to verify that a reconstructed path is structurally valid in the network
 *        and that its bottleneck exactly matches the reported maximum bandwidth.
 */
template <typename Weight>
void verify_path_validity(const dsa::telephone_network<Weight> &net,
                          std::size_t src, std::size_t dst,
                          const dsa::bandwidth_result<Weight> &res) {
  if (src == dst) {
    assert(res.has_path());
    assert(res.path.size() == 1 && res.path[0] == src);
    return;
  }

  if (!res.has_path()) {
    assert(res.path.empty());
    return;
  }

  assert(!res.path.empty());
  assert(res.path.front() == src);
  assert(res.path.back() == dst);

  // Traverse each hop along the path and calculate the path's bottleneck
  Weight path_bottleneck = std::numeric_limits<Weight>::max();

  for (std::size_t i = 0; i + 1 < res.path.size(); ++i) {
    const std::size_t u = res.path[i];
    const std::size_t v = res.path[i + 1];

    bool edge_found = false;
    Weight max_edge_bw = Weight{0};

    for (const auto &line : net.adjacent(u)) {
      if (line.to == v) {
        edge_found = true;
        max_edge_bw = std::max(max_edge_bw, line.bandwidth);
      }
    }

    assert(edge_found && "Path uses a non-existent communication line!");
    path_bottleneck = std::min(path_bottleneck, max_edge_bw);
  }

  assert(
      std::abs(path_bottleneck - *res.max_bandwidth) < 1e-9 &&
      "Reconstructed path's bottleneck does not match reported max bandwidth!");
}

/**
 * @brief Tests canonical telephone network topology with competing paths:
 *
 * Path 1: direct line with low bandwidth (hop count 1, bw = 10).
 * Path 2: 3-hop alternate route through high-capacity trunks (bw = 100, 80, 90; bottleneck = 80).
 * Optimal path between a=0 and b=3 must select the 80 bottleneck route over the 10 direct line.
 */
void test_competing_paths() {
  using namespace dsa;
  std::cout << "[RUN] test_competing_paths\n";

  telephone_network<double> net(4);
  // Direct low-bandwidth line
  net.add_line(0, 3, 10.0);
  // High-capacity 3-hop detour: 0 --(100)--> 1 --(80)--> 2 --(90)--> 3
  net.add_line(0, 1, 100.0);
  net.add_line(1, 2, 80.0);
  net.add_line(2, 3, 90.0);

  const auto res_dj = max_bandwidth_dijkstra(net, 0, 3);
  const auto res_kr = max_bandwidth_kruskal(net, 0, 3);
  const auto oracle = max_bandwidth_binary_search(net, 0, 3);

  assert(res_dj.has_path());
  assert(res_kr.has_path());
  assert(oracle.has_value());

  assert(*res_dj.max_bandwidth == 80.0);
  assert(*res_kr.max_bandwidth == 80.0);
  assert(*oracle == 80.0);

  verify_path_validity(net, 0, 3, res_dj);
  verify_path_validity(net, 0, 3, res_kr);

  std::cout << "[PASS] test_competing_paths (Max bandwidth = 80.0)\n";
}

/**
 * @brief Tests edge cases: single center, same source and destination, and empty networks.
 */
void test_single_and_same_center() {
  using namespace dsa;
  std::cout << "[RUN] test_single_and_same_center\n";

  telephone_network<int64_t> net(1);
  const auto res_dj = max_bandwidth_dijkstra(net, 0, 0);
  const auto res_kr = max_bandwidth_kruskal(net, 0, 0);
  const auto oracle = max_bandwidth_binary_search(net, 0, 0);

  assert(res_dj.has_path());
  assert(res_kr.has_path());
  assert(oracle.has_value());
  assert(res_dj.path.size() == 1 && res_dj.path[0] == 0);
  assert(res_kr.path.size() == 1 && res_kr.path[0] == 0);

  // Network with multiple centers, query where src == dst
  telephone_network<int64_t> net5(5);
  net5.add_line(0, 1, 50);
  const auto res_same = max_bandwidth_dijkstra(net5, 2, 2);
  assert(res_same.has_path());
  assert(res_same.path.size() == 1 && res_same.path[0] == 2);

  std::cout << "[PASS] test_single_and_same_center\n";
}

/**
 * @brief Tests disconnected centers and separate network components.
 */
void test_disconnected_centers() {
  using namespace dsa;
  std::cout << "[RUN] test_disconnected_centers\n";

  telephone_network<double> net(6);
  // Component 1: {0, 1, 2}
  net.add_line(0, 1, 50.0);
  net.add_line(1, 2, 40.0);

  // Component 2: {3, 4, 5}
  net.add_line(3, 4, 100.0);
  net.add_line(4, 5, 80.0);

  // Query across disconnected components
  const auto res_dj = max_bandwidth_dijkstra(net, 0, 4);
  const auto res_kr = max_bandwidth_kruskal(net, 0, 4);
  const auto oracle = max_bandwidth_binary_search(net, 0, 4);

  assert(!res_dj.has_path());
  assert(!res_kr.has_path());
  assert(!oracle.has_value());
  assert(res_dj.path.empty());
  assert(res_kr.path.empty());

  // Connecting the bridge
  net.add_line(2, 3, 25.0);
  const auto res_connected = max_bandwidth_dijkstra(net, 0, 5);
  const auto oracle_connected = max_bandwidth_binary_search(net, 0, 5);
  assert(res_connected.has_path());
  assert(*res_connected.max_bandwidth ==
         25.0); // 25.0 is bottleneck of the bridge
  assert(oracle_connected.has_value() && *oracle_connected == 25.0);
  verify_path_validity(net, 0, 5, res_connected);

  std::cout << "[PASS] test_disconnected_centers\n";
}

/**
 * @brief Tests duplicate parallel lines between the same centers with different bandwidths,
 *        and verifies self-loops.
 */
void test_multigraph_and_self_loops() {
  using namespace dsa;
  std::cout << "[RUN] test_multigraph_and_self_loops\n";

  telephone_network<int> net(3);
  // Two parallel lines between 0 and 1: 10 Mbps and 100 Mbps
  net.add_line(0, 1, 10);
  net.add_line(0, 1, 100);

  // Self loop on 1
  net.add_line(1, 1, 999);

  // Line between 1 and 2
  net.add_line(1, 2, 50);

  const auto res_dj = max_bandwidth_dijkstra(net, 0, 2);
  const auto res_kr = max_bandwidth_kruskal(net, 0, 2);
  const auto oracle = max_bandwidth_binary_search(net, 0, 2);

  // The bottleneck must pick the 100 Mbps line, giving min(100, 50) = 50.
  assert(res_dj.has_path() && *res_dj.max_bandwidth == 50);
  assert(res_kr.has_path() && *res_kr.max_bandwidth == 50);
  assert(oracle.has_value() && *oracle == 50);

  verify_path_validity(net, 0, 2, res_dj);
  verify_path_validity(net, 0, 2, res_kr);

  std::cout << "[PASS] test_multigraph_and_self_loops\n";
}

/**
 * @brief Tests invalid inputs, out-of-range center indices, and negative bandwidths.
 */
void test_invalid_inputs_and_exceptions() {
  using namespace dsa;
  std::cout << "[RUN] test_invalid_inputs_and_exceptions\n";

  telephone_network<double> net(3);
  net.add_line(0, 1, 10.0);

  // Negative bandwidth must throw std::invalid_argument
  bool caught_negative = false;
  try {
    net.add_line(1, 2, -5.0);
  } catch (const std::invalid_argument &) {
    caught_negative = true;
  }
  assert(caught_negative);

  // Out of range vertex in add_line
  bool caught_out_of_range_add = false;
  try {
    net.add_line(1, 5, 10.0);
  } catch (const std::out_of_range &) {
    caught_out_of_range_add = true;
  }
  assert(caught_out_of_range_add);

  // Out of range queries in Dijkstra and Kruskal
  bool caught_dijkstra = false;
  try {
    [[maybe_unused]] auto res = max_bandwidth_dijkstra(net, 0, 10);
  } catch (const std::out_of_range &) {
    caught_dijkstra = true;
  }
  assert(caught_dijkstra);

  bool caught_kruskal = false;
  try {
    [[maybe_unused]] auto res = max_bandwidth_kruskal(net, 10, 0);
  } catch (const std::out_of_range &) {
    caught_kruskal = true;
  }
  assert(caught_kruskal);

  std::cout << "[PASS] test_invalid_inputs_and_exceptions\n";
}

/**
 * @brief Tests directed network paths (simplex lines).
 */
void test_directed_network() {
  using namespace dsa;
  std::cout << "[RUN] test_directed_network\n";

  telephone_network<int> net(4);
  // Directed path: 0 -> 1 -> 2 with bandwidths 40, 30
  net.add_line(0, 1, 40, /*bidirectional=*/false);
  net.add_line(1, 2, 30, /*bidirectional=*/false);

  // Dijkstra correctly handles directed graphs
  const auto res_forward = max_bandwidth_dijkstra(net, 0, 2);
  assert(res_forward.has_path() && *res_forward.max_bandwidth == 30);
  verify_path_validity(net, 0, 2, res_forward);

  // Reverse path should not exist
  const auto res_reverse = max_bandwidth_dijkstra(net, 2, 0);
  assert(!res_reverse.has_path());

  std::cout << "[PASS] test_directed_network\n";
}

/**
 * @brief Randomized stress test comparing Dijkstra, Kruskal, and Binary Search oracle
 *        over numerous random graphs with fixed seed for determinism and reproducibility.
 */
void test_randomized_stress() {
  using namespace dsa;
  std::cout << "[RUN] test_randomized_stress (50 random networks)\n";

  std::mt19937_64 rng(1319); // Fixed seed for reproducibility

  const std::size_t num_tests = 50;

  for (std::size_t t = 0; t < num_tests; ++t) {
    std::uniform_int_distribution<std::size_t> n_dist(2, 30);
    const std::size_t n = n_dist(rng);

    telephone_network<int64_t> net(n);

    std::uniform_int_distribution<std::size_t> m_dist(n - 1, n * 3);
    const std::size_t m = m_dist(rng);

    std::uniform_int_distribution<std::size_t> v_dist(0, n - 1);
    std::uniform_int_distribution<int64_t> bw_dist(1, 1000);

    for (std::size_t i = 0; i < m; ++i) {
      std::size_t u = v_dist(rng);
      std::size_t v = v_dist(rng);
      int64_t bw = bw_dist(rng);
      net.add_line(u, v, bw, /*bidirectional=*/true);
    }

    // Pick 5 random pairs of (a, b) to query
    for (int q = 0; q < 5; ++q) {
      std::size_t a = v_dist(rng);
      std::size_t b = v_dist(rng);

      const auto res_dj = max_bandwidth_dijkstra(net, a, b);
      const auto res_kr = max_bandwidth_kruskal(net, a, b);
      const auto oracle = max_bandwidth_binary_search(net, a, b);

      // Verify reachability agreement
      assert(res_dj.has_path() == res_kr.has_path());
      assert(res_dj.has_path() == oracle.has_value());

      if (res_dj.has_path()) {
        assert(*res_dj.max_bandwidth == *res_kr.max_bandwidth);
        assert(*res_dj.max_bandwidth == *oracle);

        verify_path_validity(net, a, b, res_dj);
        verify_path_validity(net, a, b, res_kr);
      }
    }
  }

  std::cout << "[PASS] test_randomized_stress (all 50 instances verified)\n";
}

/**
 * @brief Performance sanity check on a large-scale telephone network.
 *        Confirms efficiency and complexity bounds.
 */
void test_performance_sanity() {
  using namespace dsa;
  std::cout << "[RUN] test_performance_sanity (N = 10,000 centers, M = 40,000 "
               "lines)\n";

  const std::size_t n = 10000;
  const std::size_t m = 40000;

  telephone_network<double> net(n);
  std::mt19937_64 rng(42);
  std::uniform_int_distribution<std::size_t> v_dist(0, n - 1);
  std::uniform_real_distribution<double> bw_dist(1.0, 10000.0);

  // Guarantee connectivity via a backbone chain
  for (std::size_t i = 0; i + 1 < n; ++i) {
    net.add_line(i, i + 1, bw_dist(rng));
  }

  // Add random cross-trunk lines
  for (std::size_t i = n - 1; i < m; ++i) {
    net.add_line(v_dist(rng), v_dist(rng), bw_dist(rng));
  }

  const std::size_t src = 0;
  const std::size_t dst = n - 1;

  const auto t0 = std::chrono::steady_clock::now();
  const auto res_dj = max_bandwidth_dijkstra(net, src, dst);
  const auto t1 = std::chrono::steady_clock::now();
  const auto res_kr = max_bandwidth_kruskal(net, src, dst);
  const auto t2 = std::chrono::steady_clock::now();

  const auto time_dj_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
  const auto time_kr_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

  assert(res_dj.has_path() && res_kr.has_path());
  assert(std::abs(*res_dj.max_bandwidth - *res_kr.max_bandwidth) < 1e-6);
  verify_path_validity(net, src, dst, res_dj);
  verify_path_validity(net, src, dst, res_kr);

  std::cout << "  Dijkstra runtime: " << time_dj_ms << " ms\n";
  std::cout << "  Kruskal  runtime: " << time_kr_ms << " ms\n";
  std::cout << "  Optimal Max Bandwidth: " << *res_dj.max_bandwidth << "\n";
  std::cout << "  Dijkstra Path Hops:    " << res_dj.hop_count() << "\n";
  std::cout << "[PASS] test_performance_sanity\n";
}

} // namespace

int main() {
  std::cout << "========================================================\n";
  std::cout << " C-13.19: Maximum Bandwidth Path in Telephone Networks\n";
  std::cout << "========================================================\n\n";

  test_competing_paths();
  test_single_and_same_center();
  test_disconnected_centers();
  test_multigraph_and_self_loops();
  test_invalid_inputs_and_exceptions();
  test_directed_network();
  test_randomized_stress();
  test_performance_sanity();

  std::cout << "\nAll test suites passed successfully!\n";
  return 0;
}
