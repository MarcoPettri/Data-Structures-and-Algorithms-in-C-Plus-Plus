/**
 * @file test_graphs.cpp
 *
 * @brief GoogleTest suite for simple_graph (3 backends) and the full graph ADT.
 *
 *  Build with cmake, clang and Ninja:
     cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release -DSANITIZE=ON
     cmake --build build -j
     ./build/test_graphs        # or: ctest --test-dir build --output-on-failure
 *
 * Build (no CMake needed) with g++ and sanitizers directly:
    g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic \
        -fsanitize=address,undefined -I. test_graphs.cpp \
        -lgtest -lgtest_main -pthread -o test_graphs
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <list>
#include <numeric>
#include <random>
#include <string>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

#include "graph.hpp"
#include "simple_graph.hpp"

namespace {

using graph_adt::adjacency_list_storage;
using graph_adt::adjacency_matrix_storage;
using graph_adt::edge_list_storage;

using seconds = std::chrono::duration<double>;

// ============================================================================
// simple_graph: typed tests over the three storage back ends
// ============================================================================

template <typename Storage> class simple_graph_test : public ::testing::Test {
public:
  using graph_t = graph_adt::simple_graph<int, Storage>;
  using edge_t = std::pair<int, int>;
};

using storages = ::testing::Types<adjacency_matrix_storage,
                                  adjacency_list_storage, edge_list_storage>;
TYPED_TEST_SUITE(simple_graph_test, storages);

TYPED_TEST(simple_graph_test, EmptyGraph) {
  using G = typename TestFixture::graph_t;
  G g;
  EXPECT_TRUE(g.empty());
  EXPECT_EQ(g.num_vertices(), 0u);
  EXPECT_EQ(g.num_edges(), 0u);
  EXPECT_TRUE(g.vertices().empty());
  EXPECT_TRUE(g.edges().empty());
  EXPECT_FALSE(g.contains_vertex(1));
  EXPECT_THROW((void)g.degree(1), std::out_of_range);
  EXPECT_THROW((void)g.neighbors(1), std::out_of_range);
  EXPECT_THROW((void)g.are_adjacent(1, 2), std::out_of_range);

  G from_empty_ranges(std::vector<int>{}, std::vector<std::pair<int, int>>{});
  EXPECT_TRUE(from_empty_ranges.empty());
}

TYPED_TEST(simple_graph_test, SingleVertexAndIsolatedVertices) {
  using G = typename TestFixture::graph_t;
  G one(std::vector<int>{7}, std::vector<std::pair<int, int>>{});
  EXPECT_EQ(one.num_vertices(), 1u);
  EXPECT_EQ(one.num_edges(), 0u);
  EXPECT_EQ(one.degree(7), 0u);
  EXPECT_TRUE(one.neighbors(7).empty());
  EXPECT_FALSE(one.are_adjacent(7, 7));

  G iso({1, 2, 3}, {});
  EXPECT_EQ(iso.num_vertices(), 3u);
  EXPECT_TRUE(iso.edges().empty());
  EXPECT_FALSE(iso.are_adjacent(1, 2));
}

TYPED_TEST(simple_graph_test, SmallGraphQueries) {
  using G = typename TestFixture::graph_t;
  // Triangle on {1,2,3} plus isolated vertex 4.
  G g({1, 2, 3, 4}, {{1, 2}, {2, 3}, {3, 1}});
  EXPECT_EQ(g.num_vertices(), 4u);
  EXPECT_EQ(g.num_edges(), 3u);
  EXPECT_EQ(g.vertices(), (std::vector<int>{1, 2, 3, 4}));
  EXPECT_EQ(g.edges(),
            (std::vector<std::pair<int, int>>{{1, 2}, {1, 3}, {2, 3}}));
  EXPECT_EQ(g.neighbors(1), (std::vector<int>{2, 3}));
  EXPECT_EQ(g.neighbors(3), (std::vector<int>{1, 2}));
  EXPECT_TRUE(g.neighbors(4).empty());
  EXPECT_EQ(g.degree(2), 2u);
  EXPECT_EQ(g.degree(4), 0u);
  EXPECT_TRUE(g.are_adjacent(1, 3));
  EXPECT_TRUE(g.are_adjacent(3, 1)); // symmetric
  EXPECT_FALSE(g.are_adjacent(1, 4));
  EXPECT_FALSE(g.are_adjacent(2, 2)); // no self adjacency
  EXPECT_TRUE(g.contains_vertex(4));
  EXPECT_FALSE(g.contains_vertex(5));
}

TYPED_TEST(simple_graph_test, OrderFollowsConstructionNotValue) {
  using G = typename TestFixture::graph_t;
  // Vertex indices: 30 -> 0, 10 -> 1, 20 -> 2.
  G g({30, 10, 20}, {{20, 30}, {10, 30}});
  EXPECT_EQ(g.vertices(), (std::vector<int>{30, 10, 20}));
  EXPECT_EQ(g.edges(), (std::vector<std::pair<int, int>>{{30, 10}, {30, 20}}));
  EXPECT_EQ(g.neighbors(30), (std::vector<int>{10, 20}));
  EXPECT_EQ(g.neighbors(20), (std::vector<int>{30}));
}

TYPED_TEST(simple_graph_test, InvalidInputThrows) {
  using G = typename TestFixture::graph_t;
  EXPECT_THROW(G({1, 2, 1}, {}), std::invalid_argument); // dup vertex
  EXPECT_THROW(G({1, 2}, {{1, 3}}), std::invalid_argument); // unknown
  EXPECT_THROW(G({1, 2}, {{3, 1}}), std::invalid_argument); // unknown
  EXPECT_THROW(G({1, 2}, {{1, 1}}), std::invalid_argument); // self-loop
  EXPECT_THROW(G({1, 2}, {{1, 2}, {1, 2}}), std::invalid_argument);
  EXPECT_THROW(G({1, 2}, {{1, 2}, {2, 1}}), std::invalid_argument); // reversed
}

TYPED_TEST(simple_graph_test, QueriesOnUnknownVerticesThrow) {
  using G = typename TestFixture::graph_t;
  G g({1, 2}, {{1, 2}});
  EXPECT_THROW((void)g.degree(9), std::out_of_range);
  EXPECT_THROW((void)g.neighbors(9), std::out_of_range);
  EXPECT_THROW((void)g.are_adjacent(1, 9), std::out_of_range);
  EXPECT_THROW((void)g.are_adjacent(9, 1), std::out_of_range);
}

TYPED_TEST(simple_graph_test, AllConstructorsAgree) {
  using G = typename TestFixture::graph_t;
  const std::vector<int> vs{5, 3, 9, 4};
  const std::vector<std::pair<int, int>> es{{5, 3}, {9, 3}};
  const G a(vs, es);
  const G b(vs.begin(), vs.end(), es.begin(), es.end());
  const G c({5, 3, 9, 4}, {{5, 3}, {9, 3}});

  const std::list<int> lv(vs.begin(), vs.end());
  std::vector<std::tuple<int, int>> te;
  te.emplace_back(5, 3);
  te.emplace_back(9, 3);
  const G d(lv, te); // list of vertices, tuple-like edges

  const std::vector<std::pair<int, int>> expected{{5, 3}, {3, 9}};
  for (const G *g : {&a, &b, &c, &d}) {
    EXPECT_EQ(g->vertices(), vs);
    EXPECT_EQ(g->edges(), expected);
  }
}

TYPED_TEST(simple_graph_test, NonTrivialVertexType) {
  using G = graph_adt::simple_graph<std::string, TypeParam>;
  const std::vector<std::string> vs{"alpha", "beta", "gamma"};
  const std::vector<std::pair<std::string, std::string>> es{{"alpha", "beta"},
                                                            {"gamma", "beta"}};
  const G g(vs, es);
  EXPECT_EQ(g.num_edges(), 2u);
  EXPECT_TRUE(g.are_adjacent("beta", "gamma"));
  EXPECT_FALSE(g.are_adjacent("alpha", "gamma"));
  EXPECT_EQ(g.neighbors("beta"), (std::vector<std::string>{"alpha", "gamma"}));
}

TYPED_TEST(simple_graph_test, RandomizedAgainstReference) {
  using G = typename TestFixture::graph_t;
  std::mt19937 rng(12345); // fixed seed
  for (int trial = 0; trial < 200; ++trial) {
    const std::size_t n = rng() % 25;
    std::vector<int> vals(n);
    std::iota(vals.begin(), vals.end(), 1000);
    std::shuffle(vals.begin(), vals.end(), rng);
    const double p = static_cast<double>(rng() % 100) / 100.0;

    std::vector<std::vector<bool>> ref(n, std::vector<bool>(n, false));
    std::vector<std::pair<int, int>> input;
    for (std::size_t i = 0; i < n; ++i)
      for (std::size_t j = i + 1; j < n; ++j)
        if (static_cast<double>(rng() % 1000) / 1000.0 < p) {
          ref[i][j] = ref[j][i] = true;
          if (rng() & 1u)
            input.emplace_back(vals[i], vals[j]);
          else
            input.emplace_back(vals[j], vals[i]);
        }
    std::shuffle(input.begin(), input.end(), rng);

    const G g(vals, input);

    std::vector<std::pair<int, int>> expected_edges;
    for (std::size_t i = 0; i < n; ++i)
      for (std::size_t j = i + 1; j < n; ++j)
        if (ref[i][j])
          expected_edges.emplace_back(vals[i], vals[j]);
    EXPECT_EQ(g.edges(), expected_edges);
    EXPECT_EQ(g.num_edges(), expected_edges.size());
    EXPECT_EQ(g.num_vertices(), n);

    for (std::size_t i = 0; i < n; ++i) {
      std::vector<int> expected_nbrs;
      for (std::size_t j = 0; j < n; ++j)
        if (ref[i][j])
          expected_nbrs.push_back(vals[j]);
      EXPECT_EQ(g.neighbors(vals[i]), expected_nbrs);
      EXPECT_EQ(g.degree(vals[i]), expected_nbrs.size());
      for (std::size_t j = 0; j < n; ++j)
        EXPECT_EQ(g.are_adjacent(vals[i], vals[j]),
                  static_cast<bool>(ref[i][j]));
    }
  }
}

TYPED_TEST(simple_graph_test, PerformanceSanity) {
  using G = typename TestFixture::graph_t;
  constexpr std::size_t n = 4000;
  std::vector<int> vals(n);
  std::iota(vals.begin(), vals.end(), 0); // value == index

  const auto make_edges = [&](std::size_t m, std::uint64_t seed,
                              std::unordered_set<std::uint64_t> &keys) {
    std::mt19937_64 rng(seed);
    std::vector<std::pair<int, int>> out;
    out.reserve(m);
    while (out.size() < m) {
      std::size_t a = rng() % n, b = rng() % n;
      if (a == b)
        continue;
      if (a > b)
        std::swap(a, b);
      if (!keys.insert(a * n + b).second)
        continue;
      out.emplace_back(vals[a], vals[b]);
    }
    return out;
  };

  // Best of three builds, to reduce timing noise.
  const auto time_build = [&](const std::vector<std::pair<int, int>> &es) {
    double best = 1e9;
    for (int rep = 0; rep < 3; ++rep) {
      const auto t0 = std::chrono::steady_clock::now();
      const G g(vals, es);
      const double t = seconds(std::chrono::steady_clock::now() - t0).count();
      EXPECT_EQ(g.num_edges(), es.size());
      best = std::min(best, t);
    }
    return best;
  };

  std::unordered_set<std::uint64_t> small_keys, large_keys;
  const auto small = make_edges(20000, 1, small_keys);
  const auto large = make_edges(160000, 2, large_keys); // 8x more edges
  const double t_small = time_build(small);
  const double t_large = time_build(large);
  // Construction is O(n + m log m) (plus O(n^2) constant for the matrix), so
  // 8x more edges must cost far less than 40x. Deliberately loose: this
  // catches accidental quadratic behaviour, not constant factors.
  EXPECT_LT(t_large, 40.0 * std::max(t_small, 1e-3));

  // Query throughput and correctness on the large graph.
  const G g(vals, large);
  std::mt19937_64 rng(99);
  const auto t0 = std::chrono::steady_clock::now();
  for (int q = 0; q < 200000; ++q) {
    std::size_t a = rng() % n, b = rng() % n;
    const bool got = g.are_adjacent(vals[a], vals[b]);
    if (a > b)
      std::swap(a, b);
    const bool want = a != b && large_keys.count(a * n + b) != 0;
    ASSERT_EQ(got, want);
  }
  EXPECT_LT(seconds(std::chrono::steady_clock::now() - t0).count(), 10.0);
}

TEST(simple_graph_aliases, ThreeAliasesBehaveIdentically) {
  graph_adt::adjacency_matrix_graph<int> m({1, 2, 3}, {{1, 2}, {2, 3}});
  graph_adt::adjacency_list_graph<int> l({1, 2, 3}, {{1, 2}, {2, 3}});
  graph_adt::edge_list_graph<int> e({1, 2, 3}, {{1, 2}, {2, 3}});
  EXPECT_EQ(m.edges(), l.edges());
  EXPECT_EQ(l.edges(), e.edges());
  EXPECT_EQ(m.neighbors(2), e.neighbors(2));
}

// ============================================================================
// graph<V, E>: full ADT (P-13.4)
// ============================================================================

using vertex_handle = graph_adt::vertex_handle;
using edge_handle = graph_adt::edge_handle;

TEST(graph_full, ConstructFromCollections) {
  graph_adt::graph<std::string> g(
      std::vector<std::string>{"a", "b", "c"},
      std::vector<std::pair<std::string, std::string>>{{"a", "b"}, {"b", "c"}});
  EXPECT_EQ(g.num_vertices(), 3u);
  EXPECT_EQ(g.num_edges(), 2u);
  const auto vs = g.vertices();
  ASSERT_EQ(vs.size(), 3u);
  EXPECT_EQ(g.element(vs[0]), "a");
  EXPECT_EQ(g.element(vs[2]), "c");
  EXPECT_TRUE(g.are_adjacent(vs[0], vs[1]));
  EXPECT_FALSE(g.are_adjacent(vs[0], vs[2]));
  EXPECT_FALSE(g.get_edge(vs[0], vs[2]).has_value());
  const auto e = g.get_edge(vs[1], vs[0]); // undirected: both ways
  ASSERT_TRUE(e.has_value());
  EXPECT_FALSE(g.is_directed(*e));
  EXPECT_EQ(*g.find_vertex("b"), vs[1]);
  EXPECT_FALSE(g.find_vertex("zzz").has_value());
}

TEST(graph_full, ConstructorRejectsBadInput) {
  using G = graph_adt::graph<int>;
  EXPECT_THROW(G({1, 2, 1}, {}), std::invalid_argument);
  EXPECT_THROW(G({1, 2}, {{1, 3}}), std::invalid_argument);
  EXPECT_THROW(G({1, 2}, {{2, 2}}), std::invalid_argument);
  G ok({1, 2}, {{1, 2}, {2, 1}}); // parallel edges are allowed here
  EXPECT_EQ(ok.num_edges(), 2u);
}

TEST(graph_full, EmptyGraph) {
  graph_adt::graph<int, int> g;
  EXPECT_TRUE(g.empty());
  EXPECT_TRUE(g.vertices().empty());
  EXPECT_TRUE(g.edges().empty());
  EXPECT_FALSE(g.contains(vertex_handle{}));
  EXPECT_THROW((void)g.degree(vertex_handle{}), std::invalid_argument);
  EXPECT_THROW((void)g.element(edge_handle{}), std::invalid_argument);
}

TEST(graph_full, ElementsAndReplace) {
  graph_adt::graph<std::string, int> g;
  const auto a = g.insert_vertex("a");
  const auto b = g.insert_vertex("b");
  const auto e = g.insert_edge(a, b, 42);
  EXPECT_EQ(g.element(a), "a");
  EXPECT_EQ(g.element(e), 42);
  EXPECT_EQ(g.replace(a, "z"), "a");
  EXPECT_EQ(g.element(a), "z");
  EXPECT_EQ(g.replace(e, 7), 42);
  EXPECT_EQ(g.element(e), 7);
}

TEST(graph_full, UndirectedEdgeSemantics) {
  graph_adt::graph<int, int> g;
  const auto a = g.insert_vertex(1);
  const auto b = g.insert_vertex(2);
  const auto c = g.insert_vertex(3);
  const auto e = g.insert_edge(a, b, 10);
  EXPECT_FALSE(g.is_directed(e));
  EXPECT_EQ(g.degree(a), 1u);
  EXPECT_EQ(g.in_degree(a), 1u);
  EXPECT_EQ(g.out_degree(a), 1u);
  EXPECT_EQ(g.opposite(a, e), b);
  EXPECT_EQ(g.opposite(b, e), a);
  EXPECT_EQ(g.end_vertices(e), std::make_pair(a, b));
  EXPECT_THROW((void)g.opposite(c, e), std::invalid_argument);
  EXPECT_THROW((void)g.origin(e), std::invalid_argument);
  EXPECT_THROW((void)g.destination(e), std::invalid_argument);
  EXPECT_THROW(g.insert_edge(a, a, 0), std::invalid_argument); // self-loop
  EXPECT_EQ(g.num_edges(), 1u); // unchanged
  EXPECT_EQ(g.degree(c), 0u);
}

TEST(graph_full, DirectedEdgeSemantics) {
  graph_adt::graph<int, int> g;
  const auto a = g.insert_vertex(1);
  const auto b = g.insert_vertex(2);
  const auto e = g.insert_directed_edge(a, b, 5);
  EXPECT_TRUE(g.is_directed(e));
  EXPECT_EQ(g.origin(e), a);
  EXPECT_EQ(g.destination(e), b);
  EXPECT_EQ(g.out_degree(a), 1u);
  EXPECT_EQ(g.in_degree(a), 0u);
  EXPECT_EQ(g.out_degree(b), 0u);
  EXPECT_EQ(g.in_degree(b), 1u);
  EXPECT_EQ(g.degree(a), 1u);
  EXPECT_TRUE(g.get_edge(a, b).has_value());
  EXPECT_FALSE(g.get_edge(b, a).has_value());
  EXPECT_TRUE(g.are_adjacent(b, a)); // adjacency ignores direction
  EXPECT_EQ(g.out_edges(a), (std::vector<edge_handle>{e}));
  EXPECT_TRUE(g.out_edges(b).empty());
  EXPECT_EQ(g.in_edges(b), (std::vector<edge_handle>{e}));
  EXPECT_EQ(g.out_adjacent_vertices(a), (std::vector<vertex_handle>{b}));
  EXPECT_EQ(g.in_adjacent_vertices(b), (std::vector<vertex_handle>{a}));
  EXPECT_EQ(g.adjacent_vertices(b), (std::vector<vertex_handle>{a}));
}

TEST(graph_full, ChangingDirection) {
  graph_adt::graph<int, int> g;
  const auto a = g.insert_vertex(1);
  const auto b = g.insert_vertex(2);
  const auto c = g.insert_vertex(3);
  const auto e = g.insert_directed_edge(a, b, 0);

  g.reverse_direction(e);
  EXPECT_EQ(g.origin(e), b);
  EXPECT_EQ(g.destination(e), a);
  EXPECT_TRUE(g.get_edge(b, a).has_value());
  EXPECT_FALSE(g.get_edge(a, b).has_value());

  g.set_direction_from(e, a);
  EXPECT_EQ(g.origin(e), a);
  g.set_direction_to(e, a);
  EXPECT_EQ(g.destination(e), a);
  EXPECT_EQ(g.out_degree(a), 0u);
  EXPECT_EQ(g.in_degree(a), 1u);

  EXPECT_THROW(g.set_direction_from(e, c), std::invalid_argument);
  EXPECT_THROW(g.set_direction_to(e, c), std::invalid_argument);

  g.make_undirected(e);
  EXPECT_FALSE(g.is_directed(e));
  EXPECT_EQ(g.out_degree(a), 1u);
  EXPECT_EQ(g.in_degree(a), 1u);
  EXPECT_THROW(g.reverse_direction(e), std::invalid_argument);

  g.set_direction_from(e, b); // undirected -> directed b -> a
  EXPECT_TRUE(g.is_directed(e));
  EXPECT_EQ(g.origin(e), b);
}

TEST(graph_full, ParallelEdgesAndRemoval) {
  graph_adt::graph<int, int> g;
  const auto a = g.insert_vertex(1);
  const auto b = g.insert_vertex(2);
  const auto e1 = g.insert_edge(a, b, 1);
  const auto e2 = g.insert_edge(a, b, 2);
  EXPECT_EQ(g.degree(a), 2u);
  EXPECT_EQ(g.adjacent_vertices(a), (std::vector<vertex_handle>{b, b}));
  EXPECT_EQ(g.remove_edge(e1), 1);
  EXPECT_EQ(g.degree(a), 1u);
  EXPECT_EQ(g.get_edge(a, b).value(), e2);
  EXPECT_THROW((void)g.element(e1), std::invalid_argument); // stale
  EXPECT_THROW(g.remove_edge(e1), std::invalid_argument); // double remove
  EXPECT_EQ(g.remove_edge(e2), 2);
  EXPECT_EQ(g.num_edges(), 0u);
  EXPECT_FALSE(g.are_adjacent(a, b));
}

TEST(graph_full, RemoveVertexRemovesIncidentEdges) {
  graph_adt::graph<int, int> g;
  const auto a = g.insert_vertex(1);
  const auto b = g.insert_vertex(2);
  const auto c = g.insert_vertex(3);
  const auto ab = g.insert_edge(a, b, 0);
  const auto bc = g.insert_directed_edge(b, c, 0);
  const auto ac = g.insert_edge(a, c, 0);
  EXPECT_EQ(g.remove_vertex(b), 2);
  EXPECT_EQ(g.num_vertices(), 2u);
  EXPECT_EQ(g.num_edges(), 1u);
  EXPECT_FALSE(g.contains(b));
  EXPECT_FALSE(g.contains(ab));
  EXPECT_FALSE(g.contains(bc));
  EXPECT_TRUE(g.contains(ac));
  EXPECT_EQ(g.degree(a), 1u);
  EXPECT_EQ(g.degree(c), 1u);
  EXPECT_EQ(g.in_degree(c), 1u);
  EXPECT_THROW((void)g.degree(b), std::invalid_argument);
  EXPECT_THROW(g.insert_edge(a, b, 0), std::invalid_argument);
  EXPECT_THROW(g.remove_vertex(b), std::invalid_argument);
}

TEST(graph_full, ForeignAndNullHandlesAreRejected) {
  graph_adt::graph<int> g1, g2;
  const auto v1 = g1.insert_vertex(1);
  const auto w1 = g1.insert_vertex(2);
  const auto e1 = g1.insert_edge(v1, w1);
  g2.insert_vertex(1);
  EXPECT_THROW((void)g2.degree(v1), std::invalid_argument);
  EXPECT_THROW((void)g2.is_directed(e1), std::invalid_argument);
  EXPECT_THROW((void)g1.degree(vertex_handle{}), std::invalid_argument);
}

TEST(graph_full, MoveKeepsHandlesValid) {
  graph_adt::graph<int> g;
  const auto a = g.insert_vertex(1);
  const auto b = g.insert_vertex(2);
  const auto e = g.insert_edge(a, b);

  graph_adt::graph<int> h(std::move(g));
  EXPECT_EQ(g.num_vertices(), 0u); // NOLINT(bugprone-use-after-move)
  EXPECT_EQ(g.num_edges(), 0u);
  EXPECT_THROW((void)g.degree(a), std::invalid_argument);
  EXPECT_EQ(h.num_vertices(), 2u);
  EXPECT_TRUE(h.contains(e));
  EXPECT_TRUE(h.are_adjacent(a, b));

  graph_adt::graph<int> k;
  k.insert_vertex(99);
  k = std::move(h);
  EXPECT_EQ(k.num_vertices(), 2u);
  EXPECT_EQ(k.remove_vertex(a), 1);
  EXPECT_EQ(k.num_edges(), 0u);
  k.clear();
  EXPECT_TRUE(k.empty());
}

namespace {

struct ref_edge {
  edge_handle h;
  vertex_handle a, b; // origin/destination if directed
  bool directed;
  int value;
};

// Recomputes every observable property from the reference model and compares.
void check_graph(const graph_adt::graph<int, int> &g,
                 const std::vector<vertex_handle> &rv,
                 const std::vector<ref_edge> &re) {
  ASSERT_EQ(g.num_vertices(), rv.size());
  ASSERT_EQ(g.num_edges(), re.size());
  EXPECT_EQ(g.vertices(), rv);
  std::vector<edge_handle> all_edges;
  for (const auto &e : re)
    all_edges.push_back(e.h);
  EXPECT_EQ(g.edges(), all_edges);

  for (const auto &e : re) {
    EXPECT_EQ(g.end_vertices(e.h), std::make_pair(e.a, e.b));
    EXPECT_EQ(g.is_directed(e.h), e.directed);
    EXPECT_EQ(g.element(e.h), e.value);
    EXPECT_EQ(g.opposite(e.a, e.h), e.b);
    EXPECT_EQ(g.opposite(e.b, e.h), e.a);
  }

  for (const vertex_handle v : rv) {
    std::vector<edge_handle> inc, outs, ins;
    std::vector<vertex_handle> adj, out_adj, in_adj;
    for (const auto &e : re) {
      const bool is_a = e.a == v;
      const bool is_b = e.b == v;
      if (!is_a && !is_b)
        continue;
      const vertex_handle other = is_a ? e.b : e.a;
      inc.push_back(e.h);
      adj.push_back(other);
      if (!e.directed || is_a) {
        outs.push_back(e.h);
        out_adj.push_back(other);
      }
      if (!e.directed || is_b) {
        ins.push_back(e.h);
        in_adj.push_back(other);
      }
    }
    EXPECT_EQ(g.degree(v), inc.size());
    EXPECT_EQ(g.out_degree(v), outs.size());
    EXPECT_EQ(g.in_degree(v), ins.size());
    EXPECT_EQ(g.incident_edges(v), inc);
    EXPECT_EQ(g.out_edges(v), outs);
    EXPECT_EQ(g.in_edges(v), ins);
    EXPECT_EQ(g.adjacent_vertices(v), adj);
    EXPECT_EQ(g.out_adjacent_vertices(v), out_adj);
    EXPECT_EQ(g.in_adjacent_vertices(v), in_adj);
  }
}

} // namespace

TEST(graph_full, RandomizedAgainstReferenceModel) {
  graph_adt::graph<int, int> g;
  std::vector<vertex_handle> rv;
  std::vector<ref_edge> re;
  std::mt19937 rng(2024); // fixed seed
  const auto pick = [&rng](std::size_t n) {
    return std::uniform_int_distribution<std::size_t>(0, n - 1)(rng);
  };

  for (int step = 0; step < 4000; ++step) {
    const std::size_t op = pick(100);
    if (rv.size() < 2 || (op < 20 && rv.size() < 40)) {
      rv.push_back(g.insert_vertex(step));
    } else if (op < 50) {
      const std::size_t i = pick(rv.size());
      const std::size_t j = pick(rv.size());
      if (i == j) {
        EXPECT_THROW(g.insert_edge(rv[i], rv[j], step), std::invalid_argument);
      } else {
        const bool directed = pick(2) == 0;
        const edge_handle h = directed
                                  ? g.insert_directed_edge(rv[i], rv[j], step)
                                  : g.insert_edge(rv[i], rv[j], step);
        re.push_back({h, rv[i], rv[j], directed, step});
      }
    } else if (op < 65) {
      if (!re.empty()) {
        const std::size_t idx = pick(re.size());
        EXPECT_EQ(g.remove_edge(re[idx].h), re[idx].value);
        EXPECT_FALSE(g.contains(re[idx].h));
        re.erase(re.begin() + static_cast<std::ptrdiff_t>(idx));
      }
    } else if (op < 72) {
      if (rv.size() > 2) {
        const std::size_t idx = pick(rv.size());
        const vertex_handle v = rv[idx];
        g.remove_vertex(v);
        EXPECT_FALSE(g.contains(v));
        re.erase(std::remove_if(
                     re.begin(), re.end(),
                     [v](const ref_edge &e) { return e.a == v || e.b == v; }),
                 re.end());
        rv.erase(rv.begin() + static_cast<std::ptrdiff_t>(idx));
      }
    } else if (!re.empty()) {
      ref_edge &e = re[pick(re.size())];
      switch (pick(4)) {
      case 0:
        if (e.directed) {
          g.reverse_direction(e.h);
          std::swap(e.a, e.b);
        } else {
          EXPECT_THROW(g.reverse_direction(e.h), std::invalid_argument);
        }
        break;
      case 1: {
        const vertex_handle v = pick(2) == 0 ? e.a : e.b;
        g.set_direction_from(e.h, v);
        if (v == e.b)
          std::swap(e.a, e.b);
        e.directed = true;
        break;
      }
      case 2: {
        const vertex_handle v = pick(2) == 0 ? e.a : e.b;
        g.set_direction_to(e.h, v);
        if (v == e.a)
          std::swap(e.a, e.b);
        e.directed = true;
        break;
      }
      default:
        g.make_undirected(e.h);
        e.directed = false;
        break;
      }
    }

    ASSERT_EQ(g.num_vertices(), rv.size());
    ASSERT_EQ(g.num_edges(), re.size());

    if (step % 50 == 0 || step == 3999) {
      check_graph(g, rv, re);
      for (int k = 0; k < 30; ++k) {
        const vertex_handle u = rv[pick(rv.size())];
        const vertex_handle w = rv[pick(rv.size())];
        bool adjacent = false, traversable = false;
        for (const auto &e : re) {
          if ((e.a == u && e.b == w) || (e.a == w && e.b == u))
            adjacent = true;
          if ((e.a == u && e.b == w) || (!e.directed && e.a == w && e.b == u))
            traversable = true;
        }
        EXPECT_EQ(g.are_adjacent(u, w), adjacent);
        const auto found = g.get_edge(u, w);
        EXPECT_EQ(found.has_value(), traversable);
        if (found) {
          const auto it =
              std::find_if(re.begin(), re.end(),
                           [&](const ref_edge &e) { return e.h == *found; });
          ASSERT_TRUE(it != re.end());
          EXPECT_TRUE((it->a == u && it->b == w) ||
                      (!it->directed && it->a == w && it->b == u));
        }
      }
    }
  }

  // Tear everything down through the public API.
  while (!rv.empty()) {
    g.remove_vertex(rv.back());
    rv.pop_back();
  }
  EXPECT_TRUE(g.empty());
  EXPECT_EQ(g.num_edges(), 0u);
}

TEST(graph_full, PerformanceSanity) {
  const auto run = [](std::size_t n) {
    std::mt19937_64 rng(7);
    const auto t0 = std::chrono::steady_clock::now();
    graph_adt::graph<int, int> g;
    std::vector<vertex_handle> vs;
    vs.reserve(n);
    for (std::size_t i = 0; i < n; ++i)
      vs.push_back(g.insert_vertex(static_cast<int>(i)));
    for (std::size_t i = 0; i < 2 * n; ++i) {
      const std::size_t u = rng() % n, w = rng() % n;
      if (u != w)
        g.insert_edge(vs[u], vs[w], static_cast<int>(i));
    }
    std::size_t degree_sum = 0;
    for (const vertex_handle v : vs)
      degree_sum += g.degree(v);
    EXPECT_EQ(degree_sum, 2 * g.num_edges()); // handshake lemma
    for (const vertex_handle v : vs)
      g.remove_vertex(v);
    EXPECT_TRUE(g.empty());
    EXPECT_EQ(g.num_edges(), 0u);
    return seconds(std::chrono::steady_clock::now() - t0).count();
  };

  const double t_small = run(20000);
  const double t_large = run(160000); // 8x larger
  // All operations are expected O(1) or O(deg); 8x data must cost well below
  // 40x time. Loose on purpose (cache effects, sanitizers).
  EXPECT_LT(t_large, 40.0 * std::max(t_small, 1e-3));
}

} // namespace
