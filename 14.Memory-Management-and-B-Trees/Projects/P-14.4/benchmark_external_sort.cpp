/**
 * @file benchmark_external_sort.cpp
 * @brief Experimental comparison of extsort::external_merge_sorter against
 *        the internal-memory `std::sort`.
 *
 * @details
 * For every pair (n, memory) the benchmark sorts the same pseudo-random
 * `uint64_t` data three ways and prints one table row:
 *  - `ext`      : external merge sort, file -> file (includes all disk I/O);
 *  - `sort_ram` : `std::sort` on a vector already in RAM (pure CPU bound);
 *  - `sort+io`  : read file into RAM, `std::sort`, write file (the fair
 *                 "internal" counterpart, only possible when data fits RAM).
 * It also reports the number of runs, merge passes, the measured block
 * transfers and the I/O-model prediction $2\,(n/B)(1+P)$.
 *
 * Build (example):
 *   g++ -std=c++17 -O2 -DNDEBUG benchmark_external_sort.cpp -o bench
 *
 * Usage:
 *   ./bench [--sizes=1000000,4000000,16000000] [--mem-mib=1,4,16]
 *           [--dir=/path/on/the/disk/to/test] [--seed=42]
 *
 * @note The OS page cache hides most disk latency when the data set fits in
 *       RAM. To observe real I/O costs use sizes larger than free RAM, a
 *       `--dir` on the target device, or drop caches between runs
 *       (`echo 3 | sudo tee /proc/sys/vm/drop_caches` on Linux).
 */
#include "external_merge_sort.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using clock_type = std::chrono::steady_clock;

namespace {

double seconds_since(clock_type::time_point t0) {
  return std::chrono::duration<double>(clock_type::now() - t0).count();
}

std::vector<std::size_t> parse_list(const std::string &s) {
  std::vector<std::size_t> out;
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, ','))
    if (!item.empty())
      out.push_back(static_cast<std::size_t>(std::stoull(item)));
  return out;
}

} // namespace

int main(int argc, char **argv) {
  std::vector<std::size_t> sizes = {1'000'000, 4'000'000, 16'000'000};
  std::vector<std::size_t> mem_mib = {1, 4, 16};
  fs::path dir = fs::temp_directory_path();
  std::uint64_t seed = 42;

  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--sizes=", 0) == 0)
      sizes = parse_list(a.substr(8));
    else if (a.rfind("--mem-mib=", 0) == 0)
      mem_mib = parse_list(a.substr(10));
    else if (a.rfind("--dir=", 0) == 0)
      dir = a.substr(6);
    else if (a.rfind("--seed=", 0) == 0)
      seed = std::stoull(a.substr(7));
    else {
      std::fprintf(stderr, "unknown argument: %s\n", a.c_str());
      return 2;
    }
  }

  const fs::path work =
      dir / ("extsort_bench_" + std::to_string(std::random_device{}()));
  fs::create_directories(work);
  const fs::path in = work / "in.bin";
  const fs::path out = work / "out.bin";
  const fs::path out2 = work / "out_ram.bin";

  std::printf("%-11s %-8s %-6s %-7s %-10s %-10s %-10s %-9s %-12s %-12s\n", "n",
              "mem(MiB)", "runs", "passes", "ext(s)", "sort_ram(s)",
              "sort+io(s)", "ext/ram", "io_measured", "io_model");

  int exit_code = 0;
  for (const std::size_t n : sizes) {
    // Generate the data once per n and keep a copy in RAM for the baselines.
    std::mt19937_64 rng(seed);
    std::vector<std::uint64_t> data(n);
    for (auto &x : data)
      x = rng();
    extsort::write_records(in, data.data(), n);

    // Baseline 1: std::sort on data already in RAM.
    std::vector<std::uint64_t> ram = data;
    auto t0 = clock_type::now();
    std::sort(ram.begin(), ram.end());
    const double t_ram = seconds_since(t0);

    // Baseline 2: file -> RAM -> std::sort -> file.
    t0 = clock_type::now();
    {
      auto v = extsort::read_records<std::uint64_t>(in);
      std::sort(v.begin(), v.end());
      extsort::write_records(out2, v.data(), v.size());
    }
    const double t_ram_io = seconds_since(t0);
    fs::remove(out2);

    for (const std::size_t mib : mem_mib) {
      extsort::config cfg;
      cfg.memory_bytes = mib << 20;
      cfg.block_bytes = std::size_t{64} << 10;
      cfg.temp_dir = work;
      extsort::external_merge_sorter<std::uint64_t> sorter(cfg);

      t0 = clock_type::now();
      const extsort::sort_stats st = sorter.sort_file(in, out);
      const double t_ext = seconds_since(t0);

      // Verify against the in-RAM result.
      const auto got = extsort::read_records<std::uint64_t>(out);
      if (got != ram) {
        std::fprintf(stderr, "MISMATCH for n=%zu mem=%zuMiB\n", n, mib);
        exit_code = 1;
      }
      fs::remove(out);

      const double B = static_cast<double>(sorter.block_elems());
      const double model =
          2.0 * (static_cast<double>(n) / B) * (1.0 + double(st.merge_passes));
      std::printf(
          "%-11zu %-8zu %-6llu %-7llu %-10.3f %-10.3f %-10.3f %-9.2f "
          "%-12llu %-12.0f\n",
          n, mib, static_cast<unsigned long long>(st.initial_runs),
          static_cast<unsigned long long>(st.merge_passes), t_ext, t_ram,
          t_ram_io, t_ext / t_ram,
          static_cast<unsigned long long>(st.block_reads + st.block_writes),
          model);
      std::fflush(stdout);
    }
    fs::remove(in);
  }

  std::error_code ec;
  fs::remove_all(work, ec);
  std::printf("\n(ext/ram = external time divided by in-RAM std::sort time; "
              "see the file header for page-cache caveats.)\n");
  return exit_code;
}
