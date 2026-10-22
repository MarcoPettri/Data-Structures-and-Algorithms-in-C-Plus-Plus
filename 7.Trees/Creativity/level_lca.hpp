/**
 * @file level_lca.hpp
 * @brief Lowest common ancestor of two nodes of a binary tree, computed
 *        directly from their level-numbering labels.
 *
 * Level numbering: f(root) = 0; a left child of p has f = 2 f(p) + 1; a right
 * child of p has f = 2 f(p) + 2. No tree is needed to use these functions.
 */
#ifndef C7_LEVEL_LCA_HPP
#define C7_LEVEL_LCA_HPP

#include <cstdint>
#include <limits>
#include <stdexcept>

namespace adt {

namespace detail {

/**
 * @brief Number of bits needed to write @p x (0 for x == 0).
 * @par Complexity O(1): six shift-and-test steps for a 64-bit word.
 */
constexpr unsigned bitLength(std::uint64_t x) noexcept {
  unsigned n = 0;
  for (unsigned shift = 32; shift != 0; shift >>= 1) {
    if ((x >> shift) != 0) {
      x >>= shift;
      n += shift;
    }
  }
  return n + static_cast<unsigned>(x); // x is now 0 or 1
}

/**
 * @brief One-based label g = f + 1, whose binary form is "1" followed by the
 *        root-to-node path (0 = left, 1 = right).
 * @throws std::invalid_argument if @p f + 1 would overflow 64 bits.
 */
constexpr std::uint64_t oneBasedLabel(std::uint64_t f) {
  if (f == std::numeric_limits<std::uint64_t>::max()) {
    throw std::invalid_argument("level number is too large (f + 1 overflows)");
  }
  return f + 1;
}

} // namespace detail

/**
 * @brief Depth of the node whose level number is @p f (the root has depth 0).
 *
 * The depth is floor(log2(f + 1)).
 *
 * @param f Level-numbering label of a node.
 * @return Number of proper ancestors of that node.
 * @throws std::invalid_argument if @p f is the largest 64-bit value.
 *
 * @par Complexity O(1).
 */
constexpr unsigned levelNumberDepth(std::uint64_t f) {
  return detail::bitLength(detail::oneBasedLabel(f)) - 1;
}

/**
 * @brief Level number of the LCA of two nodes, from their level numbers.
 *
 * The LCA is the lowest node having both nodes as descendants, where a node
 * counts as a descendant of itself.
 *
 * Algorithm: with g = f + 1, the binary form of g is a leading 1 followed by
 * the root-to-node path. Shift the deeper label right until both have the
 * same length (this is the ancestor of the deeper node at the shallower
 * depth), then drop every bit from the highest differing bit downwards. What
 * remains is the common prefix, i.e. the label of the LCA.
 *
 * @param fv Level number of the first node.
 * @param fw Level number of the second node (may equal @p fv).
 * @return Level number of the lowest common ancestor.
 * @throws std::invalid_argument if @p fv or @p fw is the largest 64-bit value.
 *
 * @par Complexity
 * O(1): a constant number of word operations (labels are assumed to fit in
 * 64 bits, which holds for trees of height at most 63).
 */
constexpr std::uint64_t lcaLevelNumber(std::uint64_t fv, std::uint64_t fw) {
  std::uint64_t gv = detail::oneBasedLabel(fv);
  std::uint64_t gw = detail::oneBasedLabel(fw);

  const unsigned lv = detail::bitLength(gv);
  const unsigned lw = detail::bitLength(gw);
  if (lv > lw) {
    gv >>= (lv - lw); // ancestor of v at the depth of w
  } else {
    gw >>= (lw - lv); // ancestor of w at the depth of v
  }

  // Same length and same leading 1, so the XOR has at most L - 1 <= 63 bits.
  const unsigned k = detail::bitLength(gv ^ gw);
  return (gv >> k) - 1;
}

/**
 * @brief Same result as lcaLevelNumber(), obtained by climbing with the
 *        parent formula (f - 1) / 2.
 *
 * The node with the larger label is never shallower than the other, and is
 * not the LCA unless the labels are equal, so it is moved to its parent
 * until the labels coincide.
 *
 * @param fv Level number of the first node.
 * @param fw Level number of the second node (may equal @p fv).
 * @return Level number of the lowest common ancestor.
 *
 * @par Complexity
 * O(depth(v) + depth(w)) = O(h) steps, each O(1). Needs no precondition on
 * the labels, so it also serves as a reference implementation.
 */
constexpr std::uint64_t lcaLevelNumberByClimbing(std::uint64_t fv,
                                                 std::uint64_t fw) noexcept {
  while (fv != fw) {
    if (fv > fw) {
      fv = (fv - 1) / 2;
    } else {
      fw = (fw - 1) / 2;
    }
  }
  return fv;
}

} // namespace adt

#endif // C7_LEVEL_LCA_HPP