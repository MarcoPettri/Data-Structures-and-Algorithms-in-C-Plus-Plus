// File: include/Packet.hpp
/*
  Packet Class Interface
*/

#ifndef PACKET_H
#define PACKET_H

#include <chrono>
#include <string>

// An immutable unit of data travelling from a source host to a destination
// host.
class Packet {
public:
  using Clock = std::chrono::steady_clock;

  Packet(int id, std::string source, std::string destination,
         std::string payload);

  int id() const;
  const std::string &source() const;
  const std::string &destination() const;
  const std::string &payload() const;

  // Time elapsed since this packet was created.
  std::chrono::milliseconds age() const;

  std::string to_string() const;

private:
  int id_;
  std::string source_;
  std::string destination_;
  std::string payload_;
  Clock::time_point createdAt_;
};

#endif // PACKET_H