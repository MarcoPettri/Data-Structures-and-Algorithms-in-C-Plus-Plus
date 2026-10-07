// File: include/Alice.hpp
/*
  Alice Class Interface
*/
#ifndef ALICE_H
#define ALICE_H

#include <atomic>
#include <chrono>
#include <random>
#include <string>

#include "Computer.hpp"
#include "PeriodicProcess.hpp"

// Alice periodically creates a random-sized set of packets addressed to a
// recipient and places them on her computer's outbox.
class Alice : public PeriodicProcess {
public:
  Alice(Computer &machine, std::string recipient,
        std::chrono::milliseconds period, int maxPacketsPerBurst,
        unsigned seed = std::random_device{}());
  ~Alice() override;

  int total_created() const;

protected:
  void step() override;

private:
  Computer &machine_;
  std::string recipient_;
  int maxPacketsPerBurst_;
  int nextId_;
  std::mt19937 rng_;
  std::atomic<int> totalCreated_;
};

#endif // ALICE_H