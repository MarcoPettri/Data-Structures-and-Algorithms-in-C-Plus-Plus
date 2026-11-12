/**
 * @file union_find.hpp
 * @brief Tree-based union/find (disjoint-set forest) with union-by-size and path compression.
 *
 * @details
 * Elements are the dense integer ids `0 .. size()-1`. Each set is a tree whose
 * root is the set's representative.
 *
 *  - **Union by size**: `unite` always hangs the smaller tree under the root of
 *    the larger one, so tree height is at most `floor(log2 n)`.
 *  - **Path compression**: `find` makes every node on the search path point
 *    directly at the root, flattening the tree for later queries.
 *
 * Together they give an amortised cost of O(α(n)) per operation (α is the
 * inverse Ackermann function, ≤ 4 for any practical n); a sequence of m
 * operations on n elements costs O(m · α(n)).
 *
 * **Memory layout.** A single `std::ptrdiff_t` array stores both pieces of
 * information per element: a non-negative entry is the parent's id; a
 * negative entry marks a root and holds minus the size of its tree. That is
 * 8 bytes per element on 64-bit platforms, with one memory access per hop.
 *
 * For arbitrary element types (strings, structs, ...) see
 * @ref partition.hpp, which wraps this class.
 *
 * @par Example
 * @code
 * algo::UnionFind uf(6);          // {0} {1} {2} {3} {4} {5}
 * uf.unite(0, 1);
 * uf.unite(2, 3);
 * uf.unite(1, 3);                 // {0 1 2 3} {4} {5}
 * bool same = uf.connected(0, 2); // true
 * auto n    = uf.numSets();       // 3
 * @endcode
 */

#ifndef ALGO_UNION_FIND_HPP
#define ALGO_UNION_FIND_HPP

#include <cstddef>
#include <vector>

namespace algo {

/**
 * @brief Disjoint-set forest over the ids `0 .. size()-1`.
 *
 * @par Exception safety
 * Operations that throw (`std::out_of_range` for bad ids, `std::bad_alloc`
 * from growth) leave the structure unchanged (strong guarantee).
 *
 * @note Not thread-safe, not even for concurrent `find` calls: `find`
 *       mutates the forest (path compression).
 */
class UnionFind {
public:
    using size_type = std::size_t;

    /// @brief Creates an empty structure (no elements). O(1).
    UnionFind() = default;

    /**
     * @brief Creates @p n singleton sets `{0}, {1}, ..., {n-1}`.
     * @param n  Number of elements.
     * @par Complexity O(n).
     */
    explicit UnionFind(size_type n);

    /**
     * @brief Adds a new element as a singleton set (the ADT's `makeSet`).
     * @return Id of the new element (equal to the previous `size()`).
     * @par Complexity O(1) amortised.
     */
    size_type makeSet();

    /// @brief Reserves storage for @p n elements.
    void reserve(size_type n) { link_.reserve(n); }

    /// @brief Removes all elements. O(1)–O(n) depending on the allocator.
    void clear() noexcept {
        link_.clear();
        numSets_ = 0;
    }

    /**
     * @brief Returns the representative (root) of the set containing @p x.
     *
     * Applies full path compression: every node on the path from @p x to the
     * root is re-parented directly to the root.
     *
     * @param x  Element id.
     * @throws std::out_of_range if `x >= size()`.
     * @par Complexity O(α(n)) amortised.
     */
    [[nodiscard]] size_type find(size_type x);

    /**
     * @brief Merges the sets containing @p a and @p b (union by size).
     *
     * The smaller tree is attached under the root of the larger one. On equal
     * sizes the root of @p a's set stays the root.
     *
     * @return `true` if two distinct sets were merged, `false` if @p a and
     *         @p b were already in the same set.
     * @throws std::out_of_range if either id is `>= size()`.
     * @par Complexity O(α(n)) amortised.
     */
    bool unite(size_type a, size_type b);

    /**
     * @brief Tests whether @p a and @p b belong to the same set.
     * @throws std::out_of_range if either id is `>= size()`.
     * @par Complexity O(α(n)) amortised.
     */
    [[nodiscard]] bool connected(size_type a, size_type b);

    /**
     * @brief Number of elements in the set containing @p x.
     * @throws std::out_of_range if `x >= size()`.
     * @par Complexity O(α(n)) amortised.
     */
    [[nodiscard]] size_type setSize(size_type x);

    /**
     * @brief Number of edges between @p x and its root, **without** compressing.
     *
     * Diagnostic helper (used by the tests to observe both heuristics).
     * Bounded by `floor(log2 n)` thanks to union by size.
     *
     * @throws std::out_of_range if `x >= size()`.
     * @par Complexity O(depth of x).
     */
    [[nodiscard]] size_type depth(size_type x) const;

    /// @brief Total number of elements. O(1).
    [[nodiscard]] size_type size() const noexcept { return link_.size(); }

    /// @brief Number of disjoint sets. O(1).
    [[nodiscard]] size_type numSets() const noexcept { return numSets_; }

    /// @brief True if there are no elements. O(1).
    [[nodiscard]] bool empty() const noexcept { return link_.empty(); }

private:
    using link_type = std::ptrdiff_t;

    /// Throws std::out_of_range if @p x is not a valid id.
    void check(size_type x) const;

    /// Root lookup with path compression; @p x must be valid.
    size_type findUnchecked(size_type x) noexcept;

    /// `link_[i] >= 0`: parent of i.  `link_[i] < 0`: i is a root of a tree of size `-link_[i]`.
    std::vector<link_type> link_;
    size_type numSets_ = 0;
};

}  // namespace algo

#endif  // ALGO_UNION_FIND_HPP