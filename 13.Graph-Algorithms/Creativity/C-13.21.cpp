// File : C-13.21.cpp

/*
    C-13.21 NASA wants to link n stations spread over the country using communica-
    tion channels. Each pair of stations has a different bandwidth available,
    which is known a priori. NASA wants to select n - 1 channels (the mini-
    mum possible) in such a way that all the stations are linked by the channels
    and the total bandwidth (defined as the sum of the individual bandwidths
    of the channels) is maximum. Give an efficient algorithm for this prob-
    lem and determine its worst-case time complexity. Consider the weighted
    graph G = (V, E), where V is the set of stations and E is the set of chan-
    nels between the stations. Define the weight w(e) of an edge e in E as the
    bandwidth of the corresponding channel
*/

#include "maximum_spanning_tree.hpp"

#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <random>
#include <vector>

namespace {

/**
 * @brief Helper to verify that a computed spanning tree is structurally valid:
 * 1. Contains exactly n - 1 edges (for n >= 1).
 * 2. Forms an acyclic connected graph linking all n stations.
 * 3. Every edge belongs to the original network with matching bandwidth.
 * 4. Sum of edge bandwidths matches reported total_bandwidth.
 */
template <typename Weight>
void verify_spanning_tree_validity(
    const dsa::station_network<Weight> &net,
    const dsa::spanning_tree_result<Weight> &res) {
  const std::size_t n = net.num_stations();
  if (n <= 1) {
    assert(res.is_connected);
    assert(res.tree_edges.empty());
    assert(res.total_bandwidth == Weight{0});
    return;
  }

  if (!res.is_connected) {
    assert(res.tree_edges.size() < n - 1);
    return;
  }

  assert(res.tree_edges.size() == n - 1 &&
         "Spanning tree on n vertices must have exactly n - 1 edges");

  dsa::disjoint_set dsu(n);
  Weight sum_bw{0};

  for (const auto &edge : res.tree_edges) {
    assert(edge.u < n && edge.v < n);
    assert(edge.u != edge.v);

    // Verify channel exists in network with same bandwidth
    bool found = false;
    for (const auto &nbr : net.adjacent(edge.u)) {
      if (nbr.to == edge.v && std::abs(nbr.bandwidth - edge.bandwidth) < 1e-9) {
        found = true;
        break;
      }
    }
    assert(found && "Selected channel does not exist in the network!");

    // Verify acyclic property using DSU
    const bool merged = dsu.union_sets(edge.u, edge.v);
    assert(merged && "Spanning tree contains a cycle!");

    sum_bw += edge.bandwidth;
  }

  assert(dsu.count() == 1 && "Spanning tree does not link all stations!");
  assert(std::abs(sum_bw - res.total_bandwidth) < 1e-9 &&
         "Reported total bandwidth does not match sum of channel bandwidths!");
}

/**
 * @brief Independent brute-force oracle for small n.
 * Enumerates all subsets of size n - 1 from all available channels, tests whether
 * the subset links all n stations without cycles, and finds the true maximum bandwidth sum.
 */
template <typename Weight>
std::optional<Weight>
brute_force_max_st_oracle(const dsa::station_network<Weight> &net) {
  const std::size_t n = net.num_stations();
  if (n <= 1) {
    return Weight{0};
  }

  const auto &channels = net.all_channels();
  const std::size_t m = channels.size();
  if (m < n - 1) {
    return std::nullopt;
  }

  std::optional<Weight> max_bw = std::nullopt;

  // Combinations generator of size k = n - 1 from m elements
  std::vector<std::size_t> indices(n - 1);
  std::iota(indices.begin(), indices.end(), 0);

  auto check_combination = [&]() {
    dsa::disjoint_set dsu(n);
    Weight cur_sum{0};
    bool valid_tree = true;

    for (std::size_t idx : indices) {
      const auto &ch = channels[idx];
      if (!dsu.union_sets(ch.u, ch.v)) {
        valid_tree = false;
        break;
      }
      cur_sum += ch.bandwidth;
    }

    if (valid_tree && dsu.count() == 1) {
      if (!max_bw.has_value() || cur_sum > *max_bw) {
        max_bw = cur_sum;
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
  return max_bw;
}

/**
 * @brief Canonical NASA stations network test.
 *
 * 4 NASA tracking stations (Goldstone=0, Madrid=1, Canberra=2, Houston=3).
 * Channels with varying bandwidths:
 *   (0, 1) = 100 Gbps, (1, 2) = 150 Gbps, (2, 3) = 120 Gbps,
 *   (0, 3) = 80 Gbps,  (0, 2) = 200 Gbps, (1, 3) = 90 Gbps.
 *
 * MaxST must pick (0, 2: 200), (1, 2: 150), (2, 3: 120) with total = 470 Gbps.
 * MinST picks (0, 3: 80), (1, 3: 90), (0, 1: 100) with total = 270 Gbps.
 */
void test_canonical_nasa_network() {
  std::cout << "[RUN] test_canonical_nasa_network" << std::endl;
  dsa::station_network<double> net(4);

  net.add_channel(0, 1, 100.0);
  net.add_channel(1, 2, 150.0);
  net.add_channel(2, 3, 120.0);
  net.add_channel(0, 3, 80.0);
  net.add_channel(0, 2, 200.0);
  net.add_channel(1, 3, 90.0);

  const auto max_kruskal = dsa::maximum_spanning_tree_kruskal(net);
  const auto max_prim = dsa::maximum_spanning_tree_prim(net, 0);
  const auto min_kruskal = dsa::minimum_spanning_tree_kruskal(net);
  const auto oracle = brute_force_max_st_oracle(net);

  assert(max_kruskal.is_connected);
  assert(max_prim.is_connected);
  assert(oracle.has_value());

  assert(std::abs(max_kruskal.total_bandwidth - 470.0) < 1e-9);
  assert(std::abs(max_prim.total_bandwidth - 470.0) < 1e-9);
  assert(std::abs(*oracle - 470.0) < 1e-9);

  assert(std::abs(min_kruskal.total_bandwidth - 290.0) < 1e-9);
  assert(max_kruskal.total_bandwidth > min_kruskal.total_bandwidth);

  verify_spanning_tree_validity(net, max_kruskal);
  verify_spanning_tree_validity(net, max_prim);
  verify_spanning_tree_validity(net, min_kruskal);

  std::cout << "[PASS] test_canonical_nasa_network (Max bandwidth = 470 Gbps, "
               "Min = 290 Gbps)"
            << std::endl;
}

/**
 * @brief Tests edge cases: 0 stations, 1 station, 2 stations.
 */
void test_small_and_boundary_networks() {
  std::cout << "[RUN] test_small_and_boundary_networks" << std::endl;

  // 0 stations
  dsa::station_network<int> net0(0);
  const auto res0_k = dsa::maximum_spanning_tree_kruskal(net0);
  const auto res0_p = dsa::maximum_spanning_tree_prim(net0);
  assert(res0_k.is_connected && res0_k.tree_edges.empty());
  assert(res0_p.is_connected && res0_p.tree_edges.empty());

  // 1 station
  dsa::station_network<int> net1(1);
  const auto res1_k = dsa::maximum_spanning_tree_kruskal(net1);
  const auto res1_p = dsa::maximum_spanning_tree_prim(net1, 0);
  assert(res1_k.is_connected && res1_k.tree_edges.empty());
  assert(res1_p.is_connected && res1_p.tree_edges.empty());

  // 2 stations, 1 channel
  dsa::station_network<int> net2(2);
  net2.add_channel(0, 1, 500);
  const auto res2_k = dsa::maximum_spanning_tree_kruskal(net2);
  const auto res2_p = dsa::maximum_spanning_tree_prim(net2, 0);
  assert(res2_k.is_connected && res2_k.total_bandwidth == 500);
  assert(res2_p.is_connected && res2_p.total_bandwidth == 500);
  assert(res2_k.tree_edges.size() == 1);
  verify_spanning_tree_validity(net2, res2_k);
  verify_spanning_tree_validity(net2, res2_p);

  std::cout << "[PASS] test_small_and_boundary_networks" << std::endl;
}

/**
 * @brief Tests disconnected networks where all stations cannot be linked.
 */
void test_disconnected_network() {
  std::cout << "[RUN] test_disconnected_network" << std::endl;

  dsa::station_network<int64_t> net(5);
  // Component 1: {0, 1}
  net.add_channel(0, 1, 100);
  // Component 2: {2, 3, 4}
  net.add_channel(2, 3, 200);
  net.add_channel(3, 4, 300);

  const auto res_k = dsa::maximum_spanning_tree_kruskal(net);
  const auto res_p = dsa::maximum_spanning_tree_prim(net, 0);
  const auto oracle = brute_force_max_st_oracle(net);

  assert(!net.is_connected());
  assert(!res_k.is_connected);
  assert(!res_p.is_connected);
  assert(!oracle.has_value());

  // Connecting the bridge
  net.add_channel(1, 2, 50);
  assert(net.is_connected());
  const auto res_conn_k = dsa::maximum_spanning_tree_kruskal(net);
  const auto res_conn_p = dsa::maximum_spanning_tree_prim(net, 0);
  assert(res_conn_k.is_connected);
  assert(res_conn_p.is_connected);
  assert(res_conn_k.total_bandwidth == 100 + 200 + 300 + 50);
  assert(res_conn_p.total_bandwidth == res_conn_k.total_bandwidth);
  verify_spanning_tree_validity(net, res_conn_k);
  verify_spanning_tree_validity(net, res_conn_p);

  std::cout << "[PASS] test_disconnected_network" << std::endl;
}

/**
 * @brief Tests duplicate parallel channels with different bandwidths.
 * Algorithm must greedily select the higher-bandwidth channel.
 */
void test_multigraph_parallel_channels() {
  std::cout << "[RUN] test_multigraph_parallel_channels" << std::endl;

  dsa::station_network<int> net(3);
  // Two channels between 0 and 1: 50 Mbps and 500 Mbps
  net.add_channel(0, 1, 50);
  net.add_channel(0, 1, 500);

  // Two channels between 1 and 2: 30 Mbps and 300 Mbps
  net.add_channel(1, 2, 30);
  net.add_channel(1, 2, 300);

  const auto res_k = dsa::maximum_spanning_tree_kruskal(net);
  const auto res_p = dsa::maximum_spanning_tree_prim(net, 0);
  const auto oracle = brute_force_max_st_oracle(net);

  assert(res_k.is_connected);
  assert(res_p.is_connected);
  assert(oracle.has_value());
  assert(res_k.total_bandwidth == 800); // 500 + 300
  assert(res_p.total_bandwidth == 800);
  assert(*oracle == 800);

  verify_spanning_tree_validity(net, res_k);
  verify_spanning_tree_validity(net, res_p);

  std::cout << "[PASS] test_multigraph_parallel_channels" << std::endl;
}

/**
 * @brief Tests invalid inputs and exception handling.
 */
void test_invalid_inputs_and_exceptions() {
  std::cout << "[RUN] test_invalid_inputs_and_exceptions" << std::endl;

  dsa::station_network<double> net(3);

  // Negative bandwidth
  bool caught_negative = false;
  try {
    net.add_channel(0, 1, -10.0);
  } catch (const std::invalid_argument &) {
    caught_negative = true;
  }
  assert(caught_negative);

  // Self loop
  bool caught_self_channel = false;
  try {
    net.add_channel(1, 1, 50.0);
  } catch (const std::invalid_argument &) {
    caught_self_channel = true;
  }
  assert(caught_self_channel);

  // Out of bounds station
  bool caught_oob = false;
  try {
    net.add_channel(0, 5, 20.0);
  } catch (const std::out_of_range &) {
    caught_oob = true;
  }
  assert(caught_oob);

  // Out of bounds Prim start
  bool caught_prim_start = false;
  try {
    [[maybe_unused]] auto res = dsa::maximum_spanning_tree_prim(net, 10);
  } catch (const std::out_of_range &) {
    caught_prim_start = true;
  }
  assert(caught_prim_start);

  std::cout << "[PASS] test_invalid_inputs_and_exceptions" << std::endl;
}

/**
 * @brief Randomized stress test comparing Kruskal, Prim, and Brute-Force oracle.
 */
void test_randomized_stress() {
  std::cout << "[RUN] test_randomized_stress (50 random networks)" << std::endl;
  std::mt19937_64 rng(1321); // Fixed seed

  const std::size_t num_tests = 50;

  for (std::size_t t = 0; t < num_tests; ++t) {
    std::uniform_int_distribution<std::size_t> n_dist(3, 6);
    const std::size_t n = n_dist(rng);

    dsa::station_network<int64_t> net(n);

    // Guarantee connectivity by building a spanning backbone
    std::uniform_int_distribution<int64_t> bw_dist(10, 1000);
    for (std::size_t i = 1; i < n; ++i) {
      std::uniform_int_distribution<std::size_t> p_dist(0, i - 1);
      net.add_channel(i, p_dist(rng), bw_dist(rng));
    }

    // Add extra random channels
    std::uniform_int_distribution<std::size_t> extra_dist(1, n * 2);
    const std::size_t extra_m = extra_dist(rng);
    std::uniform_int_distribution<std::size_t> v_dist(0, n - 1);

    for (std::size_t e = 0; e < extra_m; ++e) {
      std::size_t u = v_dist(rng);
      std::size_t v = v_dist(rng);
      if (u != v) {
        net.add_channel(u, v, bw_dist(rng));
      }
    }

    const auto res_k = dsa::maximum_spanning_tree_kruskal(net);
    const auto res_p = dsa::maximum_spanning_tree_prim(net, 0);
    const auto oracle = brute_force_max_st_oracle(net);

    assert(res_k.is_connected && res_p.is_connected);
    assert(oracle.has_value());
    assert(res_k.total_bandwidth == res_p.total_bandwidth);
    assert(res_k.total_bandwidth == *oracle);

    verify_spanning_tree_validity(net, res_k);
    verify_spanning_tree_validity(net, res_p);
  }

  std::cout << "[PASS] test_randomized_stress (50 random networks verified)"
            << std::endl;
}

/**
 * @brief Performance sanity check on a large-scale network confirming O(m log n) efficiency.
 */
void test_performance_sanity() {
  std::cout << "[RUN] test_performance_sanity (N = 10,000 stations, M = 50,000 "
               "channels)"
            << std::endl;

  const std::size_t n = 10000;
  const std::size_t m = 50000;

  dsa::station_network<double> net(n);
  std::mt19937_64 rng(42);
  std::uniform_real_distribution<double> bw_dist(1.0, 10000.0);
  std::uniform_int_distribution<std::size_t> v_dist(0, n - 1);

  // Spanning chain
  for (std::size_t i = 0; i + 1 < n; ++i) {
    net.add_channel(i, i + 1, bw_dist(rng));
  }

  // Extra channels
  for (std::size_t i = n - 1; i < m; ++i) {
    std::size_t u = v_dist(rng);
    std::size_t v = v_dist(rng);
    if (u != v) {
      net.add_channel(u, v, bw_dist(rng));
    }
  }

  const auto t0 = std::chrono::steady_clock::now();
  const auto res_k = dsa::maximum_spanning_tree_kruskal(net);
  const auto t1 = std::chrono::steady_clock::now();
  const auto res_p = dsa::maximum_spanning_tree_prim(net, 0);
  const auto t2 = std::chrono::steady_clock::now();

  const auto kruskal_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
  const auto prim_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

  assert(res_k.is_connected && res_p.is_connected);
  assert(std::abs(res_k.total_bandwidth - res_p.total_bandwidth) < 1e-4);

  verify_spanning_tree_validity(net, res_k);
  verify_spanning_tree_validity(net, res_p);

  std::cout << "  Kruskal MaxST time: " << kruskal_ms << " ms\n";
  std::cout << "  Prim    MaxST time: " << prim_ms << " ms\n";
  std::cout << "  Total Max Bandwidth: " << res_k.total_bandwidth << "\n";
  std::cout << "[PASS] test_performance_sanity" << std::endl;
}

} // namespace

int main() {
  std::cout << "========================================================\n";
  std::cout << " C-13.21: NASA Max Bandwidth Spanning Tree (MaxST)\n";
  std::cout << "========================================================\n"
            << std::endl;

  test_canonical_nasa_network();
  test_small_and_boundary_networks();
  test_disconnected_network();
  test_multigraph_parallel_channels();
  test_invalid_inputs_and_exceptions();
  test_randomized_stress();
  test_performance_sanity();

  std::cout << "\nAll test suites passed successfully!" << std::endl;
  return 0;
}
