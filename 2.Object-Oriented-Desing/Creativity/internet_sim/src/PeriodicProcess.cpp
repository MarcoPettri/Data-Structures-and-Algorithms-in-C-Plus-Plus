// File: src/PeriodicProcess.cpp
/*
  PeriodicProcess Class Implementation
*/

#include "PeriodicProcess.hpp"

#include <utility>

#include "Logger.hpp"

PeriodicProcess::PeriodicProcess(std::string name,
                                 std::chrono::milliseconds period)
    : name_(std::move(name)), period_(period), stopRequested_(false) {}

PeriodicProcess::~PeriodicProcess() { stop(); }

void PeriodicProcess::start() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (thread_.joinable())
    return; // already running
  stopRequested_ = false;
  thread_ = std::thread(&PeriodicProcess::run, this);
}

void PeriodicProcess::stop() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    stopRequested_ = true;
  }
  wakeup_.notify_all();
  if (thread_.joinable())
    thread_.join();
}

const std::string &PeriodicProcess::name() const { return name_; }

void PeriodicProcess::log(const std::string &message) const {
  Logger::log(name_, message);
}

void PeriodicProcess::run() {
  std::unique_lock<std::mutex> lock(mutex_);
  while (!stopRequested_) {
    lock.unlock();
    step(); // do the work without holding the control mutex
    lock.lock();
    // Sleep for one period, but wake immediately if stop() is called.
    wakeup_.wait_for(lock, period_, [this] { return stopRequested_; });
  }
}