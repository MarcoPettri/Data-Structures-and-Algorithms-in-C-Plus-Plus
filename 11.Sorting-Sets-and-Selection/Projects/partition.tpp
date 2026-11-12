/**
 * @file partition.tpp
 * @brief Template implementation of algo::Partition (included by partition.hpp).
 *
 * Do not include this file directly; include @ref partition.hpp.
 */

#ifndef ALGO_PARTITION_TPP
#define ALGO_PARTITION_TPP

#include <stdexcept>

namespace algo {

template <typename T, typename Hash, typename KeyEqual>
bool Partition<T, Hash, KeyEqual>::makeSet(const T& x) {
    const auto [it, inserted] = ids_.try_emplace(x, elems_.size());
    if (!inserted) {
        return false;
    }
    // Keep the three containers in sync; roll back on failure (strong guarantee).
    try {
        elems_.push_back(x);
        try {
            sets_.makeSet();
        } catch (...) {
            elems_.pop_back();
            throw;
        }
    } catch (...) {
        ids_.erase(it);
        throw;
    }
    return true;
}

template <typename T, typename Hash, typename KeyEqual>
bool Partition<T, Hash, KeyEqual>::contains(const T& x) const {
    return ids_.find(x) != ids_.end();
}

template <typename T, typename Hash, typename KeyEqual>
typename Partition<T, Hash, KeyEqual>::size_type
Partition<T, Hash, KeyEqual>::idOf(const T& x) const {
    const auto it = ids_.find(x);
    if (it == ids_.end()) {
        throw std::out_of_range("Partition: unknown element");
    }
    return it->second;
}

template <typename T, typename Hash, typename KeyEqual>
const T& Partition<T, Hash, KeyEqual>::find(const T& x) {
    return elems_[sets_.find(idOf(x))];
}

template <typename T, typename Hash, typename KeyEqual>
bool Partition<T, Hash, KeyEqual>::unite(const T& a, const T& b) {
    const size_type ia = idOf(a);
    const size_type ib = idOf(b);
    return sets_.unite(ia, ib);
}

template <typename T, typename Hash, typename KeyEqual>
bool Partition<T, Hash, KeyEqual>::connected(const T& a, const T& b) {
    const size_type ia = idOf(a);
    const size_type ib = idOf(b);
    return sets_.connected(ia, ib);
}

template <typename T, typename Hash, typename KeyEqual>
typename Partition<T, Hash, KeyEqual>::size_type
Partition<T, Hash, KeyEqual>::setSize(const T& x) {
    return sets_.setSize(idOf(x));
}

}  // namespace algo

#endif  // ALGO_PARTITION_TPP