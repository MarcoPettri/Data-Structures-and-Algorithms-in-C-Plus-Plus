/**
 * @file test_external_merge_sort.cpp
 * @brief Self-contained unit, stress and performance-sanity tests for
 *        extsort::external_merge_sorter.
 *
 * @details
 * Build (example):
 *   g++ -std=c++17 -O1 -g -Wall -Wextra -fsanitize=address,undefined \
 *       test_external_merge_sort.cpp -o test_external_merge_sort
 *
 * The harness uses its own CHECK macros (not `assert`) so the checks stay
 * active under `-DNDEBUG`. Set `EXTSORT_SKIP_TIMING=1` to skip the (loose)
 * wall-clock assertion in the performance test on noisy machines; the
 * deterministic block-transfer assertions always run.
 */
#include "external_merge_sort.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

int g_checks = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    ++g_checks;                                                                \
    if (!(cond)) {                                                             \
      std::fprintf(stderr, "CHECK failed: %s (%s:%d)\n", #cond, __FILE__,      \
                   __LINE__);                                                  \
      std::abort();                                                            \
    }                                                                          \
  } while (0)

#define CHECK_THROWS_AS(expr, ExType)                                          \
  do {                                                                         \
    ++g_checks;                                                                \
    bool caught_ = false;                                                      \
    try {                                                                      \
      (void)(expr);                                                            \
    } catch (const ExType &) {                                                 \
      caught_ = true;                                                          \
    } catch (...) {                                                            \
    }                                                                          \
    if (!caught_) {                                                            \
      std::fprintf(stderr, "CHECK_THROWS_AS failed: %s as %s (%s:%d)\n",       \
                   #expr, #ExType, __FILE__, __LINE__);                        \
      std::abort();                                                            \
    }                                                                          \
  } while (0)

/// Per-test scratch directory removed on destruction.
class scratch_dir {
public:
  scratch_dir()
      : root_(fs::temp_directory_path() /
              ("extsort_test_" + std::to_string(std::random_device{}()))) {
    fs::create_directories(root_ / "tmp");
  }
  scratch_dir(const scratch_dir &) = delete;
  scratch_dir &operator=(const scratch_dir &) = delete;
  ~scratch_dir() {
    std::error_code ec;
    fs::remove_all(root_, ec);
  }
  fs::path file(const std::string &name) const { return root_ / name; }
  fs::path tmp() const { return root_ / "tmp"; }
  bool tmp_is_empty() const { return fs::is_empty(tmp()); }

private:
  fs::path root_;
};

struct record {
  std::uint32_t key;
  std::uint32_t seq;
  double payload;
};
struct by_key {
  bool operator()(const record &a, const record &b) const {
    return a.key < b.key;
  }
};

template <typename V>
extsort::config small_cfg(std::size_t mem_elems, std::size_t block_elems,
                          std::size_t max_fan_in = 0) {
  extsort::config c;
  c.memory_bytes = mem_elems * sizeof(V);
  c.block_bytes = block_elems * sizeof(V);
  c.max_fan_in = max_fan_in;
  return c;
}

template <typename V>
bool same_bytes(const std::vector<V> &a, const std::vector<V> &b) {
  return a.size() == b.size() &&
         (a.empty() ||
          std::memcmp(a.data(), b.data(), a.size() * sizeof(V)) == 0);
}

/// Expected number of merge passes for r runs and fan-in k: ceil(log_k r).
std::uint64_t expected_passes(std::uint64_t r, std::uint64_t k) {
  if (r <= 1)
    return 0;
  std::uint64_t p = 0;
  while (r > k) {
    r = (r + k - 1) / k;
    ++p;
  }
  return p + 1;
}

/// Sorts `data` externally and checks the result against std::stable_sort,
/// that the input is untouched and that no temporary file is left behind.
template <typename V, typename C = std::less<V>>
extsort::sort_stats run_and_check(const scratch_dir &sd,
                                  const std::vector<V> &data,
                                  extsort::config cfg, C comp = C()) {
  cfg.temp_dir = sd.tmp();
  const fs::path in = sd.file("in.bin");
  const fs::path out = sd.file("out.bin");
  extsort::write_records(in, data.data(), data.size());

  extsort::external_merge_sorter<V, C> sorter(cfg, comp);
  const extsort::sort_stats st = sorter.sort_file(in, out);

  std::vector<V> expect = data;
  std::stable_sort(expect.begin(), expect.end(), comp);
  CHECK(same_bytes(extsort::read_records<V>(out), expect));
  CHECK(same_bytes(extsort::read_records<V>(in), data)); // input untouched
  CHECK(st.elements == data.size());
  CHECK(sd.tmp_is_empty());
  CHECK(!fs::exists(fs::path(out.string() + ".extsort.tmp")));
  return st;
}

std::vector<std::int64_t> random_ints(std::mt19937_64 &rng, std::size_t n,
                                      std::int64_t lo, std::int64_t hi) {
  std::uniform_int_distribution<std::int64_t> d(lo, hi);
  std::vector<std::int64_t> v(n);
  for (auto &x : v)
    x = d(rng);
  return v;
}

// ---------------------------------------------------------------- tests ---

void test_empty_and_tiny() {
  scratch_dir sd;
  std::mt19937_64 rng(1);
  for (std::size_t n : {0u, 1u, 2u, 3u, 4u, 5u}) {
    auto data = random_ints(rng, n, -50, 50);
    const auto st = run_and_check(sd, data, small_cfg<std::int64_t>(16, 2));
    CHECK(st.initial_runs == (n == 0 ? 0u : 1u));
    CHECK(st.merge_passes == 0);
  }
}

void test_run_boundaries_and_pass_counts() {
  // M = 16, B = 2 -> run_elems = 14, fan-in = 7.
  scratch_dir sd;
  std::mt19937_64 rng(2);
  struct row {
    std::size_t n, runs, passes;
  };
  const row rows[] = {{14, 1, 0}, {15, 2, 1}, {28, 2, 1},  {29, 3, 1},
                      {98, 7, 1}, {99, 8, 2}, {196, 14, 2}};
  for (const row &r : rows) {
    auto data = random_ints(rng, r.n, -1000, 1000);
    const auto st = run_and_check(sd, data, small_cfg<std::int64_t>(16, 2));
    CHECK(st.initial_runs == r.runs);
    CHECK(st.merge_passes == r.passes);
  }
}

void test_multi_pass_small_fan_in() {
  scratch_dir sd;
  std::mt19937_64 rng(3);
  auto data = random_ints(rng, 1000, -100000, 100000);
  for (std::size_t fan : {2u, 3u, 5u}) {
    const auto st =
        run_and_check(sd, data, small_cfg<std::int64_t>(16, 1, fan));
    CHECK(st.initial_runs == (1000 + 14) / 15); // run_elems = 16 - 1 = 15
    CHECK(st.merge_passes == expected_passes(st.initial_runs, fan));
  }
}

void test_duplicates_and_presorted() {
  scratch_dir sd;
  std::mt19937_64 rng(4);
  const auto cfg = small_cfg<std::int64_t>(32, 4, 3);

  run_and_check(sd, std::vector<std::int64_t>(500, 7), cfg); // all equal
  run_and_check(sd, random_ints(rng, 700, 0, 3), cfg);       // heavy dups

  std::vector<std::int64_t> asc(600);
  std::iota(asc.begin(), asc.end(), -300);
  run_and_check(sd, asc, cfg); // already sorted
  std::reverse(asc.begin(), asc.end());
  run_and_check(sd, asc, cfg); // reverse sorted

  std::vector<std::int64_t> extremes = {INT64_MAX, INT64_MIN, 0, INT64_MAX,
                                        INT64_MIN, -1,        1, INT64_MAX - 1};
  run_and_check(sd, extremes, small_cfg<std::int64_t>(8, 1));
}

void test_custom_comparator() {
  scratch_dir sd;
  std::mt19937_64 rng(5);
  auto data = random_ints(rng, 900, -500, 500);
  run_and_check(sd, data, small_cfg<std::int64_t>(24, 2, 4),
                std::greater<std::int64_t>());
}

void test_stability() {
  scratch_dir sd;
  std::mt19937_64 rng(6);
  std::uniform_int_distribution<std::uint32_t> key(0, 9);
  std::vector<record> data(5000);
  for (std::size_t i = 0; i < data.size(); ++i)
    data[i] = record{key(rng), static_cast<std::uint32_t>(i), 0.5 * i};
  // run_and_check compares bytes against std::stable_sort, which proves
  // stability (all records share only ~10 distinct keys).
  run_and_check(sd, data, small_cfg<record>(64, 4, 3), by_key{});
  run_and_check(sd, data, small_cfg<record>(64, 4), by_key{});
}

void test_in_place() {
  scratch_dir sd;
  std::mt19937_64 rng(7);
  auto data = random_ints(rng, 400, -99, 99);
  const fs::path p = sd.file("same.bin");
  extsort::write_records(p, data.data(), data.size());

  extsort::config cfg = small_cfg<std::int64_t>(16, 2, 3);
  cfg.temp_dir = sd.tmp();
  extsort::external_merge_sorter<std::int64_t> s(cfg);
  s.sort_file(p, p);

  std::sort(data.begin(), data.end());
  CHECK(same_bytes(extsort::read_records<std::int64_t>(p), data));
  CHECK(sd.tmp_is_empty());
}

void test_reuse_sorter_and_stats() {
  scratch_dir sd;
  std::mt19937_64 rng(8);
  extsort::config cfg = small_cfg<std::int64_t>(16, 2);
  cfg.temp_dir = sd.tmp();
  extsort::external_merge_sorter<std::int64_t> s(cfg);
  CHECK(s.run_elems() == 14 && s.fan_in() == 7 && s.block_elems() == 2 &&
        s.memory_elems() == 16);
  for (std::size_t n : {100u, 5u, 0u, 250u}) {
    auto data = random_ints(rng, n, -9, 9);
    extsort::write_records(sd.file("a.bin"), data.data(), data.size());
    const auto st = s.sort_file(sd.file("a.bin"), sd.file("b.bin"));
    CHECK(st.elements == n); // stats are reset on every call
    CHECK(s.stats().elements == n);
    std::sort(data.begin(), data.end());
    CHECK(same_bytes(extsort::read_records<std::int64_t>(sd.file("b.bin")),
                     data));
  }
}

void test_invalid_inputs() {
  scratch_dir sd;
  extsort::config cfg = small_cfg<std::int64_t>(16, 2);
  cfg.temp_dir = sd.tmp();

  // Invalid configurations.
  CHECK_THROWS_AS((extsort::external_merge_sorter<std::int64_t>(
                      small_cfg<std::int64_t>(3, 1))),
                  std::invalid_argument);
  CHECK_THROWS_AS((extsort::external_merge_sorter<std::int64_t>(
                      small_cfg<std::int64_t>(16, 2, 1))),
                  std::invalid_argument);

  extsort::external_merge_sorter<std::int64_t> s(cfg);

  // Missing input.
  CHECK_THROWS_AS(s.sort_file(sd.file("nope.bin"), sd.file("o.bin")),
                  std::exception);
  CHECK(!fs::exists(sd.file("o.bin")));

  // Input whose size is not a multiple of the record size.
  {
    std::ofstream f(sd.file("bad.bin"), std::ios::binary);
    f.write("0123456789abc", 13);
  }
  CHECK_THROWS_AS(s.sort_file(sd.file("bad.bin"), sd.file("o.bin")),
                  std::runtime_error);
  CHECK(!fs::exists(sd.file("o.bin")));
  CHECK(sd.tmp_is_empty());

  // Unwritable output location: input must survive, no temp files leak.
  std::mt19937_64 rng(9);
  auto data = random_ints(rng, 300, 0, 1000);
  extsort::write_records(sd.file("ok.bin"), data.data(), data.size());
  CHECK_THROWS_AS(s.sort_file(sd.file("ok.bin"), sd.file("no_such_dir/o.bin")),
                  std::runtime_error);
  CHECK(
      same_bytes(extsort::read_records<std::int64_t>(sd.file("ok.bin")), data));
  CHECK(sd.tmp_is_empty());

  // Unusable temp directory.
  extsort::config bad = cfg;
  bad.temp_dir = sd.file("missing_tmp_dir");
  extsort::external_merge_sorter<std::int64_t> s2(bad);
  CHECK_THROWS_AS(s2.sort_file(sd.file("ok.bin"), sd.file("o.bin")),
                  std::runtime_error);
  CHECK(!fs::exists(sd.file("o.bin")));
}

void test_exception_from_comparator_cleans_up() {
  scratch_dir sd;
  std::mt19937_64 rng(10);
  auto data = random_ints(rng, 500, 0, 100000);
  extsort::write_records(sd.file("in.bin"), data.data(), data.size());

  int budget = 4000; // throw somewhere in the middle of merging
  auto throwing = [&budget](std::int64_t a, std::int64_t b) {
    if (--budget < 0)
      throw std::logic_error("boom");
    return a < b;
  };
  extsort::config cfg = small_cfg<std::int64_t>(16, 1, 2);
  cfg.temp_dir = sd.tmp();
  extsort::external_merge_sorter<std::int64_t, decltype(throwing)> s(cfg,
                                                                     throwing);
  CHECK_THROWS_AS(s.sort_file(sd.file("in.bin"), sd.file("out.bin")),
                  std::logic_error);
  CHECK(!fs::exists(sd.file("out.bin")));
  CHECK(sd.tmp_is_empty());
  CHECK(
      same_bytes(extsort::read_records<std::int64_t>(sd.file("in.bin")), data));
}

void test_randomized_stress() {
  scratch_dir sd;
  std::mt19937_64 rng(0xC0FFEE); // fixed seed: reproducible
  for (int iter = 0; iter < 250; ++iter) {
    const std::size_t n =
        std::uniform_int_distribution<std::size_t>(0, 3000)(rng);
    const std::size_t mem =
        std::uniform_int_distribution<std::size_t>(4, 200)(rng);
    const std::size_t blk =
        std::uniform_int_distribution<std::size_t>(1, 64)(rng);
    const std::size_t fan_choices[] = {0, 2, 3, 5, 17};
    const std::size_t fan = fan_choices[rng() % 5];
    const std::uint32_t key_range = (rng() % 2) ? 8 : 1000000;

    std::vector<record> data(n);
    for (std::size_t i = 0; i < n; ++i)
      data[i] = record{static_cast<std::uint32_t>(rng() % key_range),
                       static_cast<std::uint32_t>(i), 0.0};

    const auto st =
        run_and_check(sd, data, small_cfg<record>(mem, blk, fan), by_key{});

    // Cross-check the plan against the public accessors.
    extsort::external_merge_sorter<record, by_key> probe(
        small_cfg<record>(mem, blk, fan));
    const std::uint64_t runs =
        n == 0 ? 0 : (n + probe.run_elems() - 1) / probe.run_elems();
    CHECK(st.initial_runs == runs);
    CHECK(st.merge_passes == expected_passes(runs, probe.fan_in()));
  }
}

void test_performance_sanity() {
  using clock = std::chrono::steady_clock;
  scratch_dir sd;
  std::mt19937_64 rng(11);

  struct measurement {
    double seconds;
    std::uint64_t transfers;
  };
  auto measure = [&](std::size_t n) {
    std::vector<std::uint64_t> data(n);
    for (auto &x : data)
      x = rng();
    extsort::config cfg;
    cfg.memory_bytes = 256u << 10; // M = 32 Ki records
    cfg.block_bytes = 16u << 10;   // B = 2 Ki records, fan-in = 15
    cfg.temp_dir = sd.tmp();
    extsort::write_records(sd.file("big_in.bin"), data.data(), n);

    extsort::external_merge_sorter<std::uint64_t> s(cfg);
    const auto t0 = clock::now();
    const auto st = s.sort_file(sd.file("big_in.bin"), sd.file("big_out.bin"));
    const double secs =
        std::chrono::duration<double>(clock::now() - t0).count();

    auto got = extsort::read_records<std::uint64_t>(sd.file("big_out.bin"));
    std::sort(data.begin(), data.end());
    CHECK(got == data);
    CHECK(sd.tmp_is_empty());

    // Deterministic I/O-model check: every pass (run formation counts as
    // pass 0) transfers each record once, plus at most one partial block
    // per run.
    const std::uint64_t B = s.block_elems();
    const std::uint64_t full_blocks = (n + B - 1) / B;
    CHECK(st.merge_passes == expected_passes(st.initial_runs, s.fan_in()));
    const std::uint64_t bound =
        (st.merge_passes + 1) * (full_blocks + st.initial_runs);
    CHECK(st.block_reads <= bound);
    CHECK(st.block_writes <= bound);
    CHECK(st.block_reads >= full_blocks && st.block_writes >= full_blocks);
    return measurement{secs, st.block_reads + st.block_writes};
  };

  const std::size_t n_small = std::size_t{1} << 18; //  2 MiB of data
  const std::size_t n_large = std::size_t{1} << 21; // 16 MiB of data (8x)
  const measurement a = measure(n_small);
  const measurement b = measure(n_large);

  std::printf("  perf: n=%zu -> %.3fs (%llu transfers); n=%zu -> %.3fs (%llu "
              "transfers)\n",
              n_small, a.seconds, static_cast<unsigned long long>(a.transfers),
              n_large, b.seconds, static_cast<unsigned long long>(b.transfers));

  // Transfers grow like (n/B) log_k(n/M): 8x data may add a pass but must
  // stay far from quadratic (64x).
  CHECK(b.transfers <= 8 * a.transfers * 2);
  // Loose wall-clock check (n log n would give about 9-12x, n^2 gives 64x).
  if (std::getenv("EXTSORT_SKIP_TIMING") == nullptr)
    CHECK(b.seconds <= 32.0 * std::max(a.seconds, 1e-3));
}

} // namespace

int main() {
  struct named_test {
    const char *name;
    void (*fn)();
  };
  const named_test tests[] = {
      {"empty_and_tiny", test_empty_and_tiny},
      {"run_boundaries_and_pass_counts", test_run_boundaries_and_pass_counts},
      {"multi_pass_small_fan_in", test_multi_pass_small_fan_in},
      {"duplicates_and_presorted", test_duplicates_and_presorted},
      {"custom_comparator", test_custom_comparator},
      {"stability", test_stability},
      {"in_place", test_in_place},
      {"reuse_sorter_and_stats", test_reuse_sorter_and_stats},
      {"invalid_inputs", test_invalid_inputs},
      {"exception_from_comparator_cleans_up",
       test_exception_from_comparator_cleans_up},
      {"randomized_stress", test_randomized_stress},
      {"performance_sanity", test_performance_sanity},
  };
  for (const auto &t : tests) {
    std::printf("[ RUN  ] %s\n", t.name);
    std::fflush(stdout);
    t.fn();
    std::printf("[ PASS ] %s\n", t.name);
  }
  std::printf("All tests passed (%d checks).\n", g_checks);
  return 0;
}
