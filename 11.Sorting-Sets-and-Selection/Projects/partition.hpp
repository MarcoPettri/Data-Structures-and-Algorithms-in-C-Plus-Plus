/**
 * @file partition.hpp
 * @brief Partition ADT (makeSet / find / union) over arbitrary hashable elements.
 *
 * @details
 * Thin generic layer on top of algo::UnionFind: a hash map translates each
 * element to a dense id, and the id-based forest (union by size + path
 * compression) does the real work. The implementation lives in
 * @ref partition.tpp (included at the bottom of this header).
 *
 * Costs: `find`, `unite`, `connected`, `setSize` are O(α(n)) amortised plus
 * one hash lookup per element argument (O(1) expected).
 *
 * @par Example
 * @code
 * algo::Partition<std::string> p;
 * p.makeSet("a"); p.makeSet("b"); p.makeSet("c");
 * p.unite("a", "b");
 * p.connected("a", "b");   // true
 * p.find("b");             // representative: "a" or "b"
 * p.numSets();             // 2
 * @endcode
 */

#ifndef ALGO_PARTITION_HPP
#define ALGO_PARTITION_HPP

#include "union_find.hpp"

#include <cstddef>
#include <functional>
#include <unordered_map>
#include <vector>

namespace algo {

/**
 * @brief Collection of disjoint sets of elements of type `T`.
 *
 * @tparam T         Element type; copy-constructible.
 * @tparam Hash      Hash functor for `T` (default `std::hash<T>`).
 * @tparam KeyEqual  Equality predicate for `T` (default `std::equal_to<T>`).
 *
 * @par Exception safety
 * Strong guarantee for every operation. Lookups of unknown elements throw
 * `std::out_of_range`.
 *
 * @note Elements are stored twice (in the hash map and in a vector indexed by
 *       id) so that representatives can be returned by reference.
 * @note Not thread-safe: `find` mutates the forest.
 */
template <typename T, typename Hash = std::hash<T>,
          typename KeyEqual = std::equal_to<T>>
class Partition {
public:
  using value_type = T;
  using size_type = std::size_t;

  /// @brief Creates an empty partition. O(1).
  Partition() = default;

  /**
     * @brief Creates the singleton set `{x}` (the ADT's `makeSet`).
     * @return `true` if @p x was new, `false` if it already belongs to a set
     *         (nothing changes in that case).
     * @par Complexity O(1) expected.
     */
  bool makeSet(const T &x);

  /// @brief True if @p x has been added with makeSet(). O(1) expected.
  [[nodiscard]] bool contains(const T &x) const;

  /**
     * @brief Returns the representative element of the set containing @p x.
     *
     * The representative of a set can change when the set is merged. The
     * returned reference stays valid until the next call to makeSet().
     *
     * @throws std::out_of_range if @p x is unknown.
     * @par Complexity O(α(n)) amortised.
     */
  [[nodiscard]] const T &find(const T &x);

  /**
     * @brief Merges the sets containing @p a and @p b.
     * @return `true` if two distinct sets were merged, `false` if already together.
     * @throws std::out_of_range if either element is unknown.
     * @par Complexity O(α(n)) amortised.
     */
  bool unite(const T &a, const T &b);

  /// @brief True if @p a and @p b are in the same set. @throws std::out_of_range if unknown.
  [[nodiscard]] bool connected(const T &a, const T &b);

  /// @brief Size of the set containing @p x. @throws std::out_of_range if unknown.
  [[nodiscard]] size_type setSize(const T &x);

  /// @brief Total number of elements. O(1).
  [[nodiscard]] size_type size() const noexcept { return elems_.size(); }

  /// @brief Number of disjoint sets. O(1).
  [[nodiscard]] size_type numSets() const noexcept { return sets_.numSets(); }

  /// @brief True if there are no elements. O(1).
  [[nodiscard]] bool empty() const noexcept { return elems_.empty(); }

private:
  /// Id of @p x; throws std::out_of_range when unknown.
  size_type idOf(const T &x) const;

  UnionFind sets_; ///< Forest over ids.
  std::vector<T> elems_; ///< id -> element.
  std::unordered_map<T, size_type, Hash, KeyEqual> ids_; ///< element -> id.
};

} // namespace algo

#include "partition.tpp"

#endif // ALGO_PARTITION_HPP