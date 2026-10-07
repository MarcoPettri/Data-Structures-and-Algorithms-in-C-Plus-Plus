// File: src/Internet.cpp
/*
  Internet Class Implementation
*/

#include "Internet.hpp"

#include <vector>

Internet::Internet(Computer &from, Computer &to,
                   std::chrono::milliseconds pollPeriod)
    : PeriodicProcess("Internet", pollPeriod), from_(from), to_(to),
      totalDelivered_(0) {}

Internet::~Internet() { stop(); }

int Internet::total_delivered() const { return totalDelivered_.load(); }

void Internet::step() {
  // Atomically grab everything Alice has queued, then hand it to Bob.
  const std::vector<Packet> packets = from_.take_all_out_going();
  for (const Packet &packet : packets) {
    to_.deliver(packet);
    ++totalDelivered_;
    log("delivers " + packet.to_string());
  }
}