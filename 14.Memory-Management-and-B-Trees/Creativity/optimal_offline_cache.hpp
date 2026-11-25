/**
 * @file optimal_offline_cache.hpp
 * @brief Optimal offline page replacement (Belady's Farthest-in-Future)
 *        and verification of the m + n/m miss bound for a pool of m + 1 pages.
 *
 * ### Optimal Strategy (Belady's Algorithm)
 * On a page miss when the cache of capacity @p m is full, evict the cached
 * page whose next request is farthest in the future (or never requested again).
 *
 * ### Proof Sketch of the `m + floor(n / m)` Bound (Pool Size = m + 1)
 * 1. Starting from an empty cache, at most @p m cold-start misses occur before
 *    the cache holds @p m distinct pages.
 * 2. Once full, since the pool has only @p m + 1 pages, there is always
 *    exactly 1 page missing from the cache. When a miss evicts page `q` (whose
 *    next request is farthest in the future), all other `m - 1` cached pages
 *    must be requested before `q` is requested again (if ever). Thus each
 *    post-warmup miss is separated by at least `m` requests (1 miss + m - 1
 *    hits), yielding at most `floor(n / m)` misses after warmup, and at most
 *    `m + floor(n / m)` total misses.
 *
 */

#ifndef DS_OPTIMAL_OFFLINE_CACHE_HPP
#define DS_OPTIMAL_OFFLINE_CACHE_HPP

#include <cstddef>
#include <limits>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ds {

/**
 * @struct CacheSimulationResult
 * @brief Summary of an offline page caching simulation.
 */
struct CacheSimulationResult {
  std::size_t misses = 0;         ///< Total page faults (misses)
  std::size_t hits = 0;           ///< Total cache hits
  std::vector<int> evicted_pages; ///< Page evicted on each full-cache miss
};

/**
 * @brief Simulates Belady's optimal offline page replacement algorithm.
 *
 * @param requests        Sequence P of n page requests.
 * @param cache_capacity  Number of pages m the cache can hold (m >= 1).
 * @return                CacheSimulationResult with miss/hit counts and
 *                        eviction trace.
 * @throws std::invalid_argument if cache_capacity == 0.
 *
 * @section complexity
 *   - **Time Complexity:** $O(n \log m)$, where $n = \text{requests.size()}$
 *     and $m = \text{cache\_capacity}$.
 *   - **Auxiliary Space:** $O(n + m)$ for next-use precomputation and cache
 *     state.
 */
[[nodiscard]] inline CacheSimulationResult
simulate_belady_optimal(const std::vector<int> &requests,
                        std::size_t cache_capacity) {
  if (cache_capacity == 0) {
    throw std::invalid_argument(
        "simulate_belady_optimal: cache_capacity must be >= 1");
  }

  CacheSimulationResult res;
  const std::size_t n = requests.size();
  if (n == 0)
    return res;

  constexpr std::size_t kInf = std::numeric_limits<std::size_t>::max();

  // Step 1: Precompute next_use[i] = next index > i where requests[i] appears
  // (or kInf if it does not appear again). O(n) expected time.
  std::vector<std::size_t> next_use(n, kInf);
  std::unordered_map<int, std::size_t> last_seen;
  last_seen.reserve(cache_capacity + 2);

  for (std::size_t i = n; i > 0; --i) {
    const std::size_t idx = i - 1;
    const int page = requests[idx];
    auto it = last_seen.find(page);
    if (it != last_seen.end())
      next_use[idx] = it->second;
    last_seen[page] = idx;
  }

  // Step 2: Simulate cache using a balanced BST keyed by (next_use, page_id)
  // so the farthest-in-future page is always *cache_order.rbegin().
  std::set<std::pair<std::size_t, int>> cache_order;
  std::unordered_map<int, std::size_t> in_cache_next;
  in_cache_next.reserve(cache_capacity);

  for (std::size_t i = 0; i < n; ++i) {
    const int page = requests[i];
    const std::size_t nxt = next_use[i];

    auto it = in_cache_next.find(page);
    if (it != in_cache_next.end()) {
      // Cache hit: update page's next-use timestamp in O(log m).
      ++res.hits;
      cache_order.erase({it->second, page});
      it->second = nxt;
      cache_order.insert({nxt, page});
    } else {
      // Cache miss: evict farthest-in-future if cache is full.
      ++res.misses;
      if (in_cache_next.size() == cache_capacity) {
        auto victim = *cache_order.rbegin();
        cache_order.erase(std::prev(cache_order.end()));
        in_cache_next.erase(victim.second);
        res.evicted_pages.push_back(victim.second);
      }
      in_cache_next[page] = nxt;
      cache_order.insert({nxt, page});
    }
  }

  return res;
}

} // namespace ds

#endif // DS_OPTIMAL_OFFLINE_CACHE_HPP
