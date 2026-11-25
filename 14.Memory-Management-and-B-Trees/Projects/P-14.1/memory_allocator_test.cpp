/**
 * @brief Self-contained assert-based tests for memsim::memory_allocator and
 *        the experiment helpers. Clean under -fsanitize=address,undefined.
 *
 * @details
 * Coverage: construction and invalid input, exact fit, exhaustion, all
 * coalescing cases, policy-specific placement and tie-breaking, statistics,
 * randomized stress against a bitmap-based reference model (fixed seeds, two
 * SizeType instantiations, tiny and medium capacities), trace-generator
 * properties, and a scaling sanity check for the $O(\log h)$ claims.
 * CHECK never compiles away, even under NDEBUG.
 */
#include "fit_experiment.hpp"
#include "memory_allocator.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::fprintf(stderr, "CHECK failed: %s (%s:%d)\n", #cond, __FILE__,      \
                   __LINE__);                                                  \
      std::abort();                                                            \
    }                                                                          \
  } while (0)

#define CHECK_THROWS(expr, exception_type)                                     \
  do {                                                                         \
    bool caught_ = false;                                                      \
    try {                                                                      \
      (void)(expr);                                                            \
    } catch (const exception_type &) {                                         \
      caught_ = true;                                                          \
    }                                                                          \
    CHECK(caught_);                                                            \
  } while (0)

namespace {

using memsim::fit_policy;
using alloc_t = memsim::memory_allocator<std::size_t>;

// ---------------------------------------------------------------------------
// Reference model: one bool per unit; policies implemented by brute force.
// ---------------------------------------------------------------------------
class reference_model {
public:
  struct run {
    std::size_t start;
    std::size_t length;
  };

  reference_model(std::size_t capacity, fit_policy policy)
      : in_use_(capacity, false), policy_(policy) {}

  std::vector<run> free_runs() const {
    std::vector<run> runs;
    std::size_t i = 0;
    while (i < in_use_.size()) {
      if (in_use_[i]) {
        ++i;
        continue;
      }
      const std::size_t start = i;
      while (i < in_use_.size() && !in_use_[i])
        ++i;
      runs.push_back({start, i - start});
    }
    return runs;
  }

  std::optional<std::size_t> allocate(std::size_t size) {
    const auto runs = free_runs();
    std::optional<std::size_t> chosen;
    switch (policy_) {
    case fit_policy::first_fit:
      for (std::size_t i = 0; i < runs.size() && !chosen; ++i)
        if (runs[i].length >= size)
          chosen = i;
      break;
    case fit_policy::best_fit:
      for (std::size_t i = 0; i < runs.size(); ++i)
        if (runs[i].length >= size &&
            (!chosen || runs[i].length < runs[*chosen].length))
          chosen = i;
      break;
    case fit_policy::worst_fit:
      for (std::size_t i = 0; i < runs.size(); ++i)
        if (runs[i].length >= size &&
            (!chosen || runs[i].length > runs[*chosen].length))
          chosen = i;
      break;
    case fit_policy::next_fit: {
      std::size_t k = 0;
      while (k < runs.size() && runs[k].start + runs[k].length <= cursor_)
        ++k;
      for (std::size_t n = 0; n < runs.size() && !chosen; ++n) {
        const std::size_t i = (k + n) % runs.size();
        if (runs[i].length >= size)
          chosen = i;
      }
      break;
    }
    }
    if (!chosen)
      return std::nullopt;
    const std::size_t start = runs[*chosen].start;
    for (std::size_t i = start; i < start + size; ++i)
      in_use_[i] = true;
    used_ += size;
    cursor_ = start + size;
    return start;
  }

  void deallocate(std::size_t address, std::size_t size) {
    for (std::size_t i = address; i < address + size; ++i)
      in_use_[i] = false;
    used_ -= size;
  }

  std::size_t used_bytes() const { return used_; }

private:
  std::vector<bool> in_use_;
  fit_policy policy_;
  std::size_t used_ = 0;
  std::size_t cursor_ = 0;
};

// ---------------------------------------------------------------------------
// Functional tests
// ---------------------------------------------------------------------------
void test_construction_and_invalid_input() {
  CHECK_THROWS((alloc_t(0, fit_policy::first_fit)), std::invalid_argument);
  for (const auto p : memsim::all_fit_policies) {
    alloc_t a(100, p);
    CHECK(a.check_invariants());
    CHECK(a.capacity() == 100 && a.policy() == p);
    CHECK(a.free_bytes() == 100 && a.used_bytes() == 0);
    CHECK(a.hole_count() == 1 && a.largest_hole() == 100);
    CHECK_THROWS(a.allocate(0), std::invalid_argument);
    CHECK_THROWS(a.deallocate(7), std::invalid_argument);
    CHECK(!a.allocate(101));        // too large: nullopt, state unchanged
    CHECK(a.check_invariants() && a.hole_count() == 1);
  }
}

void test_single_block_and_double_free() {
  for (const auto p : memsim::all_fit_policies) {
    alloc_t a(100, p);
    const auto x = a.allocate(100); // boundary: whole memory
    CHECK(x && *x == 0);
    CHECK(a.free_bytes() == 0 && a.hole_count() == 0);
    CHECK(a.block_size(0) == std::optional<std::size_t>(100));
    CHECK(!a.allocate(1));
    CHECK(a.check_invariants());
    a.deallocate(*x);
    CHECK(a.hole_count() == 1 && a.largest_hole() == 100);
    CHECK(!a.block_size(0));
    CHECK_THROWS(a.deallocate(*x), std::invalid_argument); // double free
    CHECK(a.check_invariants());
  }
}

void test_exact_fit_removes_hole() {
  for (const auto p : memsim::all_fit_policies) {
    alloc_t a(30, p);
    const auto x = a.allocate(10);
    const auto y = a.allocate(10);
    const auto z = a.allocate(10);
    CHECK(x && y && z);
    a.deallocate(*y);
    CHECK(a.hole_count() == 1);
    const auto again = a.allocate(10); // exact fit
    CHECK(again && *again == *y);
    CHECK(a.hole_count() == 0 && a.free_bytes() == 0);
    CHECK(a.check_invariants());
  }
}

void test_coalescing_all_cases() {
  for (const auto p : memsim::all_fit_policies) {
    {   // merge with next, then with previous
      alloc_t a(30, p);
      const auto x = *a.allocate(10), y = *a.allocate(10), z = *a.allocate(10);
      a.deallocate(y);
      CHECK(a.hole_count() == 1 && a.largest_hole() == 10);
      a.deallocate(x); // merges with next hole
      CHECK(a.hole_count() == 1 && a.largest_hole() == 20);
      a.deallocate(z); // merges with previous hole
      CHECK(a.hole_count() == 1 && a.largest_hole() == 30);
      CHECK(a.check_invariants());
    }
    {   // merge with both neighbours at once
      alloc_t a(30, p);
      const auto x = *a.allocate(10), y = *a.allocate(10), z = *a.allocate(10);
      a.deallocate(x);
      a.deallocate(z);
      CHECK(a.hole_count() == 2);
      a.deallocate(y);
      CHECK(a.hole_count() == 1 && a.largest_hole() == 30);
      CHECK(a.check_invariants());
    }
  }
}

alloc_t make_layout(fit_policy p) {
  // Blocks A..G of sizes 10,20,10,30,10,15,5 at 0,10,30,40,70,80,95; then
  // free B, D, F -> holes 20@10, 30@40, 15@80 (65 free, none adjacent).
  alloc_t a(100, p);
  const std::size_t sizes[] = {10, 20, 10, 30, 10, 15, 5};
  const std::size_t expected[] = {0, 10, 30, 40, 70, 80, 95};
  std::vector<std::size_t> addrs;
  for (const std::size_t s : sizes) {
    const auto r = a.allocate(s);
    CHECK(r.has_value());
    addrs.push_back(*r);
  }
  for (std::size_t i = 0; i < addrs.size(); ++i)
    CHECK(addrs[i] == expected[i]);
  a.deallocate(addrs[1]);
  a.deallocate(addrs[3]);
  a.deallocate(addrs[5]);
  CHECK(a.hole_count() == 3 && a.check_invariants());
  return a;
}

void test_policy_placement() {
  // Single request of 12 from holes {20@10, 30@40, 15@80}.
  const std::pair<fit_policy, std::size_t> single[] = {
      {fit_policy::first_fit, 10},
      {fit_policy::next_fit, 10}, // cursor at 100 wraps to the first hole
      {fit_policy::best_fit, 80},
      {fit_policy::worst_fit, 40}};
  for (const auto &[p, want] : single) {
    auto a = make_layout(p);
    const auto r = a.allocate(12);
    CHECK(r && *r == want);
    CHECK(a.check_invariants());
  }
  // Two requests (25 then 12): next-fit resumes after the previous block.
  const std::pair<fit_policy, std::pair<std::size_t, std::size_t>> two[] = {
      {fit_policy::first_fit, {40, 10}},
      {fit_policy::next_fit, {40, 80}},
      {fit_policy::best_fit, {40, 80}},
      {fit_policy::worst_fit, {40, 10}}};
  for (const auto &[p, want] : two) {
    auto a = make_layout(p);
    const auto r1 = a.allocate(25);
    const auto r2 = a.allocate(12);
    CHECK(r1 && *r1 == want.first);
    CHECK(r2 && *r2 == want.second);
    CHECK(a.check_invariants());
  }
}

void test_tie_breaking() {
  for (const auto p : {fit_policy::best_fit, fit_policy::worst_fit}) {
    alloc_t a(30, p);
    const auto b0 = *a.allocate(10), b1 = *a.allocate(5);
    const auto b2 = *a.allocate(10), b3 = *a.allocate(5);
    (void)b1;
    (void)b3;
    a.deallocate(b0); // hole 10@0
    a.deallocate(b2); // hole 10@15, equal length
    const auto r = a.allocate(10);
    CHECK(r && *r == 0); // lowest address wins ties
    CHECK(a.check_invariants());
  }
}

void test_stats() {
  const auto a = make_layout(fit_policy::best_fit);
  const auto s = a.stats();
  CHECK(s.capacity == 100 && s.used_bytes == 35 && s.free_bytes == 65);
  CHECK(s.largest_hole == 30 && s.hole_count == 3 && s.live_blocks == 4);
  CHECK(std::fabs(s.external_fragmentation - (1.0 - 30.0 / 65.0)) < 1e-12);
  alloc_t full(8, fit_policy::first_fit);
  CHECK(full.allocate(8).has_value());
  CHECK(full.stats().external_fragmentation == 0.0); // nothing free
}

void test_exhaust_with_unit_blocks() {
  for (const auto p : memsim::all_fit_policies) {
    constexpr std::size_t cap = 257;
    alloc_t a(cap, p);
    std::vector<std::size_t> addrs;
    while (const auto r = a.allocate(1))
      addrs.push_back(*r);
    CHECK(addrs.size() == cap && a.free_bytes() == 0);
    CHECK(a.check_invariants());
    std::mt19937 rng(7);
    std::shuffle(addrs.begin(), addrs.end(), rng);
    for (const auto addr : addrs)
      a.deallocate(addr);
    CHECK(a.hole_count() == 1 && a.largest_hole() == cap);
    CHECK(a.check_invariants());
  }
}

// ---------------------------------------------------------------------------
// Randomized stress against the reference model
// ---------------------------------------------------------------------------
template <typename SizeType>
void stress_against_reference(fit_policy policy, std::size_t capacity,
                              std::uint32_t seed, int ops,
                              std::size_t max_request) {
  std::mt19937 rng(seed);
  memsim::memory_allocator<SizeType> a(static_cast<SizeType>(capacity), policy);
  reference_model ref(capacity, policy);
  std::vector<std::pair<std::size_t, std::size_t>> live; // (addr, size)

  for (int i = 0; i < ops; ++i) {
    const bool do_alloc = live.empty() || (rng() % 100 < 55);
    if (do_alloc) {
      const std::size_t size = 1 + rng() % max_request;
      const auto got = a.allocate(static_cast<SizeType>(size));
      const auto want = ref.allocate(size);
      CHECK(got.has_value() == want.has_value());
      if (got) {
        CHECK(static_cast<std::size_t>(*got) == *want);
        live.emplace_back(*want, size);
      }
    } else {
      std::swap(live[rng() % live.size()], live.back());
      const auto [addr, size] = live.back();
      live.pop_back();
      a.deallocate(static_cast<SizeType>(addr));
      ref.deallocate(addr, size);
    }
    CHECK(static_cast<std::size_t>(a.used_bytes()) == ref.used_bytes());
    if (i % 53 == 0)
      CHECK(a.hole_count() == ref.free_runs().size());
    if (i % 97 == 0)
      CHECK(a.check_invariants());
  }
  CHECK(a.check_invariants());
  CHECK(a.live_blocks() == live.size());
}

void test_randomized_stress() {
  for (const auto p : memsim::all_fit_policies) {
    const auto seed = 12345u + static_cast<std::uint32_t>(p);
    stress_against_reference<std::size_t>(p, 2000, seed, 20000, 150);
    stress_against_reference<std::uint16_t>(p, 2000, seed + 1, 20000, 150);
    stress_against_reference<std::size_t>(p, 8, seed + 2, 5000, 8);
    stress_against_reference<std::uint32_t>(p, 1, seed + 3, 200, 1);
  }
}

// ---------------------------------------------------------------------------
// Experiment helpers
// ---------------------------------------------------------------------------
bool same_trace(const std::vector<memsim::trace_event> &a,
                const std::vector<memsim::trace_event> &b) {
  return a.size() == b.size() &&
         std::equal(a.begin(), a.end(), b.begin(), [](auto &x, auto &y) {
           return x.is_alloc == y.is_alloc && x.id == y.id && x.size == y.size;
         });
}

void test_trace_generation_and_replay() {
  using namespace memsim;
  constexpr std::size_t cap = 1 << 16; // multiple of 256
  const workload_spec specs[] = {
      {"u", size_distribution::uniform_small, free_order::random},
      {"e", size_distribution::exponential, free_order::fifo},
      {"b", size_distribution::bimodal, free_order::lifo},
      {"l", size_distribution::large_heavy, free_order::random},
      {"g", size_distribution::growing, free_order::random}};

  for (const auto &spec : specs) {
    const auto t1 = generate_trace(spec, cap, 5000, 0.8, 99);
    const auto t2 = generate_trace(spec, cap, 5000, 0.8, 99);
    CHECK(t1.size() == 5000 && same_trace(t1, t2)); // deterministic
    CHECK(!same_trace(t1, generate_trace(spec, cap, 5000, 0.8, 100)));

    std::vector<char> state(t1.size() + 1, 0); // 0 unseen, 1 live, 2 freed
    for (const auto &e : t1) {
      CHECK(e.size >= 1 && e.size <= cap);
      if (e.is_alloc) {
        CHECK(state[e.id] == 0);
        state[e.id] = 1;
      } else {
        CHECK(state[e.id] == 1); // no double free, no free-before-alloc
        state[e.id] = 2;
      }
    }
    const auto r1 = replay_trace(t1, cap, fit_policy::best_fit);
    const auto r2 = replay_trace(t1, cap, fit_policy::best_fit);
    CHECK(r1.alloc_failures == r2.alloc_failures &&
          r1.alloc_requests == r2.alloc_requests);
    CHECK(r1.fragmentation_failures <= r1.alloc_failures);
  }

  // Fixed-size blocks never fragment: every policy must fail identically.
  const workload_spec fixed{"f", size_distribution::fixed_size,
                            free_order::random};
  const auto trace = generate_trace(fixed, cap, 20000, 1.1, 5);
  const auto base = replay_trace(trace, cap, fit_policy::first_fit);
  CHECK(base.alloc_failures > 0);
  for (const auto p : all_fit_policies) {
    const auto r = replay_trace(trace, cap, p);
    CHECK(r.alloc_failures == base.alloc_failures);
    CHECK(r.fragmentation_failures == 0);
  }

  CHECK_THROWS(generate_trace(fixed, 0, 10, 0.5, 1), std::invalid_argument);
  CHECK_THROWS(generate_trace(fixed, cap, 10, 0.0, 1), std::invalid_argument);
  CHECK_THROWS(replay_trace(trace, cap, fit_policy::best_fit, 0),
               std::invalid_argument);
}

// ---------------------------------------------------------------------------
// Performance sanity: n log n scaling, not n^2
// ---------------------------------------------------------------------------
double time_cycle(fit_policy p, std::size_t n) {
  const auto t0 = std::chrono::steady_clock::now();
  alloc_t a(2 * n, p);
  std::vector<std::size_t> addrs;
  addrs.reserve(2 * n);
  for (std::size_t i = 0; i < 2 * n; ++i)
    addrs.push_back(*a.allocate(1));
  for (std::size_t i = 0; i < 2 * n; i += 2)
    a.deallocate(addrs[i]); // n isolated one-unit holes
  CHECK(a.hole_count() == n);
  for (std::size_t i = 0; i < n; ++i)
    CHECK(!a.allocate(2).has_value()); // O(1) rejection: no hole of size 2
  for (std::size_t i = 0; i < n; ++i)
    CHECK(a.allocate(1).has_value());
  CHECK(a.hole_count() == 0);
  return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0)
      .count();
}

double best_of_three(fit_policy p, std::size_t n) {
  double best = time_cycle(p, n);
  for (int i = 0; i < 2; ++i)
    best = std::min(best, time_cycle(p, n));
  return best;
}

void test_performance_scaling() {
  constexpr std::size_t small_n = 12500, large_n = 100000; // 8x apart
  for (const auto p : memsim::all_fit_policies) {
    const double t_small = best_of_three(p, small_n);
    const double t_large = best_of_three(p, large_n);
    const double ratio = t_large / std::max(t_small, 1e-9);
    std::printf("  %-10s n=%zu: %.4fs  n=%zu: %.4fs  ratio=%.1f\n",
                memsim::to_string(p), small_n, t_small, large_n, t_large,
                ratio);
    // 8x the work: ~8-12 for n log n (cache effects included), 64 for n^2.
    CHECK(ratio < 32.0);
  }
}

} // namespace

int main() {
  test_construction_and_invalid_input();
  test_single_block_and_double_free();
  test_exact_fit_removes_hole();
  test_coalescing_all_cases();
  test_policy_placement();
  test_tie_breaking();
  test_stats();
  test_exhaust_with_unit_blocks();
  test_randomized_stress();
  test_trace_generation_and_replay();
  test_performance_scaling();
  std::puts("All tests passed.");
  return 0;
}
