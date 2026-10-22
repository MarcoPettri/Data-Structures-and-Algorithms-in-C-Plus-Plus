/**
 * @file linked_binary_tree.hpp
 * @brief Generic linked (proper) binary tree with checked, position-based
 *        access.
 */
#ifndef C7_LINKED_BINARY_TREE_HPP
#define C7_LINKED_BINARY_TREE_HPP

#include <cstddef>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "tree_exceptions.hpp"

namespace adt {

/** @brief Order in which LinkedBinaryTree::positions() lists the nodes. */
enum class Traversal { Preorder, Inorder, Postorder, LevelOrder };

/**
 * @brief Proper binary tree (every node has 0 or 2 children) stored as linked
 *        nodes with parent pointers.
 *
 * The tree is manipulated through Position handles, which behave like
 * iterators: a position stays valid until its node is removed. Positions
 * survive moving and swapping the tree (the nodes keep their identity), but a
 * copy is a different tree, so positions of the original are rejected by it.
 *
 * Misuse is reported with the exceptions of tree_exceptions.hpp. Mutating
 * operations give the strong exception guarantee.
 *
 * Requirements on @p E: move-constructible for addRoot()/expandExternal(),
 * copy-constructible for copying the tree, non-throwing destructor.
 * Default-constructibility is only needed to call addRoot() or
 * expandExternal() without element arguments.
 *
 * @code
 * adt::LinkedBinaryTree<std::string> t;
 * auto root = t.addRoot("a");
 * t.expandExternal(root, "b", "c");
 * std::string &x = *root.left();           // "b"
 * for (auto p : t.positions(adt::Traversal::Inorder)) { ... }
 * @endcode
 *
 * @tparam E Element type stored in each node.
 */
template <typename E> class LinkedBinaryTree {
  /// Linked node: element, three links and the identity of the owning tree.
  struct Node {
    E elt;             ///< stored element
    Node *parent;      ///< parent (nullptr for the root)
    Node *left;        ///< left child (nullptr for an external node)
    Node *right;       ///< right child (nullptr for an external node)
    const void *owner; ///< identity token of the tree that owns this node

    template <typename... Args>
    Node(Node *par, const void *own, Args &&...args)
        : elt(std::forward<Args>(args)...), parent(par), left(nullptr),
          right(nullptr), owner(own) {}
  };

public:
  // -------------------------------------------------------------------------
  // Positions
  // -------------------------------------------------------------------------

  /**
   * @brief Handle to a node of the tree.
   *
   * @tparam IsConst If true the handle only gives read access to the element.
   *
   * A default-constructed position is null. Navigation works on the node
   * alone and cannot tell whether the node is still alive; see the class
   * documentation.
   */
  template <bool IsConst> class BasicPosition {
  public:
    using value_type = E;
    using reference = std::conditional_t<IsConst, const E &, E &>;
    using pointer = std::conditional_t<IsConst, const E *, E *>;

    /** @brief Constructs a null position. */
    BasicPosition() noexcept = default;

    /** @brief Converts a read/write position to a read-only one. */
    template <bool OtherConst,
              typename = std::enable_if_t<IsConst && !OtherConst>>
    BasicPosition(const BasicPosition<OtherConst> &other) noexcept
        : node_(other.node_) {}

    /** @brief True if this position refers to no node. */
    bool isNull() const noexcept { return node_ == nullptr; }

    /**
     * @brief Accesses the element.
     * @throws InvalidPosition if the position is null.
     */
    reference operator*() const { return access("operator*").elt; }

    /**
     * @brief Accesses the element's members.
     * @throws InvalidPosition if the position is null.
     */
    pointer operator->() const {
      return std::addressof(access("operator->").elt);
    }

    /**
     * @brief True if the node is the root.
     * @throws InvalidPosition if the position is null.
     */
    bool isRoot() const { return access("isRoot").parent == nullptr; }

    /**
     * @brief True if the node has no children.
     * @throws InvalidPosition if the position is null.
     */
    bool isExternal() const {
      const Node &v = access("isExternal");
      return v.left == nullptr && v.right == nullptr;
    }

    /**
     * @brief True if the node has children.
     * @throws InvalidPosition if the position is null.
     */
    bool isInternal() const {
      const Node &v = access("isInternal");
      return v.left != nullptr || v.right != nullptr;
    }

    /**
     * @brief Left child.
     * @throws InvalidPosition if the position is null.
     * @throws BoundaryViolation if the node is external.
     */
    BasicPosition left() const {
      Node &v = access("left");
      if (v.left == nullptr) {
        throw BoundaryViolation(
            "LinkedBinaryTree::Position::left: external node has no children");
      }
      return BasicPosition(v.left);
    }

    /**
     * @brief Right child.
     * @throws InvalidPosition if the position is null.
     * @throws BoundaryViolation if the node is external.
     */
    BasicPosition right() const {
      Node &v = access("right");
      if (v.right == nullptr) {
        throw BoundaryViolation(
            "LinkedBinaryTree::Position::right: external node has no children");
      }
      return BasicPosition(v.right);
    }

    /**
     * @brief Parent.
     * @throws InvalidPosition if the position is null.
     * @throws BoundaryViolation if the node is the root.
     */
    BasicPosition parent() const {
      Node &v = access("parent");
      if (v.parent == nullptr) {
        throw BoundaryViolation(
            "LinkedBinaryTree::Position::parent: the root has no parent");
      }
      return BasicPosition(v.parent);
    }

    /**
     * @brief The other child of this node's parent.
     * @throws InvalidPosition if the position is null.
     * @throws BoundaryViolation if the node is the root.
     */
    BasicPosition sibling() const {
      Node &v = access("sibling");
      if (v.parent == nullptr) {
        throw BoundaryViolation(
            "LinkedBinaryTree::Position::sibling: the root has no sibling");
      }
      return BasicPosition(v.parent->left == &v ? v.parent->right
                                                : v.parent->left);
    }

    /** @brief Positions are equal iff they refer to the same node. */
    friend bool operator==(const BasicPosition &a,
                           const BasicPosition &b) noexcept {
      return a.node_ == b.node_;
    }

    /** @brief Negation of operator==. */
    friend bool operator!=(const BasicPosition &a,
                           const BasicPosition &b) noexcept {
      return a.node_ != b.node_;
    }

  private:
    friend class LinkedBinaryTree;
    template <bool> friend class BasicPosition;

    explicit BasicPosition(Node *v) noexcept : node_(v) {}

    /// The referenced node; throws InvalidPosition if there is none.
    Node &access(const char *caller) const {
      if (node_ == nullptr) {
        throw InvalidPosition(std::string("LinkedBinaryTree::Position::") +
                              caller + ": null position");
      }
      return *node_;
    }

    Node *node_ = nullptr; ///< referenced node
  };

  using value_type = E;
  using Position = BasicPosition<false>;     ///< read/write handle
  using ConstPosition = BasicPosition<true>; ///< read-only handle
  using PositionList = std::vector<Position>;
  using ConstPositionList = std::vector<ConstPosition>;

  // -------------------------------------------------------------------------
  // Lifecycle
  // -------------------------------------------------------------------------

  /** @brief Constructs an empty tree. */
  LinkedBinaryTree() noexcept = default;

  /**
   * @brief Deep copy. Positions of @p other are not valid in the copy.
   * @throws whatever E's copy constructor or the allocator throws; nothing
   *         leaks in that case.
   * @par Complexity O(n).
   */
  LinkedBinaryTree(const LinkedBinaryTree &other);

  /** @brief Takes over the nodes of @p other (which becomes empty). O(1). */
  LinkedBinaryTree(LinkedBinaryTree &&other) noexcept;

  /**
   * @brief Copy assignment (copy-and-swap: strong guarantee).
   * @par Complexity O(n).
   */
  LinkedBinaryTree &operator=(const LinkedBinaryTree &other);

  /** @brief Move assignment; the old contents are destroyed. O(n) for that. */
  LinkedBinaryTree &operator=(LinkedBinaryTree &&other) noexcept;

  /** @brief Destroys all nodes iteratively. @par Complexity O(n). */
  ~LinkedBinaryTree();

  /** @brief Exchanges the contents of two trees; positions follow the nodes.
   * O(1). */
  void swap(LinkedBinaryTree &other) noexcept;

  /** @brief Same as the member swap(). */
  friend void swap(LinkedBinaryTree &a, LinkedBinaryTree &b) noexcept {
    a.swap(b);
  }

  // -------------------------------------------------------------------------
  // Queries
  // -------------------------------------------------------------------------

  /** @brief Number of nodes. @par Complexity O(1). */
  std::size_t size() const noexcept { return size_; }

  /** @brief True if the tree has no nodes. @par Complexity O(1). */
  bool empty() const noexcept { return size_ == 0; }

  /** @brief Root position (null if the tree is empty). O(1). */
  Position root() noexcept { return Position(root_); }

  /** @brief Read-only root position (null if the tree is empty). O(1). */
  ConstPosition root() const noexcept { return ConstPosition(root_); }

  /**
   * @brief True if @p p is a non-null position of a node of this tree.
   *
   * The node must still be alive (see the class documentation).
   *
   * @par Complexity O(1).
   */
  bool owns(const ConstPosition &p) const noexcept;

  /**
   * @brief Number of proper ancestors of @p p (the root has depth 0).
   * @throws InvalidPosition if @p p is null or not a position of this tree.
   * @par Complexity O(depth(p)).
   */
  std::size_t depth(const ConstPosition &p) const;

  /**
   * @brief Height of the tree: the depth of its deepest node.
   * @throws InvalidOperation if the tree is empty.
   * @par Complexity O(n).
   */
  std::size_t height() const;

  /**
   * @brief Height of the subtree rooted at @p p (0 for an external node).
   * @throws InvalidPosition if @p p is null or not a position of this tree.
   * @par Complexity O(size of the subtree).
   */
  std::size_t height(const ConstPosition &p) const;

  /**
   * @brief All positions, in the given order.
   * @param order Preorder (default), Inorder, Postorder or LevelOrder.
   * @throws std::invalid_argument if @p order is not an enumerator.
   * @par Complexity O(n); iterative, so deep trees are safe.
   */
  PositionList positions(Traversal order = Traversal::Preorder);

  /** @copydoc positions(Traversal) */
  ConstPositionList positions(Traversal order = Traversal::Preorder) const;

  // -------------------------------------------------------------------------
  // Updates
  // -------------------------------------------------------------------------

  /**
   * @brief Creates the root of an empty tree.
   * @param e Element of the new root (value-initialized if omitted).
   * @return Position of the root.
   * @throws InvalidOperation if the tree already has a root.
   * @par Complexity O(1).
   */
  Position addRoot(E e = E());

  /**
   * @brief Turns the external node @p p into an internal node with two new
   *        external children.
   * @param p Position of an external node of this tree.
   * @param leftElt Element of the new left child (value-initialized if
   * omitted).
   * @param rightElt Element of the new right child (value-initialized if
   * omitted).
   * @throws InvalidPosition if @p p is null or not a position of this tree.
   * @throws InvalidOperation if @p p is already internal.
   * @par Complexity O(1). Strong guarantee.
   */
  void expandExternal(const Position &p, E leftElt = E(), E rightElt = E());

  /**
   * @brief Removes the external node @p p together with its parent; the
   *        sibling of @p p takes the place of the parent.
   *
   * Invalidates @p p and the position of its parent.
   *
   * @param p Position of an external, non-root node of this tree.
   * @return Position of the promoted sibling.
   * @throws InvalidPosition if @p p is null or not a position of this tree.
   * @throws InvalidOperation if @p p is internal.
   * @throws BoundaryViolation if @p p is the root (it has no parent).
   * @par Complexity O(1). Strong guarantee.
   */
  Position removeAboveExternal(const Position &p);

  /** @brief Removes every node; invalidates all positions. @par Complexity
   * O(n). */
  void clear() noexcept;

private:
  /// Checks that @p v is non-null and owned by this tree; returns it.
  Node *requireNode(Node *v, const char *caller) const;

  /// Creates this tree's identity token if it does not exist yet.
  void ensureToken();

  /// Allocates a node owned by this tree. Throws whatever E's constructor or
  /// the allocator throws, in which case nothing is allocated.
  template <typename... Args>
  Node *makeNode(Node *parent, Args &&...args) const {
    return new Node(parent, token_.get(), std::forward<Args>(args)...);
  }

  /// Deep-copies @p other into this (empty) tree, iteratively.
  void copyFrom(const LinkedBinaryTree &other);

  /// Frees the whole tree below and including @p root without recursion or
  /// extra memory, using the parent links.
  static void destroyAll(Node *root) noexcept;

  std::vector<Node *> collect(Traversal order) const;
  std::vector<Node *> preorderNodes() const;
  std::vector<Node *> inorderNodes() const;
  std::vector<Node *> postorderNodes() const;
  std::vector<Node *> levelOrderNodes() const;

  template <typename PositionT>
  static std::vector<PositionT> wrap(const std::vector<Node *> &nodes) {
    std::vector<PositionT> out;
    out.reserve(nodes.size());
    for (Node *v : nodes) {
      out.push_back(PositionT(v));
    }
    return out;
  }

  Node *root_ = nullptr;        ///< root node or nullptr
  std::size_t size_ = 0;        ///< number of nodes
  std::unique_ptr<char> token_; ///< identity of this tree (address is unique)
};

} // namespace adt

#include "linked_binary_tree.tpp"

#endif // C7_LINKED_BINARY_TREE_HPP