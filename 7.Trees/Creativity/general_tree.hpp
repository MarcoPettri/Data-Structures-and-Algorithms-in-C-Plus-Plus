/**
 * @file general_tree.hpp
 * @brief General (ordered, rooted) tree ADT — position-based, linked
 *        representation.
 *
 * Each node is accessed through an opaque Position handle.  Children are
 * kept in insertion order so the tree can faithfully represent ordered trees
 * like parse trees or expression trees.
 *
 * The implementation avoids recursion in all O(n) operations (construction,
 * destruction, traversal) to stay safe for arbitrarily deep trees.
 *
 * @tparam T  Type of the element stored in each node.
 */
#ifndef C74_GENERAL_TREE_HPP
#define C74_GENERAL_TREE_HPP

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace adt {

/**
 * @brief General ordered rooted tree.
 * @tparam T Element type stored at each node.
 */
template <typename T>
class GeneralTree {
public:
  // -------------------------------------------------------------------------
  // Internal node type
  // -------------------------------------------------------------------------

  /**
   * @brief A tree node.
   *
   * Only the owning GeneralTree can create nodes; callers receive const
   * Position handles.
   */
  class Node {
  public:
    /** @brief Returns the element stored in this node (read-only). */
    const T &element() const noexcept { return element_; }

    /** @brief Returns a read-only view of the ordered children. */
    const std::vector<std::unique_ptr<Node>> &children() const noexcept {
      return children_;
    }

  private:
    friend class GeneralTree<T>;

    explicit Node(T e) : element_(std::move(e)) {}

    T element_;
    std::vector<std::unique_ptr<Node>> children_;
  };

  // -------------------------------------------------------------------------
  // Public types
  // -------------------------------------------------------------------------

  /** @brief Read-only handle to a node; nullptr means "no node". */
  using Position = const Node *;

  // -------------------------------------------------------------------------
  // Lifecycle
  // -------------------------------------------------------------------------

  /** @brief Constructs an empty tree. */
  GeneralTree() = default;

  GeneralTree(const GeneralTree &) = delete;
  GeneralTree &operator=(const GeneralTree &) = delete;

  /** @brief Move constructor; leaves @p other empty. */
  GeneralTree(GeneralTree &&other) noexcept
      : root_(std::move(other.root_)),
        size_(std::exchange(other.size_, 0)) {}

  /** @brief Move-assignment; leaves @p other empty. */
  GeneralTree &operator=(GeneralTree &&other) noexcept {
    if (this != &other) {
      clear();
      root_ = std::move(other.root_);
      size_ = std::exchange(other.size_, 0);
    }
    return *this;
  }

  /** @brief Destroys all nodes iteratively (safe for very deep trees). */
  ~GeneralTree() { clear(); }

  // -------------------------------------------------------------------------
  // Accessors
  // -------------------------------------------------------------------------

  /** @brief Number of nodes in the tree. */
  std::size_t size() const noexcept { return size_; }

  /** @brief True if the tree contains no nodes. */
  bool isEmpty() const noexcept { return size_ == 0; }

  /** @brief Position of the root, or nullptr if empty. */
  Position root() const noexcept { return root_.get(); }

  /**
   * @brief Children of @p p (read-only, ordered).
   * @throws std::invalid_argument if @p p is null.
   */
  const std::vector<std::unique_ptr<Node>> &children(Position p) const {
    validate(p, "children");
    return p->children_;
  }

  /**
   * @brief True if @p p has no children (is a leaf).
   * @throws std::invalid_argument if @p p is null.
   */
  bool isExternal(Position p) const {
    validate(p, "isExternal");
    return p->children_.empty();
  }

  /**
   * @brief True if @p p has at least one child.
   * @throws std::invalid_argument if @p p is null.
   */
  bool isInternal(Position p) const { return !isExternal(p); }

  // -------------------------------------------------------------------------
  // Update operations
  // -------------------------------------------------------------------------

  /**
   * @brief Creates the root node with element @p e.
   * @throws std::logic_error if the tree is not empty.
   * @return Position of the new root.
   */
  Position insertRoot(T e) {
    if (root_) {
      throw std::logic_error("insertRoot: tree is not empty");
    }
    root_ = std::unique_ptr<Node>(new Node(std::move(e)));
    size_ = 1;
    return root_.get();
  }

  /**
   * @brief Appends a new child with element @p e to @p p.
   * @throws std::invalid_argument if @p p is null.
   * @return Position of the new child.
   */
  Position addChild(Position p, T e) {
    validate(p, "addChild");
    Node *n = const_cast<Node *>(p); // safe: tree owns every node
    n->children_.push_back(std::unique_ptr<Node>(new Node(std::move(e))));
    ++size_;
    return n->children_.back().get();
  }

  /** @brief Removes all nodes; iterative, O(n). */
  void clear() noexcept {
    // Use a stack of unique_ptrs so each node is destroyed with empty
    // children_ — no recursive destructor chain.
    std::vector<std::unique_ptr<Node>> pending;
    if (root_) {
      pending.push_back(std::move(root_));
    }
    while (!pending.empty()) {
      auto node = std::move(pending.back());
      pending.pop_back();
      for (auto &child : node->children_) {
        pending.push_back(std::move(child));
      }
      node->children_.clear(); // node destroyed with no children → no recursion
    }
    size_ = 0;
  }

private:
  // -------------------------------------------------------------------------
  // Helpers
  // -------------------------------------------------------------------------

  static void validate(Position p, const char *caller) {
    if (!p) {
      throw std::invalid_argument(std::string(caller) + ": null position");
    }
  }

  std::unique_ptr<Node> root_;
  std::size_t size_ = 0;
};

} // namespace adt

#endif // C74_GENERAL_TREE_HPP
