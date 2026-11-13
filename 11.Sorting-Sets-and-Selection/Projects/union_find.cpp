/**
 * @file union_find.cpp
 * @brief Implementation of algo::UnionFind.
 */

#include "union_find.hpp"

#include <stdexcept>
#include <utility>

namespace algo {

UnionFind::UnionFind(size_type n) : link_(n, link_type{-1}), numSets_(n) {}

UnionFind::size_type UnionFind::makeSet() {
  const size_type id = link_.size();
  link_.push_back(link_type{-1}); // strong guarantee: nothing else changed yet
  ++numSets_;
  return id;
}

void UnionFind::check(size_type x) const {
  if (x >= link_.size()) {
    throw std::out_of_range("UnionFind: element id out of range");
  }
}

UnionFind::size_type UnionFind::findUnchecked(size_type x) noexcept {
  // Pass 1: locate the root.
  size_type root = x;
  while (link_[root] >= 0) {
    root = static_cast<size_type>(link_[root]);
  }
  // Pass 2: path compression — re-parent every node on the path to the root.
  while (x != root) {
    const size_type next = static_cast<size_type>(link_[x]);
    link_[x] = static_cast<link_type>(root);
    x = next;
  }
  return root;
}

UnionFind::size_type UnionFind::find(size_type x) {
  check(x);
  return findUnchecked(x);
}

bool UnionFind::unite(size_type a, size_type b) {
  check(a);
  check(b);

  size_type ra = findUnchecked(a);
  size_type rb = findUnchecked(b);
  if (ra == rb) {
    return false;
  }

  // Roots hold -size, so the more negative entry is the larger tree.
  // `ra` must end up as the larger one; on a tie the root of `a` wins.
  if (link_[ra] > link_[rb]) {
    std::swap(ra, rb);
  }
  link_[ra] += link_[rb]; // sizes add up (both negative)
  link_[rb] =
      static_cast<link_type>(ra); // hang the smaller tree under the larger
  --numSets_;
  return true;
}

bool UnionFind::connected(size_type a, size_type b) {
  check(a);
  check(b);
  return findUnchecked(a) == findUnchecked(b);
}

UnionFind::size_type UnionFind::setSize(size_type x) {
  check(x);
  return static_cast<size_type>(-link_[findUnchecked(x)]);
}

UnionFind::size_type UnionFind::depth(size_type x) const {
  check(x);
  size_type d = 0;
  while (link_[x] >= 0) {
    x = static_cast<size_type>(link_[x]);
    ++d;
  }
  return d;
}

} // namespace algo