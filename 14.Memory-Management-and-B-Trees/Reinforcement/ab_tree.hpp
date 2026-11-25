/**
 * @file ab_tree.hpp
 * @brief Ordered set implemented with a parameterised (a, b) tree (interface).
 *
 * An (a, b) tree is a multi-way search tree where every internal node has
 * between @p a and @p b children (a >= 2, b >= 2a-1) and all leaves sit at
 * the same depth. With @p a=2, @p b=4 it reduces to the classic 2-3-4 tree.
 *
 * The implementation lives in ab_tree.tpp, which is included at the bottom of
 * this file (the standard pattern for class templates).
 *
 * Complexity summary (n = number of keys stored):
 *  - contains ............................................. O(log_a n)
 *  - insert / erase ...................................... O(b * log_a n)
 *  - size / empty / height ............................... O(1)
 *  - to_sorted_vector / validate / destructor / copy ..... O(n)
 *
 */

#ifndef DS_AB_TREE_HPP
#define DS_AB_TREE_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace ds {

// ============================================================================
// Node
// ============================================================================

/**
 * @brief Internal / leaf node of an (a, b) tree.
 *
 * A c-node stores exactly c-1 sorted keys and c child pointers.
 * Leaf nodes have an empty @c children vector.
 *
 * @tparam Key  The key type.
 */
template <typename Key> struct ABNode {
  std::vector<Key> keys; ///< sorted separator keys
  std::vector<std::unique_ptr<ABNode<Key>>> children; ///< child subtrees

  /** @brief True iff this node is a leaf (no children). */
  [[nodiscard]] bool is_leaf() const noexcept { return children.empty(); }

  /** @brief Degree: number of children (0 for a leaf). */
  [[nodiscard]] std::size_t degree() const noexcept { return children.size(); }
};

// ============================================================================
// (a, b) Tree
// ============================================================================

/**
 * @class ABTree
 * @brief Ordered set backed by an (a, b) multi-way search tree.
 *
 * Structural invariants maintained at all times:
 *  1. All leaves are at the same depth (perfectly height-balanced).
 *  2. The root has [2, B] children, or is a leaf when size() <= B-1.
 *  3. Every non-root internal node has [A, B] children.
 *  4. Each c-node stores c-1 sorted keys; keys[i] separates children[i]
 *     and children[i+1].
 *
 * @tparam Key  Key type; must be LessThanComparable (operator<).
 * @tparam A    Minimum number of children per internal node (default 2).
 * @tparam B    Maximum number of children per internal node (default 4).
 *
 * @note Requirement: B >= 2*A - 1 (classic (a,b)-tree feasibility condition).
 */
template <typename Key, std::size_t A = 2, std::size_t B = 4> class ABTree {
  static_assert(A >= 2, "a must be >= 2");
  static_assert(B >= 2 * A - 1, "b must satisfy b >= 2a - 1");

public:
  using key_type = Key;
  using node_type = ABNode<Key>;
  using node_ptr = std::unique_ptr<node_type>;

  /// @name Construction, assignment, destruction
  /// @{

  /** @brief Constructs an empty (a, b) tree. */
  ABTree();

  /** @brief Deep copy. O(n). */
  ABTree(const ABTree &other);

  /** @brief Copy assignment (copy-and-swap). O(n). */
  ABTree &operator=(ABTree other) noexcept;

  /** @brief Move constructor. O(1); @p other is left empty. */
  ABTree(ABTree &&other) noexcept = default;

  /** @brief Destroys the tree and all its nodes. O(n). */
  ~ABTree() = default;

  /** @brief Exchanges contents with @p other. O(1). */
  void swap(ABTree &other) noexcept;
  /// @}

  /// @name Capacity
  /// @{

  /**
   * @brief Returns the number of keys stored.
   * @section complexity Time: O(1). Space: O(1).
   */
  [[nodiscard]] std::size_t size() const noexcept { return size_; }

  /**
   * @brief Returns true if the tree stores no keys.
   * @section complexity Time: O(1). Space: O(1).
   */
  [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

  /**
   * @brief Returns the height (number of edges on a root-to-leaf path).
   * @section complexity Time: O(1). Space: O(1).
   */
  [[nodiscard]] std::size_t height() const noexcept { return height_; }
  /// @}

  /// @name Lookup
  /// @{

  /**
   * @brief Returns true iff @p key is present in the tree.
   *
   * @param key  The key to search for.
   * @return     true if found, false otherwise.
   *
   * @section complexity
   *   Time: $O(\log_a n)$ — O(h) levels, O(b) key comparisons per level.
   *   Aux space: $O(h)$ call stack. n = number of keys, h = height.
   */
  [[nodiscard]] bool contains(const Key &key) const noexcept;
  /// @}

  /// @name Modifiers
  /// @{

  /**
   * @brief Inserts @p key into the tree.
   *
   * ### Algorithm (bottom-up split)
   * 1. Descend from the root to the correct leaf, recording the ancestor path.
   * 2. Insert @p key in sorted position within the leaf.
   * 3. Walk back up: if any node has B keys (overflow), **split** it at its
   *    median index m = ⌊B/2⌋:
   *      - keys [0..m-1]   stay in the original (left) node.
   *      - keys [m+1..B-1] move into a newly allocated right sibling.
   *      - keys [m]        is promoted into the parent.
   *      - The right sibling is inserted as the next child of the parent.
   * 4. If the root overflows, a new root is created from the promoted key
   *    and the two halves; the tree height increases by 1.
   *
   * @param key  The key to insert.
   * @return     true if inserted, false if @p key was already present.
   *
   * @section complexity
   *   Time: $O(b \cdot \log_a n)$ — O(h) levels, O(b) work per split.
   *   Aux space: $O(h)$ for the descent path.
   */
  bool insert(const Key &key);

  /**
   * @brief Removes @p key from the tree.
   *
   * ### Algorithm (bottom-up merge / transfer)
   * 1. Descend from the root to a leaf, recording the path.
   *    - If @p key is found at an **internal** node at index i, record it and
   *      continue into children[i+1] to find the in-order successor (leftmost
   *      key of that subtree's leftmost leaf).
   * 2. Swap the internal copy with its successor; delete the successor from
   *    the leaf — deletion is always performed at a leaf.
   * 3. Walk back up: if a node **underflows** (< A children, or for a leaf
   *    < A-1 keys), fix it:
   *      a. **Transfer (rotation)**: if an adjacent sibling has a key to spare,
   *         rotate one key through the parent separator.
   *      b. **Merge**: otherwise, merge the node with a sibling and the parent
   *         separator into one node. The parent loses one key and one child
   *         pointer — it may then underflow itself; propagate upward.
   * 4. If the root is left with 0 keys but 1 child, replace the root with
   *    that child; the tree height decreases by 1.
   *
   * @param key  The key to remove.
   * @return     true if erased, false if @p key was not present.
   *
   * @section complexity
   *   Time: $O(b \cdot \log_a n)$ — O(h) levels, O(b) work per merge/transfer.
   *   Aux space: $O(h)$ for the descent path.
   */
  bool erase(const Key &key);
  /// @}

  /// @name Utilities
  /// @{

  /**
   * @brief Returns all keys in ascending order via in-order traversal.
   *
   * @return std::vector<Key> containing every stored key, sorted ascending.
   *
   * @section complexity Time: $O(n)$. Space: $O(n + h)$.
   */
  [[nodiscard]] std::vector<Key> to_sorted_vector() const;

  /**
   * @brief Validates every structural invariant of the tree.
   *
   * Checks: degree bounds [A,B] on every node, 2-B on the root, uniform
   * leaf depth equal to height(), key monotonicity across the whole tree,
   * and the c-node key-count / child-count relationship.
   *
   * @return true if the tree is structurally valid.
   *
   * @section complexity Time: $O(n)$. Space: $O(h)$ call stack.
   */
  [[nodiscard]] bool validate() const;
  /// @}

private:
  // ---- Navigation helpers ---- (defined in ab_tree.tpp) -------------------
  static std::size_t lower_bound_idx(const std::vector<Key> &keys,
                                     const Key &key);
  static bool search(const node_type *node, const Key &key) noexcept;
  static node_ptr clone(const node_ptr &src);
  static bool do_split(node_type *node, bool is_leaf, Key &promoted,
                       node_ptr &right);
  static void transfer_from_left(node_type *parent, std::size_t ci,
                                 node_type *child, node_type *left_sib);
  static void transfer_from_right(node_type *parent, std::size_t ci,
                                  node_type *child, node_type *right_sib);
  static void merge_into_left(node_type *parent, std::size_t sep_idx,
                              node_type *left, node_type *right);
  static void collect(const node_type *node, std::vector<Key> &out);
  bool validate_node(const node_type *node, bool is_root, std::optional<Key> lo,
                     std::optional<Key> hi, std::size_t depth,
                     std::size_t expected_height) const;

  // ---- Data members -------------------------------------------------------
  node_ptr root_;
  std::size_t size_;
  std::size_t height_; ///< edges on a root-to-leaf path
};

/** @brief Non-member swap. */
template <typename Key, std::size_t A, std::size_t B>
void swap(ABTree<Key, A, B> &a, ABTree<Key, A, B> &b) noexcept {
  a.swap(b);
}

} // namespace ds

#include "ab_tree.tpp"

#endif // DS_AB_TREE_HPP
