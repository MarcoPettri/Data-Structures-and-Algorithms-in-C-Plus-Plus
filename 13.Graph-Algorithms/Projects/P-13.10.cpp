// File : P-13.10.cpp

/*
    P-13.10 Implement Kruskal's algorithm assuming that the edge weights are
    integers.
*/

#include "kruskal_integer.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

int g_checks = 0;
int g_failures = 0;

#define CHECK(condition)                                                       \
  do {                                                                         \
    ++g_checks;                                                                \
    if (!(condition)) {                                                        \
      ++g_failures;                                                            \
      std::cerr << __FILE__ << ':' << __LINE__                                 \
                << ": CHECK failed: " #condition "\n";                         \
    }                                                                          \
  } while (false)

#define RUN(test)                                                              \
  do {                                                                         \
    std::cout << "[*] Running " #test "...\n";                                 \
    const int failures_before = g_failures;                                    \
    test();                                                                    \
    std::cout << (g_failures == failures_before ? "[+] " : "[!] ") << #test    \
              << (g_failures == failures_before ? " passed.\n"                 \
                                                : " had failures.\n");         \
  } while (false)

using dsa::kruskal_edge;
using dsa::kruskal_mst;

struct reference_result {
  bool is_connected;
  std::int64_t total_weight;
  std::size_t edge_count;
};

reference_result reference_kruskal(std::size_t vertex_count,
                                   std::vector<kruskal_edge> edges) {
  std::sort(edges.begin(), edges.end(),
            [](const kruskal_edge &lhs, const kruskal_edge &rhs) {
              return lhs.weight < rhs.weight;
            });
  std::vector<std::size_t> parent(vertex_count);
  std::vector<std::size_t> size(vertex_count, 1);
  for (std::size_t i = 0; i < vertex_count; ++i) {
    parent[i] = i;
  }
  const auto find_root = [&parent](std::size_t value) {
    std::size_t root = value;
    while (root != parent[root]) {
      root = parent[root];
    }
    while (value != root) {
      const std::size_t next = parent[value];
      parent[value] = root;
      value = next;
    }
    return root;
  };

  std::size_t selected = 0;
  std::int64_t total = 0;
  for (const kruskal_edge &edge : edges) {
    std::size_t lhs = find_root(edge.u);
    std::size_t rhs = find_root(edge.v);
    if (lhs == rhs) {
      continue;
    }
    if (size[lhs] < size[rhs]) {
      std::swap(lhs, rhs);
    }
    parent[rhs] = lhs;
    size[lhs] += size[rhs];
    total += edge.weight;
    ++selected;
    if (vertex_count > 0 && selected == vertex_count - 1) {
      break;
    }
  }
  return {vertex_count == 0 || selected == vertex_count - 1, total, selected};
}

void test_examples_and_edge_cases() {
  const auto empty = kruskal_mst(0, {});
  CHECK(empty.is_connected);
  CHECK(empty.tree_edges.empty());
  CHECK(empty.total_weight == 0);

  const auto singleton = kruskal_mst(1, {{0, 0, 7}});
  CHECK(singleton.is_connected);
  CHECK(singleton.tree_edges.empty());
  CHECK(singleton.total_weight == 0);

  const std::vector<kruskal_edge> edges{{0, 1, 4}, {0, 2, 1}, {1, 2, 2},
                                        {1, 3, 5}, {2, 3, 8}, {0, 3, 9}};
  const auto result = kruskal_mst(4, edges);
  CHECK(result.is_connected);
  CHECK(result.tree_edges.size() == 3);
  CHECK(result.total_weight == 8);

  const auto disconnected = kruskal_mst(4, {{0, 1, 3}, {2, 3, 5}});
  CHECK(!disconnected.is_connected);
  CHECK(disconnected.tree_edges.size() == 2);
  CHECK(disconnected.total_weight == 8);
}

void test_negative_duplicate_and_boundary_weights() {
  const std::vector<kruskal_edge> edges{
      {0, 1, 3}, {0, 1, -4}, {1, 2, -2}, {0, 2, 0}, {2, 2, -100}};
  const auto result = kruskal_mst(3, edges);
  CHECK(result.is_connected);
  CHECK(result.tree_edges.size() == 2);
  CHECK(result.total_weight == -6);

  const std::int64_t low = std::numeric_limits<std::int64_t>::min();
  const std::int64_t high = std::numeric_limits<std::int64_t>::max();
  const auto extremes = kruskal_mst(3, {{0, 1, low}, {1, 2, high}, {0, 2, 0}});
  CHECK(extremes.is_connected);
  CHECK(extremes.total_weight == low);

  const auto canceling_prefix =
      kruskal_mst(4, {{0, 1, low}, {1, 2, -10}, {2, 3, 20}});
  CHECK(canceling_prefix.is_connected);
  CHECK(canceling_prefix.total_weight == low + 10);
}

void test_invalid_endpoints_and_weight_overflow() {
  bool endpoint_threw = false;
  try {
    (void)kruskal_mst(2, {{0, 2, 1}});
  } catch (const std::out_of_range &) {
    endpoint_threw = true;
  }
  CHECK(endpoint_threw);

  bool positive_overflow_threw = false;
  try {
    (void)kruskal_mst(
        3, {{0, 1, std::numeric_limits<std::int64_t>::max()}, {1, 2, 1}});
  } catch (const std::overflow_error &) {
    positive_overflow_threw = true;
  }
  CHECK(positive_overflow_threw);

  bool negative_overflow_threw = false;
  try {
    (void)kruskal_mst(
        3, {{0, 1, std::numeric_limits<std::int64_t>::min()}, {1, 2, -1}});
  } catch (const std::overflow_error &) {
    negative_overflow_threw = true;
  }
  CHECK(negative_overflow_threw);
}

void test_randomized_against_comparison_kruskal() {
  std::mt19937_64 random(0xC13A10ULL);
  std::uniform_int_distribution<std::int64_t> weight_dist(-1000, 1000);
  for (std::size_t trial = 0; trial < 400; ++trial) {
    const std::size_t n = 1 + random() % 20;
    const std::size_t m = random() % (n * n + 1);
    std::vector<kruskal_edge> edges;
    edges.reserve(m);
    for (std::size_t i = 0; i < m; ++i) {
      edges.push_back({static_cast<std::size_t>(random() % n),
                       static_cast<std::size_t>(random() % n),
                       weight_dist(random)});
    }

    const auto actual = kruskal_mst(n, edges);
    const auto expected = reference_kruskal(n, edges);
    CHECK(actual.is_connected == expected.is_connected);
    CHECK(actual.total_weight == expected.total_weight);
    CHECK(actual.tree_edges.size() == expected.edge_count);
  }
}

void test_large_chain_sanity() {
  constexpr std::size_t vertex_count = 100000;
  std::vector<kruskal_edge> edges;
  edges.reserve(vertex_count - 1);
  for (std::size_t vertex = 1; vertex < vertex_count; ++vertex) {
    edges.push_back({vertex - 1, vertex, 1});
  }

  const auto result = kruskal_mst(vertex_count, edges);
  CHECK(result.is_connected);
  CHECK(result.tree_edges.size() == vertex_count - 1);
  CHECK(result.total_weight == static_cast<std::int64_t>(vertex_count - 1));
}

} // namespace

int main() {
  RUN(test_examples_and_edge_cases);
  RUN(test_negative_duplicate_and_boundary_weights);
  RUN(test_invalid_endpoints_and_weight_overflow);
  RUN(test_randomized_against_comparison_kruskal);
  RUN(test_large_chain_sanity);

  std::cout << "========================================================\n";
  if (g_failures == 0) {
    std::cout << "          All tests passed successfully!                \n";
  } else {
    std::cout << "          " << g_failures << " of " << g_checks
              << " checks failed.                            \n";
  }
  std::cout << "========================================================\n";
  return g_failures == 0 ? 0 : 1;
}
