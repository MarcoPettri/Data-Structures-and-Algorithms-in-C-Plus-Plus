// File : P-13.7.cpp

/*
    P-13.7 Implement the Floyd-Warshall transitive closure algorithm.
*/

#include "floyd_warshall_closure.hpp"

#include <cstddef>
#include <iostream>
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

using adjacency_matrix = std::vector<std::vector<bool>>;

adjacency_matrix breadth_first_closure(const adjacency_matrix &adjacency) {
  const std::size_t n = adjacency.size();
  adjacency_matrix reachable(n, std::vector<bool>(n, false));
  for (std::size_t source = 0; source < n; ++source) {
    std::vector<std::size_t> queue{source};
    reachable[source][source] = true;
    for (std::size_t head = 0; head < queue.size(); ++head) {
      const std::size_t from = queue[head];
      for (std::size_t to = 0; to < n; ++to) {
        if (adjacency[from][to] && !reachable[source][to]) {
          reachable[source][to] = true;
          queue.push_back(to);
        }
      }
    }
  }
  return reachable;
}

void test_empty_singleton_and_boundaries() {
  const auto empty = dsa::floyd_warshall_closure({});
  CHECK(empty.empty());

  const auto singleton = dsa::floyd_warshall_closure({{false}});
  CHECK(singleton.size() == 1);
  CHECK(singleton[0][0]);

  const auto self_loop = dsa::floyd_warshall_closure({{true}});
  CHECK(self_loop[0][0]);
}

void test_known_graphs() {
  // 0 -> 1 -> 2 -> 3, with vertex 4 isolated.
  const adjacency_matrix chain{{false, true, false, false, false},
                               {false, false, true, false, false},
                               {false, false, false, true, false},
                               {false, false, false, false, false},
                               {false, false, false, false, false}};
  const auto closure = dsa::floyd_warshall_closure(chain);
  CHECK(closure[0][0] && closure[0][1] && closure[0][2] && closure[0][3]);
  CHECK(!closure[0][4]);
  CHECK(closure[1][3]);
  CHECK(!closure[3][0]);
  CHECK(closure[4][4]);
  CHECK(!closure[4][0]);

  const adjacency_matrix cycle{
      {false, true, false}, {false, false, true}, {true, false, false}};
  const auto cycle_closure = dsa::floyd_warshall_closure(cycle);
  for (const auto &row : cycle_closure) {
    for (bool reachable : row) {
      CHECK(reachable);
    }
  }
}

void test_invalid_matrix_dimensions() {
  bool too_short_threw = false;
  try {
    (void)dsa::floyd_warshall_closure({{true, false}, {true}});
  } catch (const std::invalid_argument &) {
    too_short_threw = true;
  }
  CHECK(too_short_threw);

  bool too_long_threw = false;
  try {
    (void)dsa::floyd_warshall_closure({{true}, {false, true}});
  } catch (const std::invalid_argument &) {
    too_long_threw = true;
  }
  CHECK(too_long_threw);
}

void test_randomized_against_breadth_first_search() {
  std::mt19937_64 random(0xF10D7137ULL);
  for (std::size_t trial = 0; trial < 400; ++trial) {
    const std::size_t n = random() % 20;
    adjacency_matrix adjacency(n, std::vector<bool>(n, false));
    for (std::size_t from = 0; from < n; ++from) {
      for (std::size_t to = 0; to < n; ++to) {
        adjacency[from][to] = random() % 7 == 0;
      }
    }
    const auto actual = dsa::floyd_warshall_closure(adjacency);
    CHECK(actual == breadth_first_closure(adjacency));
  }
}

void test_large_chain_sanity() {
  constexpr std::size_t n = 240;
  adjacency_matrix chain(n, std::vector<bool>(n, false));
  for (std::size_t vertex = 1; vertex < n; ++vertex) {
    chain[vertex - 1][vertex] = true;
  }

  const auto closure = dsa::floyd_warshall_closure(chain);
  CHECK(closure[0][n - 1]);
  CHECK(closure[n / 2][n - 1]);
  CHECK(!closure[n - 1][0]);
  CHECK(closure[n - 1][n - 1]);
}

} // namespace

int main() {
  RUN(test_empty_singleton_and_boundaries);
  RUN(test_known_graphs);
  RUN(test_invalid_matrix_dimensions);
  RUN(test_randomized_against_breadth_first_search);
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
