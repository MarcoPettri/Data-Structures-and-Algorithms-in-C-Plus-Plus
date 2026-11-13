/**
 * @file fasta.cpp
 * @brief Implementation of the FASTA reader.
 */

#include "fasta.hpp"

#include <cctype>
#include <fstream>
#include <istream>
#include <stdexcept>
#include <string_view>

namespace bio {

namespace {

[[nodiscard]] bool isSpace(char c) noexcept {
  return std::isspace(static_cast<unsigned char>(c)) != 0;
}

/// Splits a header (without '>') into identifier and description.
void parseHeader(std::string_view header, FastaRecord &rec) {
  std::size_t end = 0;
  while (end < header.size() && !isSpace(header[end])) {
    ++end;
  }
  rec.id.assign(header.substr(0, end));

  std::size_t begin = end;
  while (begin < header.size() && isSpace(header[begin])) {
    ++begin;
  }
  rec.description.assign(header.substr(begin));
}

}  // namespace

std::vector<FastaRecord> readFasta(std::istream &in) {
  std::vector<FastaRecord> records;
  std::string line;
  std::size_t lineNo = 0;

  while (std::getline(in, line)) {
    ++lineNo;
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (line.empty() || line.front() == ';') {
      continue;
    }

    if (line.front() == '>') {
      records.emplace_back();
      parseHeader(std::string_view(line).substr(1), records.back());
      continue;
    }

    if (records.empty()) {
      throw std::runtime_error(
          "FASTA: sequence data before first header (line " +
          std::to_string(lineNo) + ")");
    }

    std::string &seq = records.back().sequence;
    for (const char c : line) {
      if (isSpace(c) || c == '-' || c == '*') {
        continue;
      }
      if (!std::isalpha(static_cast<unsigned char>(c))) {
        throw std::runtime_error("FASTA: invalid character '" +
                                 std::string(1, c) + "' on line " +
                                 std::to_string(lineNo));
      }
      seq.push_back(
          static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }
  }
  return records;
}

std::vector<FastaRecord> readFastaFile(const std::string &path) {
  std::ifstream file(path);
  if (!file) {
    throw std::runtime_error("FASTA: cannot open file '" + path + "'");
  }
  return readFasta(file);
}

}  // namespace bio