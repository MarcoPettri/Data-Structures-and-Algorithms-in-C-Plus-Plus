// File : P-10.1.cpp

/*
    P-10.1 Write a program that performs a simple n-body simulation, called “Jump-
    ing Leprechauns.” This simulation involves n leprechauns, numbered 1 to
    n. It maintains a gold value gi for each leprechaun i, which begins with
    each leprechaun starting out with a million dollars worth of gold, that is,
    gi = 1 000 000 for each i = 1, 2, . . . , n. In addition, the simulation also
    maintains, for each leprechaun, i, a place on the horizon, which is repre-
    sented as a double-precision floating point number, xi. In each iteration
    of the simulation, the simulation processes the leprechauns in order. Pro-
    cessing a leprechaun i during this iteration begins by computing a new
    place on the horizon for i, which is determined by the assignment
    x_i <-- x_i + rg_i,
    where r is a random floating-point number between −1 and 1. The lep-
    rechaun i then steals half the gold from the nearest leprechauns on either
    side of him and adds this gold to his gold value, gi. Write a program that
    can perform a series of iterations in this simulation for a given number, n,
    of leprechauns. You must maintain the set of horizon positions using an
    ordered map data structure.
*/
/**
 * @file main.cpp
 * @brief Command line driver for the "Jumping Leprechauns" simulation.
 *
 * Usage: `jumping_leprechauns <n> <iterations> [seed]`
 */
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <system_error>

#include "leprechaun_simulation.hpp"

namespace {

/// Parses an unsigned integer; returns false on any malformed input.
bool parseUnsigned(std::string_view text, std::uint64_t &out) {
  const auto *first = text.data();
  const auto *last = text.data() + text.size();
  const auto [ptr, ec] = std::from_chars(first, last, out);
  return ec == std::errc{} && ptr == last;
}

}  // namespace

int main(int argc, char **argv) {
  if (argc < 3 || argc > 4) {
    std::cerr << "Usage: " << argv[0] << " <n> <iterations> [seed]\n";
    return EXIT_FAILURE;
  }

  std::uint64_t n = 0;
  std::uint64_t iterations = 0;
  std::uint64_t seed = leprechaun::Simulation::Engine::default_seed;
  if (!parseUnsigned(argv[1], n) || n == 0 ||
      !parseUnsigned(argv[2], iterations) ||
      (argc == 4 && !parseUnsigned(argv[3], seed))) {
    std::cerr << "Error: arguments must be non-negative integers (n >= 1).\n";
    return EXIT_FAILURE;
  }

  try {
    leprechaun::Simulation sim(static_cast<std::size_t>(n), seed);
    sim.run(static_cast<std::size_t>(iterations));

    double richest = sim.gold(0);
    double poorest = sim.gold(0);
    for (std::size_t i = 1; i < sim.size(); ++i) {
      richest = std::max(richest, sim.gold(i));
      poorest = std::min(poorest, sim.gold(i));
    }

    std::cout << std::fixed << std::setprecision(2)
              << "Leprechauns:  " << sim.size() << '\n'
              << "Iterations:   " << sim.iterations() << '\n'
              << "Total gold:   " << sim.totalGold() << '\n'
              << "Richest gold: " << richest << '\n'
              << "Poorest gold: " << poorest << '\n';
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}