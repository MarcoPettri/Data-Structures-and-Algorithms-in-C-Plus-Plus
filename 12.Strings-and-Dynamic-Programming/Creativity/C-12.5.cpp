// File : C-12.5.cpp

/*
    C-12.5 Let P be a convex polygon, a triangulation of P is an addition of diag-
    onals connecting the vertices of P so that each interior face is a triangle.
    The weight of a triangulation is the sum of the lengths of the diagonals.
    Assuming that we can compute lengths and add and compare them in con-
    stant time, give an efficient algorithm for computing a minimum-weight
    triangulation of P.
*/

#include "minimum_weight_triangulation.hpp"

#include <cassert>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

namespace {

constexpr double EPS = 1e-7;

/**
 * @brief Helper to generate regular convex n-gons inscribed in a circle of radius R.
 */
std::vector<dsa::Point2D<double>>
generate_regular_polygon(std::size_t n, double radius = 1.0) {
  std::vector<dsa::Point2D<double>> vertices;
  vertices.reserve(n);
  const double angle_step = 2.0 * M_PI / static_cast<double>(n);
  for (std::size_t i = 0; i < n; ++i) {
    const double theta = angle_step * static_cast<double>(i);
    vertices.push_back({radius * std::cos(theta), radius * std::sin(theta)});
  }
  return vertices;
}

/**
 * @brief Brute-force recursive enumerator of all triangulations for small n.
 * Used as an oracle to formally verify dynamic programming optimality.
 */
template <typename DistanceFunc>
double brute_force_mwt(std::size_t i, std::size_t j, std::size_t n,
                       DistanceFunc &&dist_fn) {
  if (j - i < 3) {
    return 0.0;
  }

  auto is_internal = [n](std::size_t u, std::size_t v) {
    if (u > v)
      std::swap(u, v);
    return !(v == u + 1 || (u == 0 && v == n - 1));
  };

  double min_cost = std::numeric_limits<double>::infinity();
  for (std::size_t k = i + 1; k < j; ++k) {
    double cost =
        brute_force_mwt(i, k, n, dist_fn) + brute_force_mwt(k, j, n, dist_fn);
    if (is_internal(i, k))
      cost += dist_fn(i, k);
    if (is_internal(k, j))
      cost += dist_fn(k, j);
    if (cost < min_cost) {
      min_cost = cost;
    }
  }
  return min_cost;
}

void test_edge_cases() {
  std::cout << "[*] Running edge case tests...\n";

  // Edge Case 1: Empty polygon (n = 0)
  {
    std::vector<dsa::Point2D<double>> poly;
    auto res = dsa::solve_minimum_weight_triangulation(poly);
    assert(res.total_weight == 0.0);
    assert(res.diagonals.empty());
    assert(res.triangles.empty());
    assert(res.is_valid_triangulation(0));
    assert(!dsa::MinimumWeightTriangulation::is_convex(poly));
  }

  // Edge Case 2: Single point (n = 1)
  {
    std::vector<dsa::Point2D<double>> poly = {{0.0, 0.0}};
    auto res = dsa::solve_minimum_weight_triangulation(poly);
    assert(res.total_weight == 0.0);
    assert(res.diagonals.empty());
    assert(res.triangles.empty());
    assert(res.is_valid_triangulation(1));
    assert(!dsa::MinimumWeightTriangulation::is_convex(poly));
  }

  // Edge Case 3: Line segment (n = 2)
  {
    std::vector<dsa::Point2D<double>> poly = {{0.0, 0.0}, {1.0, 1.0}};
    auto res = dsa::solve_minimum_weight_triangulation(poly);
    assert(res.total_weight == 0.0);
    assert(res.diagonals.empty());
    assert(res.triangles.empty());
    assert(res.is_valid_triangulation(2));
    assert(!dsa::MinimumWeightTriangulation::is_convex(poly));
  }

  // Edge Case 4: Triangle (n = 3)
  {
    std::vector<dsa::Point2D<double>> poly = {
        {0.0, 0.0}, {2.0, 0.0}, {1.0, 2.0}};
    assert(dsa::MinimumWeightTriangulation::is_convex(poly));
    auto res = dsa::solve_minimum_weight_triangulation(poly);
    assert(res.total_weight == 0.0);
    assert(res.diagonals.empty());
    assert(res.triangle_count() == 1);
    assert(res.is_valid_triangulation(3));
  }

  // Edge Case 5: Non-convex (concave) polygon detection
  {
    // Arrowhead shape: (0,0), (2,1), (0,2), (0.5, 1) -> interior indent
    std::vector<dsa::Point2D<double>> concave = {
        {0.0, 0.0}, {2.0, 1.0}, {0.0, 2.0}, {0.5, 1.0}};
    assert(!dsa::MinimumWeightTriangulation::is_convex(concave));
  }

  // Edge Case 6: Colinear vertices detection
  {
    std::vector<dsa::Point2D<double>> colinear = {
        {0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0}, {1.0, 1.0}};
    assert(!dsa::MinimumWeightTriangulation::is_convex(colinear));
  }

  std::cout << "    [+] Edge cases passed.\n";
}

void test_functional_and_analytical_cases() {
  std::cout << "[*] Running functional and analytical tests...\n";

  // Test 1: Diamond / Kite Quadrilateral with asymmetric diagonals
  // Vertices: (1, 0), (0, 2), (-1, 0), (0, -2) in CCW order
  // Diagonal 1: (v0, v2) connects (1, 0) and (-1, 0) -> length = 2.0
  // Diagonal 2: (v1, v3) connects (0, 2) and (0, -2) -> length = 4.0
  // Minimum weight triangulation MUST pick (v0, v2) with weight 2.0
  {
    std::vector<dsa::Point2D<double>> diamond = {
        {1.0, 0.0}, {0.0, 2.0}, {-1.0, 0.0}, {0.0, -2.0}};
    assert(dsa::MinimumWeightTriangulation::is_convex(diamond));

    auto res = dsa::solve_minimum_weight_triangulation(diamond);
    assert(std::abs(res.total_weight - 2.0) < EPS);
    assert(res.diagonal_count() == 1);
    assert(res.triangle_count() == 2);
    assert(res.diagonals[0].u == 0 && res.diagonals[0].v == 2);
    assert(res.is_valid_triangulation(4));
  }

  // Test 2: Rectangle (10 x 2)
  // Vertices: (0, 0), (10, 0), (10, 2), (0, 2)
  // Diagonals (0, 2) and (1, 3) both have length sqrt(100 + 4) = sqrt(104) ~ 10.198039
  {
    std::vector<dsa::Point2D<double>> rect = {
        {0.0, 0.0}, {10.0, 0.0}, {10.0, 2.0}, {0.0, 2.0}};
    assert(dsa::MinimumWeightTriangulation::is_convex(rect));

    auto res = dsa::solve_minimum_weight_triangulation(rect);
    const double expected_weight = std::hypot(10.0, 2.0);
    assert(std::abs(res.total_weight - expected_weight) < EPS);
    assert(res.diagonal_count() == 1);
    assert(res.triangle_count() == 2);
    assert(res.is_valid_triangulation(4));
  }

  // Test 3: Regular Pentagon (side = 1)
  // For a regular pentagon with side 1, the golden ratio phi = (1 + sqrt(5)) / 2
  // All 5 internal diagonals have identical length = phi = 1.618033988749895
  // Any triangulation has 5 - 3 = 2 diagonals, so the weight is 2 * phi = 3.23606797749979
  {
    const double phi = (1.0 + std::sqrt(5.0)) / 2.0;
    // Inscribe pentagon such that side length is 1: R = 1 / (2 * sin(pi/5))
    const double R = 1.0 / (2.0 * std::sin(M_PI / 5.0));
    auto pentagon = generate_regular_polygon(5, R);
    assert(dsa::MinimumWeightTriangulation::is_convex(pentagon));

    auto res = dsa::solve_minimum_weight_triangulation(pentagon);
    const double expected_weight = 2.0 * phi;
    assert(std::abs(res.total_weight - expected_weight) < EPS);
    assert(res.diagonal_count() == 2);
    assert(res.triangle_count() == 3);
    assert(res.is_valid_triangulation(5));
  }

  // Test 4: Custom Metric Callable interface
  {
    // Custom distance function for n = 5
    auto custom_dist = [](std::size_t u, std::size_t v) noexcept -> double {
      return static_cast<double>((u + 1) * (v + 1));
    };

    auto res = dsa::MinimumWeightTriangulation::solve(5, custom_dist);
    assert(res.diagonal_count() == 2);
    assert(res.triangle_count() == 3);
    assert(res.is_valid_triangulation(5));

    // Recompute weight from reconstructed diagonals to verify internal consistency
    double verified_weight = 0.0;
    for (const auto &d : res.diagonals) {
      verified_weight += custom_dist(d.u, d.v);
    }
    assert(std::abs(res.total_weight - verified_weight) < EPS);
  }

  std::cout << "    [+] Functional and analytical tests passed.\n";
}

void test_oracle_brute_force_equivalence() {
  std::cout
      << "[*] Verifying against recursive brute-force oracle (n = 4 to 8)...\n";

  std::mt19937_64 rng(42);
  std::uniform_real_distribution<double> dist(-50.0, 50.0);

  for (std::size_t n = 4; n <= 8; ++n) {
    // Generate random strictly convex polygon by sorting angles around center
    auto poly = generate_regular_polygon(n, 10.0);
    // Slightly jitter radii while maintaining strict convexity
    for (std::size_t i = 0; i < n; ++i) {
      const double angle =
          2.0 * M_PI * static_cast<double>(i) / static_cast<double>(n);
      const double r = 10.0 + 0.5 * (dist(rng) / 50.0); // tiny jitter
      poly[i] = {r * std::cos(angle), r * std::sin(angle)};
    }

    assert(dsa::MinimumWeightTriangulation::is_convex(poly));

    auto dist_fn = [&poly](std::size_t u, std::size_t v) {
      return poly[u].distance_to(poly[v]);
    };

    const double oracle_cost = brute_force_mwt(0, n - 1, n, dist_fn);
    auto dp_res = dsa::solve_minimum_weight_triangulation(poly);

    assert(std::abs(dp_res.total_weight - oracle_cost) < EPS);
    assert(dp_res.is_valid_triangulation(n));

    // Ensure sum of returned diagonal lengths equals reported total weight
    double check_sum = 0.0;
    for (const auto &d : dp_res.diagonals) {
      check_sum += poly[d.u].distance_to(poly[d.v]);
    }
    assert(std::abs(dp_res.total_weight - check_sum) < EPS);
  }

  std::cout << "    [+] Brute-force equivalence verified successfully.\n";
}

void test_stress_and_performance() {
  std::cout << "[*] Running stress and performance benchmark...\n";

  const std::vector<std::size_t> test_sizes = {50, 100, 200};

  for (const auto n : test_sizes) {
    auto poly = generate_regular_polygon(n, 100.0);

    const auto start = std::chrono::high_resolution_clock::now();
    auto res = dsa::solve_minimum_weight_triangulation(poly);
    const auto end = std::chrono::high_resolution_clock::now();

    const auto duration_ms =
        std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
            end - start)
            .count();

    assert(res.diagonal_count() == n - 3);
    assert(res.triangle_count() == n - 2);
    assert(res.is_valid_triangulation(n));

    std::cout << "    -> Polygon N = " << std::setw(3) << n
              << " | MWT Weight = " << std::fixed << std::setprecision(4)
              << res.total_weight << " | Diagonals = " << res.diagonal_count()
              << " | Elapsed Time = " << std::setprecision(2) << duration_ms
              << " ms\n";
  }

  std::cout << "    [+] Stress and benchmark tests completed successfully.\n";
}

} // namespace

int main() {
  std::cout << "========================================================\n";
  std::cout << "  C-12.5: Minimum-Weight Convex Polygon Triangulation   \n";
  std::cout << "========================================================\n\n";

  test_edge_cases();
  test_functional_and_analytical_cases();
  test_oracle_brute_force_equivalence();
  test_stress_and_performance();

  std::cout
      << "\n[SUCCESS] All unit tests, assertions, and benchmarks passed!\n";
  return 0;
}
