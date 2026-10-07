// File: include/Bob.hpp
/*
  Bob Class Interface
*/

#ifndef BOB_H
#define BOB_H

#include <atomic>
#include <chrono>

#include "Computer.hpp"
#include "PeriodicProcess.hpp"

// Bob periodically checks his computer for packets. If any have arrived he
// reads each one and deletes it (removal from the inbox is the deletion).
class Bob : public PeriodicProcess {
public:
  Bob(Computer &machine, std::chrono::milliseconds checkPeriod);
  ~Bob() override;

  int total_read() const;

protected:
  void step() override;

private:
  Computer &machine_;
  std::atomic<int> totalRead_;
};

#endif // BOB_H