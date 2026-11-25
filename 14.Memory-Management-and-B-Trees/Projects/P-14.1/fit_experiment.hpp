/**
 * @file fit_experiment.hpp
 * @brief Workload generation and trace replay for comparing fit policies.
 *
 * @details
 * A trace is generated once from a seeded PRNG, independently of any
 * allocator, and then replayed against each policy so every policy sees the
 * identical request sequence. A free event whose allocation failed during
 * replay is skipped. Note: std::*_distribution outputs are not specified
 * bit-for-bit by the standard, so traces are reproducible per standard
 * library implementation, not across implementations.
 */
#pragma once

#include "memory_allocator.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace memsim {

/** @brief Distribution of requested block sizes. */
enum class size_distribution {
  uniform_small, ///< U[1, 1024]
  exponential,   ///< 1 + Exp(mean 200), capped at 8192
  bimodal,       ///< 90% U[8, 64], 10% U[2048, 8192]
  large_heavy,   ///< U[2048, 32768]
  growing,       ///< U[1, hi] where hi grows from 64 to ~4096 over the trace
  fixed_size     ///< always 256
};

/** @brief Which live block is freed next. */
enum class free_order { random, fifo, lifo };

/** @brief Named workload description. */
struct workload_spec {
  std::string name;
  size_distribution sizes;
  free_order order;
};

/** @brief One allocator-independent request. */
struct trace_event {
  bool is_alloc;
  std::size_t id;   ///< allocation id (sequential over alloc events)
  std::size_t size; ///< requested size (also stored on free events)
};

/** @brief Metrics from replaying one trace under one policy. */
struct replay_result {
  fit_policy policy = fit_policy::first_fit;
  std::size_t alloc_requests = 0;
  std::size_t alloc_failures = 0;
  /// Failures although total free space was >= the request (pure
  /// fragmentation failures).
  std::size_t fragmentation_failures = 0;
  double mean_utilization = 0.0;
  double mean_external_fragmentation = 0.0;
  double mean_hole_count = 0.0;
  double ns_per_operation = 0.0;
};

namespace detail {

inline std::size_t sample_size(size_distribution dist, std::mt19937_64 &rng,
                               std::size_t capacity, double progress) {
  auto uniform = [&rng](std::size_t lo, std::size_t hi) {
    return std::uniform_int_distribution<std::size_t>(lo, hi)(rng);
  };
  std::size_t s = 1;
  switch (dist) {
  case size_distribution::uniform_small:
    s = uniform(1, 1024);
    break;
  case size_distribution::exponential: {
    std::exponential_distribution<double> e(1.0 / 200.0);
    s = std::min<std::size_t>(1 + static_cast<std::size_t>(e(rng)), 8192);
    break;
  }
  case size_distribution::bimodal: {
    std::bernoulli_distribution large(0.1);
    s = large(rng) ? uniform(2048, 8192) : uniform(8, 64);
    break;
  }
  case size_distribution::large_heavy:
    s = uniform(2048, 32768);
    break;
  case size_distribution::growing:
    s = uniform(1, 64 + static_cast<std::size_t>(progress * 4032.0));
    break;
  case size_distribution::fixed_size:
    s = 256;
    break;
  }
  return std::max<std::size_t>(1, std::min(s, capacity));
}

} // namespace detail

/**
 * @brief Generates a trace that hovers around a target live-byte load.
 *
 * @param spec        Size distribution and free order.
 * @param capacity    Heap capacity the load is measured against.
 * @param operations  Number of events to generate.
 * @param target_load Desired live bytes / capacity (assuming every request
 *                    succeeds); may exceed 1 to force failures.
 * @param seed        PRNG seed.
 * @return The event sequence; every free refers to an earlier live alloc.
 * @throws std::invalid_argument if @p capacity is 0 or @p target_load <= 0.
 *
 * @section complexity Complexity
 * - Time: $O(m)$ expected; space: $O(m)$, where $m$ = @p operations.
 */
[[nodiscard]] inline std::vector<trace_event>
generate_trace(const workload_spec &spec, std::size_t capacity,
               std::size_t operations, double target_load, std::uint64_t seed) {
  if (capacity == 0 || target_load <= 0.0)
    throw std::invalid_argument("generate_trace: bad capacity or load");

  std::mt19937_64 rng(seed);
  std::vector<trace_event> trace;
  trace.reserve(operations);
  std::vector<std::size_t> sizes; // indexed by id
  std::deque<std::size_t> live;   // ids of live allocations
  std::size_t live_bytes = 0;
  const auto target_bytes =
      static_cast<std::size_t>(target_load * static_cast<double>(capacity));
  std::bernoulli_distribution below_target(0.7), above_target(0.3);

  while (trace.size() < operations) {
    const double progress =
        static_cast<double>(trace.size()) / static_cast<double>(operations);
    const bool do_alloc =
        live.empty() ||
        (live_bytes < target_bytes ? below_target(rng) : above_target(rng));
    if (do_alloc) {
      const std::size_t size =
          detail::sample_size(spec.sizes, rng, capacity, progress);
      const std::size_t id = sizes.size();
      sizes.push_back(size);
      live.push_back(id);
      live_bytes += size;
      trace.push_back({true, id, size});
    } else {
      std::size_t id = 0;
      switch (spec.order) {
      case free_order::random: {
        std::uniform_int_distribution<std::size_t> pick(0, live.size() - 1);
        std::swap(live[pick(rng)], live.back());
        id = live.back();
        live.pop_back();
        break;
      }
      case free_order::fifo:
        id = live.front();
        live.pop_front();
        break;
      case free_order::lifo:
        id = live.back();
        live.pop_back();
        break;
      }
      live_bytes -= sizes[id];
      trace.push_back({false, id, sizes[id]});
    }
  }
  return trace;
}

/**
 * @brief Replays @p trace against a fresh allocator and collects metrics.
 *
 * State is sampled every @p sample_interval events. Timing covers the whole
 * replay including sampling, which is $O(1)$ per sample.
 *
 * @throws std::bad_alloc, std::invalid_argument on invalid arguments.
 *
 * @section complexity Complexity
 * - Time: $\sum$ of per-operation costs of the policy (see
 *   memory_allocator); space: $O(m + h + a)$ with $m$ = trace length.
 */
[[nodiscard]] inline replay_result
replay_trace(const std::vector<trace_event> &trace, std::size_t capacity,
             fit_policy policy, std::size_t sample_interval = 64) {
  if (sample_interval == 0)
    throw std::invalid_argument("replay_trace: sample_interval must be > 0");

  memory_allocator<std::size_t> allocator(capacity, policy);
  std::vector<std::optional<std::size_t>> address_of(trace.size());
  replay_result r;
  r.policy = policy;

  double util_sum = 0.0, frag_sum = 0.0, holes_sum = 0.0;
  std::size_t samples = 0, op_index = 0;
  const auto start = std::chrono::steady_clock::now();

  for (const trace_event &e : trace) {
    if (e.is_alloc) {
      ++r.alloc_requests;
      if (const auto got = allocator.allocate(e.size)) {
        address_of[e.id] = *got;
      } else {
        ++r.alloc_failures;
        if (allocator.free_bytes() >= e.size)
          ++r.fragmentation_failures;
      }
    } else if (address_of[e.id]) {
      allocator.deallocate(*address_of[e.id]);
      address_of[e.id].reset();
    }
    if (++op_index % sample_interval == 0) {
      const auto s = allocator.stats();
      util_sum +=
          static_cast<double>(s.used_bytes) / static_cast<double>(capacity);
      frag_sum += s.external_fragmentation;
      holes_sum += static_cast<double>(s.hole_count);
      ++samples;
    }
  }

  const auto stop = std::chrono::steady_clock::now();
  if (!trace.empty())
    r.ns_per_operation =
        std::chrono::duration<double, std::nano>(stop - start).count() /
        static_cast<double>(trace.size());
  if (samples > 0) {
    const auto n = static_cast<double>(samples);
    r.mean_utilization = util_sum / n;
    r.mean_external_fragmentation = frag_sum / n;
    r.mean_hole_count = holes_sum / n;
  }
  return r;
}

} // namespace memsim
