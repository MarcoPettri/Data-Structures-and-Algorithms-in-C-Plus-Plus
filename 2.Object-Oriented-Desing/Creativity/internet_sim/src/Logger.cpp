// File: src/Logger.cpp

#include "Logger.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

namespace {
std::mutex &output_mutex() {
  static std::mutex m;
  return m;
}

std::chrono::steady_clock::time_point program_start() {
  static const auto start = std::chrono::steady_clock::now();
  return start;
}
} // namespace

void Logger::log(const std::string &who, const std::string &message) {
  const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now() - program_start())
                           .count();

  // Build the whole line first, then print it under the lock.
  std::ostringstream line;
  line << "[" << std::setw(6) << elapsed << " ms] " << std::left << std::setw(8)
       << who << std::right << message << '\n';

  std::lock_guard<std::mutex> lock(output_mutex());
  std::cout << line.str() << std::flush;
}