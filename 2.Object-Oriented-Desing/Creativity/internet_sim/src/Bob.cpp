// File: src/Bob.cpp
/*
  Bob Class Implementation
*/

#include "Bob.hpp"

#include <vector>

Bob::Bob(Computer &machine, std::chrono::milliseconds checkPeriod)
    : PeriodicProcess("Bob", checkPeriod), machine_(machine), totalRead_(0) {}

Bob::~Bob() { stop(); }

int Bob::total_read() const { return totalRead_.load(); }

void Bob::step() {
  const std::vector<Packet> packets = machine_.take_all_in_coming();
  if (packets.empty()) {
    log("checks inbox: nothing from Alice");
    return;
  }

  log("checks inbox: " + std::to_string(packets.size()) + " packet(s) waiting");
  for (const Packet &packet : packets) {
    ++totalRead_;
    log("read+deleted " + packet.to_string() + " (age " +
        std::to_string(packet.age().count()) + " ms)");
  }
}