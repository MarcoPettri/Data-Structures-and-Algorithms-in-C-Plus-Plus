/**
 * @file extended_set.hpp
 * @brief Extended Set ADT (union / intersect / subtract / equals / ...) backed by a sorted vector.
 *
 * @details
 * Public interface of algo::ExtendedSet. The implementation lives in
 *
 * **Representation.** Elements are kept in a contiguous `std::vector`, sorted
 * strictly increasing under `Compare` (no duplicates). This gives:
 *
 * | Operation                    | Time                         |
 * |------------------------------|------------------------------|
 * | `size()`, `empty()`          | O(1)                         |
 * | `contains(e)`                | O(log n)                     |
 * | `insert(e)` / `remove(e)`    | O(log n) search + O(n) shift (O(1) amortised when inserting in increasing order) |
 * | `unite(B)`                   | O(n + m)                     |
 * | `intersect(B)`               | O(n + m), in place, no allocation |
 * | `subtract(B)`                | O(n + m), in place, no allocation |
 * | `equals(B)`                  | O(n)                         |
 *
 * where n = size of this set and m = size of B. Contiguous storage makes the
 * linear-time set operations (single merge-style pass) and iteration
 * extremely cache-friendly.
 *
 * @note `union` is a reserved word in C++, so that operation is named
 *       `unite`.
 *
 * @par Example
 * @code
 * algo::ExtendedSet<int> a{1, 3, 5, 7};
 * algo::ExtendedSet<int> b{3, 4, 5};
 *
 * auto u = a | b;          // {1, 3, 4, 5, 7}
 * auto i = a & b;          // {3, 5}
 * auto d = a - b;          // {1, 7}
 *
 * a.unite(b);              // in place: a == {1, 3, 4, 5, 7}
 * bool has = a.contains(4);
 * @endcode
 */

#ifndef ALGO_EXTENDED_SET_HPP
#define ALGO_EXTENDED_SET_HPP

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

namespace algo {

/**
 * @brief Set of unique elements with bulk set operations.
 *
 * @tparam T        Element type. Must be copy-constructible and
 *                  move-assignable (needed by the in-place operations).
 * @tparam Compare  Strict weak ordering on `T` (default `std::less<T>`).
 *                  Two elements are considered equal when neither compares
 *                  less than the other.
 *
 * @par Exception safety
 * All operations offer at least the basic guarantee: the set always remains
 * valid (sorted, duplicate-free), but if `Compare`, a copy or a move throws
 * during a bulk operation its contents are unspecified.
 *
 * @par Iterator invalidation
 * Any mutating operation may invalidate iterators and references.
 */
template <typename T, typename Compare = std::less<T>>
class ExtendedSet {
public:
    /// @name Member types
    /// @{
    using value_type      = T;
    using size_type       = std::size_t;
    using key_compare     = Compare;
    using const_iterator  = typename std::vector<T>::const_iterator;
    using iterator        = const_iterator;  ///< Elements are immutable (would break ordering).
    /// @}

    /// @name Construction
    /// @{

    /// @brief Creates an empty set. O(1).
    ExtendedSet() = default;

    /// @brief Creates an empty set using the given comparator. O(1).
    explicit ExtendedSet(Compare comp) : comp_(std::move(comp)) {}

    /**
     * @brief Creates a set from an initializer list; duplicates are discarded.
     * @param init  Elements (any order, duplicates allowed).
     * @param comp  Comparator.
     * @par Complexity O(k log k) for k = `init.size()`.
     */
    ExtendedSet(std::initializer_list<T> init, Compare comp = Compare());

    /**
     * @brief Creates a set from an iterator range; duplicates are discarded.
     * @tparam InputIt Input iterator whose `value_type` converts to `T`.
     * @param first,last  Source range `[first, last)`.
     * @param comp        Comparator.
     * @par Complexity O(k log k) for k = `distance(first, last)`.
     */
    template <typename InputIt,
              typename = typename std::iterator_traits<InputIt>::iterator_category>
    ExtendedSet(InputIt first, InputIt last, Compare comp = Compare());

    /// @}

    /// @name Capacity
    /// @{

    /// @brief Number of elements. O(1).
    [[nodiscard]] size_type size() const noexcept { return data_.size(); }

    /// @brief True when the set has no elements. O(1).
    [[nodiscard]] bool empty() const noexcept { return data_.empty(); }

    /// @brief Reserves storage for at least @p n elements.
    void reserve(size_type n) { data_.reserve(n); }

    /// @}

    /// @name Element access / modification
    /// @{

    /**
     * @brief Tests membership.
     * @param value  Element to look for.
     * @return `true` if an element equivalent to @p value is in the set.
     * @par Complexity O(log n).
     */
    [[nodiscard]] bool contains(const T& value) const;

    /**
     * @brief Inserts @p value if no equivalent element is present.
     * @return `true` if the element was inserted, `false` if already present.
     * @par Complexity O(log n) search + O(n) shift; O(1) amortised when
     *      @p value is greater than every element.
     */
    bool insert(const T& value);

    /// @copydoc insert(const T&)
    bool insert(T&& value);

    /**
     * @brief Removes the element equivalent to @p value, if any.
     * @return `true` if an element was removed, `false` if it was absent.
     * @par Complexity O(log n) search + O(n) shift.
     */
    bool remove(const T& value);

    /// @brief Removes all elements. O(n).
    void clear() noexcept { data_.clear(); }

    /// @brief Swaps contents with @p other. O(1).
    void swap(ExtendedSet& other) noexcept(std::is_nothrow_swappable_v<Compare>) {
        using std::swap;
        data_.swap(other.data_);
        swap(comp_, other.comp_);
    }

    /// @}

    /// @name Set operations (in place, as in the classical ADT)
    /// @{

    /**
     * @brief `*this = *this ∪ other`.
     * @par Complexity O(n + m) time, O(n + m) temporary space (O(1) extra when
     *      all of @p other is greater than all of `*this`, or `*this` is empty).
     * @note Safe when `&other == this` (no-op).
     */
    void unite(const ExtendedSet& other);

    /**
     * @brief `*this = *this ∩ other`.
     * @par Complexity O(n + m) time, O(1) extra space (in-place compaction).
     * @note Safe when `&other == this` (no-op).
     */
    void intersect(const ExtendedSet& other);

    /**
     * @brief `*this = *this \ other` (elements of `*this` not in @p other).
     * @par Complexity O(n + m) time, O(1) extra space (in-place compaction).
     * @note Safe when `&other == this` (result is empty).
     */
    void subtract(const ExtendedSet& other);

    /// @}

    /// @name Comparison
    /// @{

    /**
     * @brief Set equality: both sets contain exactly the same elements.
     * @par Complexity O(n) (O(1) when sizes differ).
     */
    [[nodiscard]] bool equals(const ExtendedSet& other) const;

    /// @}

    /// @name Operator sugar
    /// @{
    ExtendedSet& operator|=(const ExtendedSet& other) { unite(other);     return *this; }
    ExtendedSet& operator&=(const ExtendedSet& other) { intersect(other); return *this; }
    ExtendedSet& operator-=(const ExtendedSet& other) { subtract(other);  return *this; }
    /// @}

    /// @name Iteration (ascending order under Compare)
    /// @{
    [[nodiscard]] const_iterator begin()  const noexcept { return data_.begin();  }
    [[nodiscard]] const_iterator end()    const noexcept { return data_.end();    }
    [[nodiscard]] const_iterator cbegin() const noexcept { return data_.cbegin(); }
    [[nodiscard]] const_iterator cend()   const noexcept { return data_.cend();   }
    /// @}

    /// @brief Returns a copy of the comparator.
    [[nodiscard]] key_compare key_comp() const { return comp_; }

private:
    /// Sorts and removes duplicates from `data_` (used by range constructors).
    void normalize();

    /// Shared implementation of the two insert overloads.
    template <typename U>
    bool insertImpl(U&& value);

    std::vector<T> data_;  ///< Strictly increasing under `comp_`.
    Compare        comp_{};
};

/// @name Non-member operators
/// @{

/// @brief Set equality. @see ExtendedSet::equals
template <typename T, typename Compare>
[[nodiscard]] bool operator==(const ExtendedSet<T, Compare>& a, const ExtendedSet<T, Compare>& b);

/// @brief Set inequality.
template <typename T, typename Compare>
[[nodiscard]] bool operator!=(const ExtendedSet<T, Compare>& a, const ExtendedSet<T, Compare>& b);

/// @brief Union as a new set (`a \cup b`). O(n + m).
template <typename T, typename Compare>
[[nodiscard]] ExtendedSet<T, Compare> operator|(ExtendedSet<T, Compare> a, const ExtendedSet<T, Compare>& b);

/// @brief Intersection as a new set (`a \cap b`). O(n + m).
template <typename T, typename Compare>
[[nodiscard]] ExtendedSet<T, Compare> operator&(ExtendedSet<T, Compare> a, const ExtendedSet<T, Compare>& b);

/// @brief Difference as a new set (`a \ b`). O(n + m).
template <typename T, typename Compare>
[[nodiscard]] ExtendedSet<T, Compare> operator-(ExtendedSet<T, Compare> a, const ExtendedSet<T, Compare>& b);

/// @brief Free-function swap. O(1).
template <typename T, typename Compare>
void swap(ExtendedSet<T, Compare>& a, ExtendedSet<T, Compare>& b) noexcept(noexcept(a.swap(b)));

/// @}

}  // namespace algo

#include "extended_set.tpp"

#endif  // ALGO_EXTENDED_SET_HPP