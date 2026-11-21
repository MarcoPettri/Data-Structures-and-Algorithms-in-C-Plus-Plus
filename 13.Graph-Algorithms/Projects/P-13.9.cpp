// File : P-13.9.cpp
/*
    Implement Dijkstra’s algorithm assuming that the edge weights are integers.
*/

#include "dijkstra_integer.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <random>
#include <stdexcept>
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

using dsa::dijkstra;
using dsa::weighted_edge;
using dsa::weighted_graph;

void test_known_graph_and_paths() {
  const weighted_graph graph{
      {{1, 7}, {2, 2}, {2, 10}}, {{3, 3}}, {{1, 1}, {3, 8}}, {{4, 0}}, {}};
  const auto result = dijkstra(graph, 0);
  CHECK(result.distance[0] == 0);
  CHECK(result.distance[1] == 3);
  CHECK(result.distance[2] == 2);
  CHECK(result.distance[3] == 6);
  CHECK(result.distance[4] == 6);
  CHECK((result.path_to(4) == std::vector<std::size_t>{0, 2, 1, 3, 4}));
  CHECK((result.path_to(0) == std::vector<std::size_t>{0}));
}

void test_empty_singleton_and_unreachable() {
  bool source_threw = false;
  try {
    (void)dijkstra({}, 0);
  } catch (const std::out_of_range &) {
    source_threw = true;
  }
  CHECK(source_threw);

  const auto one = dijkstra(weighted_graph(1), 0);
  CHECK(one.distance[0] == 0);
  CHECK((one.path_to(0) == std::vector<std::size_t>{0}));

  const auto disconnected = dijkstra(weighted_graph(3), 1);
  CHECK(!disconnected.distance[0]);
  CHECK(disconnected.distance[1] == 0);
  CHECK(!disconnected.distance[2]);
  CHECK(disconnected.path_to(2).empty());
}

void test_validation_and_boundaries() {
  bool bad_target_threw = false;
  try {
    (void)dijkstra(weighted_graph{{{1, 1}}}, 0);
  } catch (const std::out_of_range &) {
    bad_target_threw = true;
  }
  CHECK(bad_target_threw);

  bool negative_threw = false;
  try {
    (void)dijkstra(weighted_graph{{{0, -1}}}, 0);
  } catch (const std::invalid_argument &) {
    negative_threw = true;
  }
  CHECK(negative_threw);

  const weighted_graph max_edge{{{1, std::numeric_limits<std::int64_t>::max()}},
                                {}};
  const auto max_result = dijkstra(max_edge, 0);
  CHECK(max_result.distance[1] ==
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()));

  const weighted_graph overflow_path{
      {{1, std::numeric_limits<std::int64_t>::max()}},
      {{2, std::numeric_limits<std::int64_t>::max()}},
      {{3, std::numeric_limits<std::int64_t>::max()}},
      {}};
  const auto overflow_result = dijkstra(overflow_path, 0);
  CHECK(overflow_result.distance[1] ==
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()));
  CHECK(overflow_result.distance[2] ==
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) *
            2);
  CHECK(!overflow_result.distance[3]);

  bool path_target_threw = false;
  try {
    (void)max_result.path_to(2);
  } catch (const std::out_of_range &) {
    path_target_threw = true;
  }
  CHECK(path_target_threw);
}

[[nodiscard]] std::vector<std::optional<std::uint64_t>>
bellman_ford_reference(const weighted_graph &graph, std::size_t source) {
  std::vector<std::optional<std::uint64_t>> distance(graph.size());
  distance[source] = 0;
  for (std::size_t pass = 1; pass < graph.size(); ++pass) {
    bool changed = false;
    for (std::size_t from = 0; from < graph.size(); ++from) {
      if (!distance[from]) {
        continue;
      }
      for (const weighted_edge &edge : graph[from]) {
        const std::uint64_t weight = static_cast<std::uint64_t>(edge.weight);
        if (weight >
            std::numeric_limits<std::uint64_t>::max() - *distance[from]) {
          continue;
        }
        const std::uint64_t candidate = *distance[from] + weight;
        if (!distance[edge.to] || candidate < *distance[edge.to]) {
          distance[edge.to] = candidate;
          changed = true;
        }
      }
    }
    if (!changed) {
      break;
    }
  }
  return distance;
}

void test_randomized_against_bellman_ford() {
  std::mt19937_64 random(0xD1A57AULL);
  for (std::size_t trial = 0; trial < 250; ++trial) {
    const std::size_t n = 1 + random() % 12;
    weighted_graph graph(n);
    for (std::size_t from = 0; from < n; ++from) {
      for (std::size_t to = 0; to < n; ++to) {
        if (random() % 5 == 0) {
          graph[from].push_back({to, static_cast<std::int64_t>(random() % 30)});
          if (random() % 17 == 0) {
            graph[from].push_back(
                {to, static_cast<std::int64_t>(random() % 30)});
          }
        }
      }
    }
    const std::size_t source = random() % n;
    const auto actual = dijkstra(graph, source);
    CHECK(actual.distance == bellman_ford_reference(graph, source));
  }
}

void test_large_chain_sanity() {
  constexpr std::size_t n = 100000;
  weighted_graph graph(n);
  for (std::size_t vertex = 1; vertex < n; ++vertex) {
    graph[vertex - 1].push_back({vertex, 1});
  }
  const auto result = dijkstra(graph, 0);
  CHECK(result.distance.back() == n - 1);
  CHECK(result.path_to(n - 1).size() == n);
}

} // namespace

int main() {
  RUN(test_known_graph_and_paths);
  RUN(test_empty_singleton_and_unreachable);
  RUN(test_validation_and_boundaries);
  RUN(test_randomized_against_bellman_ford);
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
