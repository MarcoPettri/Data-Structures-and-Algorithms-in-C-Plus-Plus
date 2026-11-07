/**
 * @file test_leprechaun_simulation.cpp
 * @brief Dependency-free unit tests for leprechaun::Simulation.
 *
 * Build it as its own executable together with leprechaun_simulation.cpp.
 * The process exit code is the number of failed checks (0 = success).
 */
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

#include "leprechaun_simulation.hpp"

namespace {

using leprechaun::Simulation;

int g_failures = 0;
int g_checks = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        ++g_checks;                                                              \
        if (!(cond)) {                                                           \
            ++g_failures;                                                        \
            std::cerr << __FILE__ << ':' << __LINE__ << ": CHECK failed: " #cond \
                      << '\n';                                                   \
        }                                                                        \
    } while (false)

#define CHECK_THROWS(expr, ExceptionType)                                        \
    do {                                                                         \
        ++g_checks;                                                              \
        bool caught = false;                                                     \
        try {                                                                    \
            (void)(expr);                                                        \
        } catch (const ExceptionType&) {                                         \
            caught = true;                                                       \
        } catch (...) {                                                          \
        }                                                                        \
        if (!caught) {                                                           \
            ++g_failures;                                                        \
            std::cerr << __FILE__ << ':' << __LINE__                             \
                      << ": expected " #ExceptionType " from " #expr << '\n';    \
        }                                                                        \
    } while (false)

constexpr double kG = Simulation::kInitialGold;

bool nearlyEqual(double a, double b, double relTol = 1e-9) {
    return std::fabs(a - b) <= relTol * std::max({1.0, std::fabs(a), std::fabs(b)});
}

void testConstruction() {
    Simulation sim(5);
    CHECK(sim.size() == 5);
    CHECK(sim.iterations() == 0);
    for (std::size_t i = 0; i < 5; ++i) {
        CHECK(sim.gold(i) == kG);
        CHECK(sim.position(i) == static_cast<double>(i));
    }
    CHECK(nearlyEqual(sim.totalGold(), 5 * kG));

    CHECK_THROWS(Simulation(0), std::invalid_argument);
    CHECK_THROWS(Simulation(std::vector<double>{}), std::invalid_argument);
    CHECK_THROWS(Simulation(std::vector<double>{0.0, INFINITY}), std::invalid_argument);
    CHECK_THROWS(Simulation(std::vector<double>{NAN}), std::invalid_argument);
}

void testIndexChecks() {
    Simulation sim(3);
    CHECK_THROWS(sim.gold(3), std::out_of_range);
    CHECK_THROWS(sim.position(3), std::out_of_range);
    CHECK_THROWS(sim.jump(3, 0.0), std::out_of_range);
}

void testStealsFromBothNeighbours() {
    Simulation sim(std::vector<double>{0.0, 10.0, 20.0});
    sim.jump(1, 0.0);  // stays in place, neighbours are 0 and 2
    CHECK(sim.gold(0) == kG / 2);
    CHECK(sim.gold(2) == kG / 2);
    CHECK(sim.gold(1) == 2 * kG);
    CHECK(sim.position(1) == 10.0);
}

void testEdgeLeprechaunHasOneNeighbour() {
    Simulation sim(std::vector<double>{0.0, 10.0, 20.0});
    sim.jump(0, 0.0);  // leftmost: only a right neighbour
    CHECK(sim.gold(0) == 1.5 * kG);
    CHECK(sim.gold(1) == kG / 2);
    CHECK(sim.gold(2) == kG);

    Simulation sim2(std::vector<double>{0.0, 10.0, 20.0});
    sim2.jump(2, 0.0);  // rightmost: only a left neighbour
    CHECK(sim2.gold(2) == 1.5 * kG);
    CHECK(sim2.gold(1) == kG / 2);
    CHECK(sim2.gold(0) == kG);
}

void testJumpChangesNeighbourhood() {
    Simulation sim(std::vector<double>{0.0, 10.0, 20.0});
    sim.jump(0, 0.5);  // x0 = 0 + 0.5 * 1e6, now to the right of everyone
    CHECK(sim.position(0) == 0.5 * kG);
    CHECK((sim.orderedIndices() == std::vector<std::size_t>{1, 2, 0}));
    // Only a left neighbour (leprechaun 2) exists after the jump.
    CHECK(sim.gold(0) == 1.5 * kG);
    CHECK(sim.gold(2) == kG / 2);
    CHECK(sim.gold(1) == kG);

    Simulation sim2(std::vector<double>{0.0, 10.0, 20.0});
    sim2.jump(2, -0.5);  // jumps far to the left
    CHECK((sim2.orderedIndices() == std::vector<std::size_t>{2, 0, 1}));
    CHECK(sim2.gold(2) == 1.5 * kG);
    CHECK(sim2.gold(0) == kG / 2);
}

void testSingleLeprechaun() {
    Simulation sim(1);
    sim.run(10);
    CHECK(sim.gold(0) == kG);
    CHECK(sim.iterations() == 10);
}

void testCoincidentPositions() {
    Simulation sim(std::vector<double>{5.0, 5.0, 5.0});
    const double before = sim.totalGold();
    sim.jump(1, 0.0);
    CHECK(nearlyEqual(sim.totalGold(), before));
    CHECK(sim.gold(1) > kG);
    CHECK(sim.orderedIndices().size() == 3);
}

void testInvariantsAfterRun() {
    constexpr std::size_t n = 200;
    Simulation sim(n, 12345);
    sim.run(100);
    CHECK(sim.iterations() == 100);

    // Gold is conserved (rounding only).
    CHECK(nearlyEqual(sim.totalGold(), n * kG, 1e-9));

    // Ordered view is a permutation sorted by position.
    auto order = sim.orderedIndices();
    CHECK(order.size() == n);
    for (std::size_t k = 1; k < order.size(); ++k) {
        CHECK(sim.position(order[k - 1]) <= sim.position(order[k]));
    }
    std::sort(order.begin(), order.end());
    bool permutation = true;
    for (std::size_t i = 0; i < n; ++i) {
        permutation = permutation && (order[i] == i);
    }
    CHECK(permutation);

    // Gold never becomes negative or non-finite.
    for (std::size_t i = 0; i < n; ++i) {
        CHECK(sim.gold(i) >= 0.0 && std::isfinite(sim.gold(i)));
        CHECK(std::isfinite(sim.position(i)));
    }
}

void testDeterminismAndMove() {
    Simulation a(50, 7);
    Simulation b(50, 7);
    a.run(20);
    b.run(20);
    bool same = true;
    for (std::size_t i = 0; i < 50; ++i) {
        same = same && a.gold(i) == b.gold(i) && a.position(i) == b.position(i);
    }
    CHECK(same);

    // Cached iterators must remain valid after a move.
    Simulation c(std::move(a));
    c.run(5);
    CHECK(c.size() == 50);
    CHECK(nearlyEqual(c.totalGold(), 50 * kG));
    CHECK(c.orderedIndices().size() == 50);
}

}  // namespace

int main() {
    testConstruction();
    testIndexChecks();
    testStealsFromBothNeighbours();
    testEdgeLeprechaunHasOneNeighbour();
    testJumpChangesNeighbourhood();
    testSingleLeprechaun();
    testCoincidentPositions();
    testInvariantsAfterRun();
    testDeterminismAndMove();

    std::cout << (g_checks - g_failures) << '/' << g_checks << " checks passed\n";
    return g_failures;
}