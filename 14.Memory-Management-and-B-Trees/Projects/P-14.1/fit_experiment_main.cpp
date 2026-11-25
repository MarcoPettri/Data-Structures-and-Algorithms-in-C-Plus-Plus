/**
 * @file fit_experiment_main.cpp
 * @brief Runs every workload against all four fit policies and prints a
 *        comparison table plus a mean-rank summary.
 *
 * @details
 * Usage: fit_experiment [operations=100000] [trials=5]
 * Each (workload, load) scenario is averaged over @c trials seeds. Policies
 * are ranked per scenario by allocation-failure rate (ties share a rank).
 */
#include "fit_experiment.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

struct accumulator {
  double fail_rate = 0, frag_fail_rate = 0, utilization = 0, fragmentation = 0,
         holes = 0, ns_per_op = 0;
};

} // namespace

int main(int argc, char **argv) {
  using namespace memsim;

  const std::size_t operations =
      argc > 1 ? std::strtoull(argv[1], nullptr, 10) : 100000;
  const std::size_t trials = argc > 2 ? std::strtoull(argv[2], nullptr, 10) : 5;
  if (operations == 0 || trials == 0) {
    std::fprintf(stderr, "usage: %s [operations>0] [trials>0]\n", argv[0]);
    return 1;
  }

  constexpr std::size_t capacity = std::size_t{1} << 20;
  constexpr std::uint64_t base_seed = 20260101;
  const std::vector<double> loads{0.80, 0.90};
  const std::vector<workload_spec> workloads{
      {"uniform[1,1024], random free", size_distribution::uniform_small,
       free_order::random},
      {"exponential(200), random free", size_distribution::exponential,
       free_order::random},
      {"bimodal small/large, random free", size_distribution::bimodal,
       free_order::random},
      {"large-heavy[2K,32K], random free", size_distribution::large_heavy,
       free_order::random},
      {"uniform[1,1024], FIFO free", size_distribution::uniform_small,
       free_order::fifo},
      {"uniform[1,1024], LIFO free", size_distribution::uniform_small,
       free_order::lifo},
      {"growing sizes, random free", size_distribution::growing,
       free_order::random},
      {"fixed 256, random free", size_distribution::fixed_size,
       free_order::random}};

  std::array<double, 4> rank_sum{};
  std::size_t scenarios = 0;

  for (const double load : loads) {
    for (const workload_spec &spec : workloads) {
      std::array<accumulator, 4> acc{};
      for (std::size_t t = 0; t < trials; ++t) {
        const auto trace =
            generate_trace(spec, capacity, operations, load, base_seed + t);
        for (std::size_t p = 0; p < all_fit_policies.size(); ++p) {
          const replay_result r =
              replay_trace(trace, capacity, all_fit_policies[p]);
          const double req =
              static_cast<double>(std::max<std::size_t>(1, r.alloc_requests));
          acc[p].fail_rate += static_cast<double>(r.alloc_failures) / req;
          acc[p].frag_fail_rate +=
              static_cast<double>(r.fragmentation_failures) / req;
          acc[p].utilization += r.mean_utilization;
          acc[p].fragmentation += r.mean_external_fragmentation;
          acc[p].holes += r.mean_hole_count;
          acc[p].ns_per_op += r.ns_per_operation;
        }
      }
      const auto n = static_cast<double>(trials);
      for (auto &a : acc) {
        a.fail_rate /= n;
        a.frag_fail_rate /= n;
        a.utilization /= n;
        a.fragmentation /= n;
        a.holes /= n;
        a.ns_per_op /= n;
      }

      std::printf("\n== %s | target load %.0f%% | %zu ops x %zu trials ==\n",
                  spec.name.c_str(), load * 100.0, operations, trials);
      std::printf("%-10s %9s %10s %9s %9s %8s %9s\n", "policy", "fail%",
                  "frag-fail%", "ext.frag", "holes", "util", "ns/op");
      for (std::size_t p = 0; p < 4; ++p)
        std::printf("%-10s %8.3f%% %9.3f%% %9.4f %9.1f %8.4f %9.1f\n",
                    to_string(all_fit_policies[p]), acc[p].fail_rate * 100.0,
                    acc[p].frag_fail_rate * 100.0, acc[p].fragmentation,
                    acc[p].holes, acc[p].utilization, acc[p].ns_per_op);

      // Competition ranking by failure rate; equal rates share a rank.
      std::array<std::size_t, 4> order{0, 1, 2, 3};
      std::sort(order.begin(), order.end(), [&](std::size_t l, std::size_t r) {
        return acc[l].fail_rate < acc[r].fail_rate;
      });
      std::size_t rank = 1;
      for (std::size_t pos = 0; pos < 4; ++pos) {
        if (pos > 0 &&
            acc[order[pos]].fail_rate > acc[order[pos - 1]].fail_rate + 1e-12)
          rank = pos + 1;
        rank_sum[order[pos]] += static_cast<double>(rank);
      }
      ++scenarios;
    }
  }

  std::printf("\n== Mean rank over %zu scenarios (1 = fewest failed "
              "allocations; ties share a rank) ==\n",
              scenarios);
  for (std::size_t p = 0; p < 4; ++p)
    std::printf("%-10s %.3f\n", to_string(all_fit_policies[p]),
                rank_sum[p] / static_cast<double>(scenarios));
  return 0;
}
