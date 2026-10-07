// File: src/Alice.cpp
/*
  Alice Class Implementation
*/

#include "Alice.hpp"

#include <utility>

Alice::Alice(Computer &machine, std::string recipient,
             std::chrono::milliseconds period, int maxPacketsPerBurst,
             unsigned seed)
    : PeriodicProcess("Alice", period), machine_(machine),
      recipient_(std::move(recipient)), maxPacketsPerBurst_(maxPacketsPerBurst),
      nextId_(1), rng_(seed), totalCreated_(0) {}

Alice::~Alice() { stop(); }

int Alice::total_created() const { return totalCreated_.load(); }

void Alice::step() {
  std::uniform_int_distribution<int> burstSize(1, maxPacketsPerBurst_);
  const int count = burstSize(rng_);

  log("creates " + std::to_string(count) + " packet(s) for " + recipient_);
  for (int i = 0; i < count; ++i) {
    Packet packet(nextId_++, machine_.name(), recipient_,
                  "part " + std::to_string(i + 1) + "/" +
                      std::to_string(count));
    machine_.queue_out_going(packet);
    ++totalCreated_;
  }
}