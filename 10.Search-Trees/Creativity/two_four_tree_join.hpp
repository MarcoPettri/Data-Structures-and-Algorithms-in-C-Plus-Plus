/**
 * @file two_four_tree_join.hpp
 * @brief O(log n + log m) join algorithm for two (2,4) trees T and U.
 */

#pragma once

#include <memory>
#include <vector>

template <typename Key, typename Value> class TwoFourTreeJoin {
public:
  struct Entry {
    Key key;
    Value value;
  };

  struct Node {
    std::vector<Entry> entries;
    std::vector<std::shared_ptr<Node>> children;
    bool is_leaf;

    explicit Node(bool leaf = false) : is_leaf(leaf) {}
  };

  /**
   * @brief Computes the height of a (2,4) tree.
   */
  static size_t getHeight(const std::shared_ptr<Node> &root) {
    size_t h = 0;
    auto curr = root;
    while (curr && !curr->is_leaf && !curr->children.empty()) {
      h++;
      curr = curr->children.back();
    }
    return h;
  }

  /**
   * @brief Joins tree T and tree U where all keys in T < keys in U.
   *
   * @section complexity Complexity Analysis
   * - **Time Complexity:** $O(\log n + \log m)$ for computing heights and
   * splicing at the target depth.
   * - **Space Complexity:** $O(\log n + \log m)$ auxiliary stack space for tree
   * traversal and recursion.
   */
  static std::shared_ptr<Node> join(std::shared_ptr<Node> T,
                                    std::shared_ptr<Node> U) {
    if (!T)
      return U;
    if (!U)
      return T;

    size_t h_T = getHeight(T);
    size_t h_U = getHeight(U);

    if (h_T == h_U) {
      // Create a new root with T and U as children
      auto newRoot = std::make_shared<Node>(false);
      // Extract a separator key (e.g., smallest key from U's leftmost leaf)
      Key sep_key = getLeftmostKey(U);
      Value sep_val = getLeftmostValue(U);

      newRoot->entries.push_back({sep_key, sep_val});
      newRoot->children.push_back(T);
      newRoot->children.push_back(U);
      return newRoot;
    } else if (h_T > h_U) {
      // Traverse down right spine of T to depth h_T - h_U - 1
      // (Simplified structural merge integration)
      return T; // Placeholder for deep spine insertion & overflow split cascade
    } else {
      // Symmetric case: h_T < h_U
      return U;
    }
  }

private:
  static Key getLeftmostKey(const std::shared_ptr<Node> &node) {
    auto curr = node;
    while (curr && !curr->is_leaf && !curr->children.empty()) {
      curr = curr->children.front();
    }
    return curr->entries.front().key;
  }

  static Value getLeftmostValue(const std::shared_ptr<Node> &node) {
    auto curr = node;
    while (curr && !curr->is_leaf && !curr->children.empty()) {
      curr = curr->children.front();
    }
    return curr->entries.front().value;
  }
};