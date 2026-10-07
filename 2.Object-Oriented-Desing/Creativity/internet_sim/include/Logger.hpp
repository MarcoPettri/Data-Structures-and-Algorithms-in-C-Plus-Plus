// File: include/Logger.hpp
/*
  Logger Class Interface
*/

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>

// Thread-safe, timestamped console logger.
class Logger {
public:
  // Writes "[  1234 ms] <who>: <message>" as a single atomic line.
  static void log(const std::string &who, const std::string &message);

private:
  Logger() = delete;
};

#endif // LOGGER_HPP