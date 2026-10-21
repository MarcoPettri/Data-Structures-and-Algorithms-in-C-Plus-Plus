/**
 * @file binary_tree.hpp
 * @brief Binary tree ADT (position-based, linked representation).
 */
#ifndef ADT_BINARY_TREE_HPP
#define ADT_BINARY_TREE_HPP

#include <cstddef>
#include <memory>
#include <stack>
#include <utility>

namespace adt {

/**
 * @brief Binary tree ADT.
 * @tparam T Element type stored in each node.
 *
 * Nodes are accessed through opaque, read-only positions (Position).
 * A null Position means "no node".
 */
template <typename T> class BinaryTree {
public:
  /** @brief A tree node. Only the owning tree can create or modify nodes. */
  class Node {
  public:
    /** @brief Returns the element stored in this node. */
    const T &element() const noexcept { return element_; }

  private:
    friend class BinaryTree<T>;
    explicit Node(T e) : element_(std::move(e)) {}

    T element_;
    std::unique_ptr<Node> left_;
    std::unique_ptr<Node> right_;
  };

  /** @brief Handle to a node. nullptr means no node. */
  using Position = const Node *;

  /** @brief Creates an empty tree. */
  BinaryTree() = default;

  BinaryTree(const BinaryTree &) = delete;
  BinaryTree &operator=(const BinaryTree &) = delete;

  /** @brief Move constructor; leaves @p other empty. */
  BinaryTree(BinaryTree &&other) noexcept;

  /** @brief Move assignment; leaves @p other empty. */
  BinaryTree &operator=(BinaryTree &&other) noexcept;

  /** @brief Destroys all nodes iteratively (no recursion). */
  ~BinaryTree() { clear(); }

  /** @name Accessors */
  ///@{
  /** @brief Number of nodes. */
  std::size_t size() const noexcept { return size_; }

  /** @brief True if the tree has no nodes. */
  bool isEmpty() const noexcept { return size_ == 0; }

  /** @brief Position of the root, or nullptr if the tree is empty. */
  Position root() const noexcept { return root_.get(); }

  /** @brief Left child of @p p, or nullptr.
   *  @throws std::invalid_argument if @p p is null. */
  Position left(Position p) const;

  /** @brief Right child of @p p, or nullptr.
   *  @throws std::invalid_argument if @p p is null. */
  Position right(Position p) const;

  /** @brief True if @p p has no children.
   *  @throws std::invalid_argument if @p p is null. */
  bool isExternal(Position p) const;

  /** @brief True if @p p has at least one child.
   *  @throws std::invalid_argument if @p p is null. */
  bool isInternal(Position p) const { return !isExternal(p); }
  ///@}

  /** @name Update operations */
  ///@{
  /**
   * @brief Creates the root.
   * @throws std::logic_error if the tree is not empty.
   */
  Position insertRoot(T e);

  /**
   * @brief Adds a left child to @p p.
   * @throws std::invalid_argument if @p p is null.
   * @throws std::logic_error if @p p already has a left child.
   */
  Position insertLeft(Position p, T e);

  /**
   * @brief Adds a right child to @p p.
   * @throws std::invalid_argument if @p p is null.
   * @throws std::logic_error if @p p already has a right child.
   */
  Position insertRight(Position p, T e);

  /** @brief Removes every node; O(n) time, no recursion. */
  void clear() noexcept;
  ///@}

private:
  std::unique_ptr<Node> root_;
  std::size_t size_ = 0;
};

} // namespace adt

#include "binary_tree.tpp"

#endif // ADT_BINARY_TREE_HPP