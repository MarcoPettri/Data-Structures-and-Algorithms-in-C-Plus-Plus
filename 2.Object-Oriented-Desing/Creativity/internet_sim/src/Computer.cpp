// File: src/Computer.cpp
/*
  Computer Class Implementation
*/

#include "Computer.hpp"

#include <iterator>
#include <utility>

Computer::Computer(std::string name) : name_(std::move(name)) {}

const std::string &Computer::name() const { return name_; }

void Computer::queue_out_going(const Packet &packet) {
  std::lock_guard<std::mutex> lock(outboxMutex_);
  outbox_.push_back(packet);
}

std::vector<Packet> Computer::take_all_out_going() {
  std::lock_guard<std::mutex> lock(outboxMutex_);
  std::vector<Packet> out(std::make_move_iterator(outbox_.begin()),
                          std::make_move_iterator(outbox_.end()));
  outbox_.clear();
  return out;
}

std::size_t Computer::out_going_count() const {
  std::lock_guard<std::mutex> lock(outboxMutex_);
  return outbox_.size();
}

void Computer::deliver(const Packet &packet) {
  std::lock_guard<std::mutex> lock(inboxMutex_);
  inbox_.push_back(packet);
}

std::vector<Packet> Computer::take_all_in_coming() {
  std::lock_guard<std::mutex> lock(inboxMutex_);
  std::vector<Packet> in(std::make_move_iterator(inbox_.begin()),
                         std::make_move_iterator(inbox_.end()));
  inbox_.clear();
  return in;
}

std::size_t Computer::in_coming_count() const {
  std::lock_guard<std::mutex> lock(inboxMutex_);
  return inbox_.size();
}