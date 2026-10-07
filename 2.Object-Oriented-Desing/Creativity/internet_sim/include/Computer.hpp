// File: include/Computer.hpp
/**
 * Computer Class Interface
 */
#ifndef COMPUTER_H
#define COMPUTER_H

#include <cstddef>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

#include "Packet.hpp"

// A host on the network. Thread-safe: its owner, the Internet process, and the
// receiving party may all touch it concurrently.
//
//   outbox: packets the owner has created and wants the network to carry away
//   inbox : packets the network has delivered but the owner has not read yet
class Computer {
public:
  explicit Computer(std::string name);

  // Not copyable (contains mutexes).
  Computer(const Computer &) = delete;
  Computer &operator=(const Computer &) = delete;

  const std::string &name() const;

  // --- outbox ---
  void queue_out_going(const Packet &packet);
  // Atomically removes and returns every outgoing packet, oldest first.
  std::vector<Packet> take_all_out_going();
  std::size_t out_going_count() const;

  // --- inbox ---
  void deliver(const Packet &packet);
  // Atomically removes and returns every incoming packet, oldest first.
  std::vector<Packet> take_all_in_coming();
  std::size_t in_coming_count() const;

private:
  std::string name_;

  mutable std::mutex outboxMutex_;
  std::deque<Packet> outbox_;

  mutable std::mutex inboxMutex_;
  std::deque<Packet> inbox_;
};

#endif // COMPUTER_H