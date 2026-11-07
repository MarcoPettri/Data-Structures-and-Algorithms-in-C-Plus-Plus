/**
 * @file leftist_heap.hpp
 * @brief Leftist Heap implementation of the Mergeable Heap ADT supporting O(log
 * n) worst-case union.
 */

#pragma once

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <utility>

template <typename Key, typename Value> class LeftistHeap {
public:
  struct Entry {
    Key key;
    Value value;
  };

private:
  struct Node {
    Entry entry;
    Node *left = nullptr;
    Node *right = nullptr;
    size_t rank = 1;

    explicit Node(Key k, Value v) : entry{k, v} {}
  };

  Node *root_ = nullptr;
  size_t size_ = 0;

  /**
   * @brief Helper to get the rank of a node safely (null nodes have rank 0).
   */
  static size_t getRank(Node *node) { return node ? node->rank : 0; }

  /**
   * @brief Core helper: merges two leftist heap subtrees in O(log n + log m)
   * time.
   */
  static Node *mergeNodes(Node *h1, Node *h2) {
    if (!h1)
      return h2;
    if (!h2)
      return h1;

    // Ensure h1 has the smaller key
    if (h2->entry.key < h1->entry.key) {
      std::swap(h1, h2);
    }

    // Recursively merge h1's right child with h2
    h1->right = mergeNodes(h1->right, h2);

    // Maintain the leftist property: rank(left) >= rank(right)
    if (getRank(h1->left) < getRank(h1->right)) {
      std::swap(h1->left, h1->right);
    }

    // Update rank
    h1->rank = getRank(h1->right) + 1;
    return h1;
  }

  /**
   * @brief Recursive helper to delete all nodes.
   */
  static void clearTree(Node *node) {
    if (!node)
      return;
    clearTree(node->left);
    clearTree(node->right);
    delete node;
  }

public:
  LeftistHeap() = default;

  // Destructor
  ~LeftistHeap() { clearTree(root_); }

  // Disable copy constructor to prevent shallow pointer duplication
  LeftistHeap(const LeftistHeap &) = delete;
  LeftistHeap &operator=(const LeftistHeap &) = delete;

  // Enable move semantics
  LeftistHeap(LeftistHeap &&other) noexcept
      : root_(other.root_), size_(other.size_) {
    other.root_ = nullptr;
    other.size_ = 0;
  }

  LeftistHeap &operator=(LeftistHeap &&other) noexcept {
    if (this != &other) {
      clearTree(root_);
      root_ = other.root_;
      size_ = other.size_;
      other.root_ = nullptr;
      other.size_ = 0;
    }
    return *this;
  }

  /**
   * @brief Inserts a key-value entry into the heap.
   * @section complexity Time Complexity: O(log n) worst-case.
   */
  void insert(const Key &key, const Value &value) {
    Node *singleNode = new Node(key, value);
    root_ = mergeNodes(root_, singleNode);
    size_++;
  }

  /**
   * @brief Returns the minimum entry without removing it.
   * @section complexity Time Complexity: O(1) worst-case.
   */
  [[nodiscard]] const Entry &min() const {
    if (!root_) {
      throw std::runtime_error("Heap is empty.");
    }
    return root_->entry;
  }

  /**
   * @brief Removes the minimum entry from the heap.
   * @section complexity Time Complexity: O(log n) worst-case.
   */
  void removeMin() {
    if (!root_) {
      throw std::runtime_error("Heap is empty.");
    }
    Node *oldRoot = root_;
    root_ = mergeNodes(root_->left, root_->right);
    delete oldRoot;
    size_--;
  }

  /**
   * @brief Performs a union with another leftist heap h, destroying h and the
   * current heap's old state.
   * @section complexity Time Complexity: O(log n + log m) worst-case.
   */
  void unionWith(LeftistHeap &&h) {
    root_ = mergeNodes(root_, h.root_);
    size_ += h.size_;

    // Destroy old version of h
    h.root_ = nullptr;
    h.size_ = 0;
  }

  /** @brief Returns the number of entries in the heap. */
  [[nodiscard]] size_t size() const { return size_; }

  /** @brief Checks if the heap is empty. */
  [[nodiscard]] bool empty() const { return size_ == 0; }
};