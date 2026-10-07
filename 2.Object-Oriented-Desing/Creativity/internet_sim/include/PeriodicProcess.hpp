// File: include/PeriodicProcess.hpp
/*
  PeriodicProcess Class Interface
*/

#ifndef PERIODIC_PROCESS_H
#define PERIODIC_PROCESS_H

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

// Abstract base class for an activity that runs on its own thread and performs
// step() once every `period`. Subclasses implement step().
//
// IMPORTANT: every concrete subclass must call stop() in its own destructor.
// By the time ~PeriodicProcess runs, the derived part of the object is already
// destroyed, so the thread must be joined before that point.
class PeriodicProcess {
public:
  PeriodicProcess(std::string name, std::chrono::milliseconds period);
  virtual ~PeriodicProcess();

  PeriodicProcess(const PeriodicProcess &) = delete;
  PeriodicProcess &operator=(const PeriodicProcess &) = delete;

  // Starts the worker thread. Calling start() on a running process is a no-op.
  void start();

  // Requests termination and joins the thread. Safe to call multiple times.
  void stop();

  const std::string &name() const;

protected:
  // One unit of work, invoked on the worker thread once per period.
  virtual void step() = 0;

  void log(const std::string &message) const;

private:
  void run();

  std::string name_;
  std::chrono::milliseconds period_;

  std::thread thread_;
  std::mutex mutex_;
  std::condition_variable wakeup_;
  bool stopRequested_;
};

#endif // PERIODIC_PROCESS_H