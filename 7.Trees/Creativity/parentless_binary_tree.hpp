/**
 * @file parentless_binary_tree.hpp
 * @brief Linked (proper) binary tree whose nodes store child pointers only.
 */
#ifndef PTREE_PARENTLESS_BINARY_TREE_HPP
#define PTREE_PARENTLESS_BINARY_TREE_HPP

#include <cstddef>
#include <vector>

namespace adt {

/**
 * @brief Proper binary tree (every node has 0 or 2 children) in which a node
 *        knows its children but not its parent.
 *
 * Operations that only go *down* (left, right, expandExternal, traversals) are
 * as fast as in the usual linked structure. Operations that need to go *up*
 * (parent, sibling, depth, removeAboveExternal) must search from the root and
 * therefore take Theta(n) time in the worst case.
 *
 * Positions stay valid until the node they refer to is removed. The class owns
 * its nodes; it is movable but not copyable.
 *
 * @tparam E Element type stored in each node (copy-constructible).
 */
template <typename E> class ParentlessBinaryTree {
  /// Linked node: element plus child pointers (deliberately no parent pointer).
  struct Node {
    E elt;                 ///< stored element
    Node *left = nullptr;  ///< left child or nullptr
    Node *right = nullptr; ///< right child or nullptr
    explicit Node(const E &e) : elt(e) {}
  };

  /// Result of a root-to-node search.
  struct Locator {
    Node *parent;      ///< parent of the target (nullptr if target is the root)
    Node *grandparent; ///< grandparent of the target (nullptr if none)
    std::size_t depth; ///< number of ancestors of the target
  };

public:
  /**
   * @brief Lightweight handle to a node (similar to an iterator).
   *
   * Because nodes have no parent pointer, a Position cannot navigate upward;
   * use ParentlessBinaryTree::parent() for that.
   */
  class Position {
  public:
    /** @brief Constructs a null position. */
    Position() noexcept = default;

    /** @brief Accesses the element. @pre !isNull() */
    E &operator*() const { return node_->elt; }

    /** @brief True if this position refers to no node. */
    bool isNull() const noexcept { return node_ == nullptr; }

    /** @brief Left child (null position if none). @pre !isNull() @par
     * Complexity O(1). */
    Position left() const { return Position(node_->left); }

    /** @brief Right child (null position if none). @pre !isNull() @par
     * Complexity O(1). */
    Position right() const { return Position(node_->right); }

    /** @brief True if the node has no children. @pre !isNull() @par Complexity
     * O(1). */
    bool isExternal() const {
      return node_->left == nullptr && node_->right == nullptr;
    }

    /** @brief True if the node has children. @pre !isNull() @par Complexity
     * O(1). */
    bool isInternal() const { return !isExternal(); }

    /** @brief Positions are equal iff they refer to the same node. */
    friend bool operator==(const Position &a, const Position &b) noexcept {
      return a.node_ == b.node_;
    }
    /** @brief Negation of operator==. */
    friend bool operator!=(const Position &a, const Position &b) noexcept {
      return a.node_ != b.node_;
    }

  private:
    friend class ParentlessBinaryTree;
    explicit Position(Node *n) noexcept : node_(n) {}
    Node *node_ = nullptr; ///< referenced node
  };

  /** @brief Constructs an empty tree. */
  ParentlessBinaryTree() noexcept = default;

  /** @brief Destroys all nodes iteratively. @par Complexity O(n) time, O(1)
   * extra space. */
  ~ParentlessBinaryTree();

  ParentlessBinaryTree(const ParentlessBinaryTree &) = delete;
  ParentlessBinaryTree &operator=(const ParentlessBinaryTree &) = delete;

  /** @brief Move constructor; @p other becomes empty. */
  ParentlessBinaryTree(ParentlessBinaryTree &&other) noexcept;

  /** @brief Move assignment; @p other becomes empty. */
  ParentlessBinaryTree &operator=(ParentlessBinaryTree &&other) noexcept;

  // ----------------------------------------------------------- accessors

  /** @brief Number of nodes. @par Complexity O(1) (maintained counter). */
  std::size_t size() const noexcept { return size_; }

  /** @brief True if the tree has no nodes. @par Complexity O(1). */
  bool empty() const noexcept { return size_ == 0; }

  /** @brief Root position (null if empty). @par Complexity O(1). */
  Position root() const noexcept { return Position(root_); }

  /** @brief True if @p p is the root. @par Complexity O(1): compares with the
   * stored root. */
  bool isRoot(const Position &p) const noexcept {
    return !p.isNull() && p.node_ == root_;
  }

  // --------------------------------------------- upward queries (searches)

  /**
   * @brief Parent of @p p.
   * @return Parent position, or a null position if @p p is the root.
   * @throws std::invalid_argument if @p p is null or not in this tree.
   * @par Complexity
   * Theta(n) worst case: the parent must be found by searching from the root.
   */
  Position parent(const Position &p) const;

  /**
   * @brief Sibling of @p p.
   * @throws std::logic_error if @p p is the root (it has no sibling).
   * @throws std::invalid_argument if @p p is null or not in this tree.
   * @par Complexity
   * Theta(n) worst case (one search for the parent, then O(1)).
   */
  Position sibling(const Position &p) const;

  /**
   * @brief Depth of @p p (number of proper ancestors; the root has depth 0).
   * @throws std::invalid_argument if @p p is null or not in this tree.
   * @par Complexity
   * Theta(n) worst case, using a single DFS that carries the depth.
   */
  std::size_t depth(const Position &p) const;

  /**
   * @brief Height of the tree (depth of the deepest node; 0 if empty or a
   * single node).
   * @par Complexity O(n).
   */
  std::size_t height() const;

  // ------------------------------------------------------------ traversals

  /**
   * @brief All positions in preorder.
   * @par Complexity O(n); iterative, so deep trees are safe.
   */
  std::vector<Position> positions() const;

  /**
   * @brief All positions in inorder (left, node, right).
   * @par Complexity O(n); iterative, so deep trees are safe.
   */
  std::vector<Position> inorder() const;

  // ------------------------------------------------------------- updates

  /**
   * @brief Creates the root of an empty tree.
   * @param e Element of the new root.
   * @return Position of the root.
   * @throws std::logic_error if the tree already has a root.
   * @par Complexity O(1).
   */
  Position addRoot(const E &e = E());

  /**
   * @brief Turns the external node @p p into an internal node with two new
   * leaves.
   * @param p Position of an external node.
   * @param left_elt Element of the new left child.
   * @param right_elt Element of the new right child.
   * @throws std::invalid_argument if @p p is null.
   * @throws std::logic_error if @p p is internal.
   * @pre @p p belongs to this tree (verifying that would cost O(n)).
   * @par Complexity O(1): only @p p itself is modified.
   */
  void expandExternal(const Position &p, const E &left_elt = E(),
                      const E &right_elt = E());

  /**
   * @brief Removes the external node @p p together with its parent; the
   *        sibling of @p p takes the place of the parent.
   * @param p Position of an external, non-root node.
   * @return Position of the promoted sibling.
   * @throws std::invalid_argument if @p p is null or not in this tree.
   * @throws std::logic_error if @p p is internal or is the root.
   * @par Complexity
   * Theta(n) worst case: one search finds the parent and grandparent, then
   * relinking is O(1). (O(1) in the version with parent pointers.)
   */
  Position removeAboveExternal(const Position &p);

  /** @brief Removes all nodes. @par Complexity O(n). */
  void clear() noexcept;

private:
  /**
   * @brief DFS from the root looking for @p target, tracking parent,
   *        grandparent and depth of every visited node.
   * @throws std::invalid_argument if @p target is null or not found.
   * @par Complexity O(n) time, O(height) extra space.
   */
  Locator locate(const Node *target) const;

  /** @brief Extracts the node of @p p; throws std::invalid_argument if null. */
  static Node *require_node(const Position &p);

  /** @brief Frees the subtree rooted at @p root without recursion or extra
   * memory. */
  static void destroy(Node *root) noexcept;

  Node *root_ = nullptr; ///< root node or nullptr
  std::size_t size_ = 0; ///< number of nodes
};

} // namespace adt

#include "parentless_binary_tree.tpp"

#endif // PTREE_PARENTLESS_BINARY_TREE_HPP