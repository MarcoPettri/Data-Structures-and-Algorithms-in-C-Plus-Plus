/**
 * @file extended_set.tpp
 * @brief Template implementation of algo::ExtendedSet (included by extended_set.hpp).
 *
 * Do not include this file directly; include @ref extended_set.hpp.
 */

#ifndef ALGO_EXTENDED_SET_TPP
#define ALGO_EXTENDED_SET_TPP

#include <algorithm>
#include <iterator>

namespace algo {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

template <typename T, typename Compare>
ExtendedSet<T, Compare>::ExtendedSet(std::initializer_list<T> init, Compare comp)
    : data_(init), comp_(std::move(comp)) {
    normalize();
}

template <typename T, typename Compare>
template <typename InputIt, typename>
ExtendedSet<T, Compare>::ExtendedSet(InputIt first, InputIt last, Compare comp)
    : data_(first, last), comp_(std::move(comp)) {
    normalize();
}

template <typename T, typename Compare>
void ExtendedSet<T, Compare>::normalize() {
    std::sort(data_.begin(), data_.end(), comp_);
    // After sorting, a <= b always holds for adjacent elements, so
    // "!comp(a, b)" is exactly "a is equivalent to b".
    data_.erase(std::unique(data_.begin(), data_.end(),
                            [this](const T& a, const T& b) { return !comp_(a, b); }),
                data_.end());
}

// ---------------------------------------------------------------------------
// Element access / modification
// ---------------------------------------------------------------------------

template <typename T, typename Compare>
bool ExtendedSet<T, Compare>::contains(const T& value) const {
    return std::binary_search(data_.begin(), data_.end(), value, comp_);
}

template <typename T, typename Compare>
template <typename U>
bool ExtendedSet<T, Compare>::insertImpl(U&& value) {
    // Fast path: strictly greater than the current maximum (sorted input).
    if (data_.empty() || comp_(data_.back(), value)) {
        data_.push_back(std::forward<U>(value));
        return true;
    }
    const auto it = std::lower_bound(data_.begin(), data_.end(), value, comp_);
    if (it != data_.end() && !comp_(value, *it)) {
        return false;  // equivalent element already present
    }
    data_.insert(it, std::forward<U>(value));
    return true;
}

template <typename T, typename Compare>
bool ExtendedSet<T, Compare>::insert(const T& value) {
    return insertImpl(value);
}

template <typename T, typename Compare>
bool ExtendedSet<T, Compare>::insert(T&& value) {
    return insertImpl(std::move(value));
}

template <typename T, typename Compare>
bool ExtendedSet<T, Compare>::remove(const T& value) {
    const auto it = std::lower_bound(data_.begin(), data_.end(), value, comp_);
    if (it == data_.end() || comp_(value, *it)) {
        return false;
    }
    data_.erase(it);
    return true;
}

// ---------------------------------------------------------------------------
// Set operations
// ---------------------------------------------------------------------------

template <typename T, typename Compare>
void ExtendedSet<T, Compare>::unite(const ExtendedSet& other) {
    if (this == &other || other.data_.empty()) {
        return;
    }
    if (data_.empty()) {
        data_ = other.data_;
        return;
    }
    // Fast path: ranges do not overlap and `other` lies entirely above.
    if (comp_(data_.back(), other.data_.front())) {
        data_.insert(data_.end(), other.data_.begin(), other.data_.end());
        return;
    }

    std::vector<T> merged;
    merged.reserve(data_.size() + other.data_.size());

    // `data_` is discarded afterwards, so its elements can be moved out when
    // that cannot throw. Elements present in both sets are taken from `*this`.
    if constexpr (std::is_nothrow_move_constructible_v<T>) {
        std::set_union(std::make_move_iterator(data_.begin()),
                       std::make_move_iterator(data_.end()),
                       other.data_.begin(), other.data_.end(),
                       std::back_inserter(merged), comp_);
    } else {
        std::set_union(data_.begin(), data_.end(),
                       other.data_.begin(), other.data_.end(),
                       std::back_inserter(merged), comp_);
    }
    data_.swap(merged);
}

template <typename T, typename Compare>
void ExtendedSet<T, Compare>::intersect(const ExtendedSet& other) {
    if (this == &other) {
        return;
    }
    if (other.data_.empty()) {
        data_.clear();
        return;
    }

    auto out        = data_.begin();  // write position (always <= a)
    auto a          = data_.begin();
    const auto aEnd = data_.end();
    auto b          = other.data_.begin();
    const auto bEnd = other.data_.end();

    while (a != aEnd && b != bEnd) {
        if (comp_(*a, *b)) {
            ++a;
        } else if (comp_(*b, *a)) {
            ++b;
        } else {  // equivalent: keep
            if (out != a) {
                *out = std::move(*a);
            }
            ++out;
            ++a;
            ++b;
        }
    }
    data_.erase(out, data_.end());
}

template <typename T, typename Compare>
void ExtendedSet<T, Compare>::subtract(const ExtendedSet& other) {
    if (this == &other) {
        data_.clear();
        return;
    }
    if (data_.empty() || other.data_.empty()) {
        return;
    }

    auto out        = data_.begin();  // write position (always <= a)
    auto a          = data_.begin();
    const auto aEnd = data_.end();
    auto b          = other.data_.begin();
    const auto bEnd = other.data_.end();

    while (a != aEnd && b != bEnd) {
        if (comp_(*a, *b)) {  // only in A: keep
            if (out != a) {
                *out = std::move(*a);
            }
            ++out;
            ++a;
        } else if (comp_(*b, *a)) {
            ++b;
        } else {  // in both: drop
            ++a;
            ++b;
        }
    }
    // Remaining tail of A is unaffected by B. std::move requires that the
    // destination does not start inside the source range, hence the guard.
    if (out != a) {
        out = std::move(a, aEnd, out);
    } else {
        out = aEnd;
    }
    data_.erase(out, data_.end());
}

// ---------------------------------------------------------------------------
// Comparison
// ---------------------------------------------------------------------------

template <typename T, typename Compare>
bool ExtendedSet<T, Compare>::equals(const ExtendedSet& other) const {
    if (this == &other) {
        return true;
    }
    if (data_.size() != other.data_.size()) {
        return false;
    }
    return std::equal(data_.begin(), data_.end(), other.data_.begin(),
                      [this](const T& x, const T& y) { return !comp_(x, y) && !comp_(y, x); });
}

// ---------------------------------------------------------------------------
// Non-member operators
// ---------------------------------------------------------------------------

template <typename T, typename Compare>
bool operator==(const ExtendedSet<T, Compare>& a, const ExtendedSet<T, Compare>& b) {
    return a.equals(b);
}

template <typename T, typename Compare>
bool operator!=(const ExtendedSet<T, Compare>& a, const ExtendedSet<T, Compare>& b) {
    return !a.equals(b);
}

template <typename T, typename Compare>
ExtendedSet<T, Compare> operator|(ExtendedSet<T, Compare> a, const ExtendedSet<T, Compare>& b) {
    a.unite(b);
    return a;
}

template <typename T, typename Compare>
ExtendedSet<T, Compare> operator&(ExtendedSet<T, Compare> a, const ExtendedSet<T, Compare>& b) {
    a.intersect(b);
    return a;
}

template <typename T, typename Compare>
ExtendedSet<T, Compare> operator-(ExtendedSet<T, Compare> a, const ExtendedSet<T, Compare>& b) {
    a.subtract(b);
    return a;
}

template <typename T, typename Compare>
void swap(ExtendedSet<T, Compare>& a, ExtendedSet<T, Compare>& b) noexcept(noexcept(a.swap(b))) {
    a.swap(b);
}

}  // namespace algo

#endif  // ALGO_EXTENDED_SET_TPP