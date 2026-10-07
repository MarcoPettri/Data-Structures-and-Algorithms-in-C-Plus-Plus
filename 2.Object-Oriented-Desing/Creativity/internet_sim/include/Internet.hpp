// File: include/Internet.hpp
/*
  Internet Class Interface
*/

#ifndef INTERNET_H
#define INTERNET_H

#include <atomic>
#include <chrono>

#include "Computer.hpp"
#include "PeriodicProcess.hpp"

// The network: continually checks whether the sender's computer has outgoing
// packets and, if so, delivers them to the receiver's computer.
class Internet : public PeriodicProcess {
public:
  Internet(Computer &from, Computer &to, std::chrono::milliseconds pollPeriod);
  ~Internet() override;

  int total_delivered() const;

protected:
  void step() override;

private:
  Computer &from_;
  Computer &to_;
  std::atomic<int> totalDelivered_;
};

#endif // INTERNET_H