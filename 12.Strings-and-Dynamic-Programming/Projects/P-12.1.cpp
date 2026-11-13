// File : P-12.1.cpp

/*
    P-12.1 Implement the LCS algorithm and use it to compute the best sequence
    alignment between some DNA strings that you can get online from Gen-Bank.
*/
/**
 * @file main.cpp
 * @brief Command-line tool: LCS-based alignment of two GenBank FASTA sequences.
 *
 * Usage:
 * @code
 * lcs_align <a.fasta> <b.fasta> [--width N] [--length-only] [--no-alignment] [--out FILE]
 * @endcode
 *
 * Only the first record of each file is used.
 */

#include <chrono>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#include "bio/alignment.hpp"
#include "bio/fasta.hpp"
#include "bio/lcs.hpp"

namespace {

struct Options {
    std::string fileA;
    std::string fileB;
    std::string outFile;
    std::size_t width = 80;
    bool lengthOnly = false;
    bool noAlignment = false;
};

void printUsage(const char* prog)
{
    std::cerr << "Usage: " << prog
              << " <a.fasta> <b.fasta> [--width N] [--length-only] [--no-alignment] [--out FILE]\n";
}

[[nodiscard]] bool parseArgs(int argc, char** argv, Options& opt)
{
    int positional = 0;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--width" && i + 1 < argc) {
            opt.width = static_cast<std::size_t>(std::strtoull(argv[++i], nullptr, 10));
        } else if (arg == "--out" && i + 1 < argc) {
            opt.outFile = argv[++i];
        } else if (arg == "--length-only") {
            opt.lengthOnly = true;
        } else if (arg == "--no-alignment") {
            opt.noAlignment = true;
        } else if (!arg.empty() && arg.front() != '-' && positional < 2) {
            (positional++ == 0 ? opt.fileA : opt.fileB) = arg;
        } else {
            return false;
        }
    }
    return positional == 2;
}

[[nodiscard]] const bio::FastaRecord& firstRecord(const std::vector<bio::FastaRecord>& records,
                                                  const std::string& path)
{
    if (records.empty() || records.front().sequence.empty()) {
        throw std::runtime_error("no sequence found in '" + path + "'");
    }
    return records.front();
}

}  // namespace

int main(int argc, char** argv)
{
    using Clock = std::chrono::steady_clock;

    Options opt;
    if (!parseArgs(argc, argv, opt)) {
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }

    try {
        const auto recordsA = bio::readFastaFile(opt.fileA);
        const auto recordsB = bio::readFastaFile(opt.fileB);
        const auto& a = firstRecord(recordsA, opt.fileA);
        const auto& b = firstRecord(recordsB, opt.fileB);

        std::cout << "A: " << a.id << " (" << a.sequence.size() << " bp) " << a.description << '\n'
                  << "B: " << b.id << " (" << b.sequence.size() << " bp) " << b.description << '\n';

        const auto start = Clock::now();

        if (opt.lengthOnly) {
            const std::size_t len = bio::lcsLength(a.sequence, b.sequence);
            const auto ms =
                std::chrono::duration<double, std::milli>(Clock::now() - start).count();
            std::cout << "LCS length: " << len << "\nTime: " << std::fixed << std::setprecision(1)
                      << ms << " ms\n";
            return EXIT_SUCCESS;
        }

        const auto aln = bio::alignByLcs(a.sequence, b.sequence);
        const auto ms = std::chrono::duration<double, std::milli>(Clock::now() - start).count();

        const std::size_t shorter = std::min(a.sequence.size(), b.sequence.size());
        std::cout << "LCS length:        " << aln.matches << '\n'
                  << "Alignment columns: " << aln.length() << '\n'
                  << "Mismatches:        " << aln.mismatches << '\n'
                  << "Gap columns:       " << aln.gaps << '\n'
                  << std::fixed << std::setprecision(2)
                  << "Column identity:   " << 100.0 * aln.identity() << " %\n"
                  << "LCS / shorter len: "
                  << 100.0 * static_cast<double>(aln.matches) / static_cast<double>(shorter)
                  << " %\n"
                  << std::setprecision(1) << "Time:              " << ms << " ms\n";

        if (opt.noAlignment) {
            return EXIT_SUCCESS;
        }

        if (!opt.outFile.empty()) {
            std::ofstream out(opt.outFile);
            if (!out) {
                throw std::runtime_error("cannot write '" + opt.outFile + "'");
            }
            bio::writeAlignment(out, aln, opt.width);
            std::cout << "Alignment written to " << opt.outFile << '\n';
        } else if (aln.length() > 10000) {
            std::cout << "Alignment is long; use --out FILE to save it (or --no-alignment).\n";
        } else {
            std::cout << '\n';
            bio::writeAlignment(std::cout, aln, opt.width);
        }
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}