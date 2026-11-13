/**
 * @file bucket_sort.tpp
 * @brief Template implementation of algo::bucket_sort (included by bucket_sort.hpp).
 *
 * Do not include this file directly; include @ref bucket_sort.hpp.
 */

#ifndef ALGO_BUCKET_SORT_TPP
#define ALGO_BUCKET_SORT_TPP

#include <cstdint>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace algo {
namespace detail {

/**
 * @brief Validates a key and converts it to a bucket index.
 *
 * @tparam K Integral key type (not bool).
 * @param k  Key to validate.
 * @param N  Key universe size.
 * @return   `k` as a `std::size_t`, guaranteed to be in [0, N-1].
 * @throws std::out_of_range if `k < 0` or `k >= N`.
 */
template <typename K>
[[nodiscard]] inline std::size_t checked_bucket_index(K k, std::size_t N) {
  static_assert(std::is_integral_v<K> && !std::is_same_v<K, bool>,
                "bucket_sort: the key extractor must return an integral type");

  if constexpr (std::is_signed_v<K>) {
    if (k < 0) {
      throw std::out_of_range("bucket_sort: key is negative");
    }
  }
  const auto u = static_cast<std::uintmax_t>(k);
  if (u >= static_cast<std::uintmax_t>(N)) {
    throw std::out_of_range("bucket_sort: key is outside [0, N-1]");
  }
  return static_cast<std::size_t>(u);
}

}  // namespace detail

template <typename T, typename Alloc, typename KeyFn>
void bucket_sort(std::list<T, Alloc> &list, std::size_t N, KeyFn key) {
  using List = std::list<T, Alloc>;

  if (N < 2) {
    throw std::invalid_argument("bucket_sort: N must be >= 2");
  }
  if (list.size() < 2) {  // std::list::size() is O(1) since C++11
    return;
  }

  // N buckets sharing the list's allocator (required for cross-list splice).
  // reserve + emplace_back avoids N copy-constructions and extra reallocation.
  std::vector<List> buckets;
  buckets.reserve(N);
  for (std::size_t i = 0; i < N; ++i) {
    buckets.emplace_back(list.get_allocator());
  }

  // Moves every bucket's nodes back to the end of `list`, in bucket order.
  const auto gather = [&]() noexcept {
    for (List &b : buckets) {
      list.splice(list.end(), b);  // O(1) each
    }
  };

  try {
    // Distribution: walk the list once, relinking each node (O(1)) at the
    // back of its bucket. Appending at the back keeps the sort stable.
    for (auto it = list.begin(); it != list.end();) {
      const auto next = std::next(it);
      const T &value = *it;
      List &bucket = buckets[detail::checked_bucket_index(key(value), N)];
      bucket.splice(bucket.end(), list, it);
      it = next;
    }
  } catch (...) {
    gather();  // nothing lost; order unspecified (basic guarantee)
    throw;
  }

  // Concatenation: O(N) splices of whole buckets.
  gather();
}

void bucket_sort(std::list<int> &list, std::size_t N) {
  // The value itself is the key.
  bucket_sort(list, N, [](const int &v) noexcept { return v; });
}

}  // namespace algo

#endif  // ALGO_BUCKET_SORT_TPP