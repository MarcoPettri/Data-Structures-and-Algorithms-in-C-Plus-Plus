/**
 * @file leprechaun_simulation.cpp
 * @brief Implementation of the "Jumping Leprechauns" simulation.
 */
#include "leprechaun_simulation.hpp"

#include <cmath>
#include <iterator>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace leprechaun {

Simulation::Simulation(std::size_t n, std::uint64_t seed) : engine_(seed) {
  if (n == 0) {
    throw std::invalid_argument("Simulation: need at least one leprechaun");
  }
  std::vector<double> positions(n);
  std::iota(positions.begin(), positions.end(), 0.0);
  initialise(positions);
}

Simulation::Simulation(const std::vector<double> &positions, std::uint64_t seed)
    : engine_(seed) {
  if (positions.empty()) {
    throw std::invalid_argument("Simulation: need at least one leprechaun");
  }
  for (const double x : positions) {
    if (!std::isfinite(x)) {
      throw std::invalid_argument("Simulation: positions must be finite");
    }
  }
  initialise(positions);
}

void Simulation::initialise(const std::vector<double> &positions) {
  const std::size_t n = positions.size();
  gold_.assign(n, kInitialGold);
  nodes_.reserve(n);
  for (Index i = 0; i < n; ++i) {
    nodes_.push_back(map_.emplace(positions[i], i));
  }
}

void Simulation::checkIndex(Index i) const {
  if (i >= gold_.size()) {
    throw std::out_of_range("Simulation: leprechaun index out of range");
  }
}

double Simulation::stealHalf(Index victim) noexcept {
  const double half = gold_[victim] * 0.5;  // exact in binary floating point
  gold_[victim] -= half;
  return half;
}

void Simulation::jump(Index i, double r) {
  checkIndex(i);

  // 1. Move: re-key the existing node (no allocation, O(log n)).
  auto node = map_.extract(nodes_[i]);
  node.key() += r * gold_[i];
  const auto self = map_.insert(std::move(node));
  nodes_[i] = self;

  // 2. Steal half the gold from the nearest neighbour on each side.
  double loot = 0.0;
  if (self != map_.begin()) {
    loot += stealHalf(std::prev(self)->second);
  }
  const auto right = std::next(self);
  if (right != map_.end()) {
    loot += stealHalf(right->second);
  }
  gold_[i] += loot;
}

void Simulation::step() {
  const std::size_t n = gold_.size();
  for (Index i = 0; i < n; ++i) {
    jump(i, dist_(engine_));
  }
  ++iterations_;
}

void Simulation::run(std::size_t iterations) {
  for (std::size_t k = 0; k < iterations; ++k) {
    step();
  }
}

double Simulation::gold(Index i) const {
  checkIndex(i);
  return gold_[i];
}

double Simulation::position(Index i) const {
  checkIndex(i);
  return nodes_[i]->first;
}

double Simulation::totalGold() const noexcept {
  return std::accumulate(gold_.begin(), gold_.end(), 0.0);
}

std::vector<Simulation::Index> Simulation::orderedIndices() const {
  std::vector<Index> order;
  order.reserve(map_.size());
  for (const auto &entry : map_) {
    order.push_back(entry.second);
  }
  return order;
}

}  // namespace leprechaun