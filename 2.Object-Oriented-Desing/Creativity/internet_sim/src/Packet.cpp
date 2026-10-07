// File: src/Packet.cpp
/*
  Packet Class Implementation
*/

#include "Packet.hpp"

#include <utility>

Packet::Packet(int id, std::string source, std::string destination,
               std::string payload)
    : id_(id), source_(std::move(source)), destination_(std::move(destination)),
      payload_(std::move(payload)), createdAt_(Clock::now()) {}

int Packet::id() const { return id_; }
const std::string &Packet::source() const { return source_; }
const std::string &Packet::destination() const { return destination_; }
const std::string &Packet::payload() const { return payload_; }

std::chrono::milliseconds Packet::age() const {
  return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() -
                                                               createdAt_);
}

std::string Packet::to_string() const {
  return "Packet#" + std::to_string(id_) + " [" + source_ + " -> " +
         destination_ + ", \"" + payload_ + "\"]";
}