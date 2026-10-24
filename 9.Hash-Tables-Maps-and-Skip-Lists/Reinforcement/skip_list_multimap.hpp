/**
 * @file skip_list_multimap.hpp
 * @brief Skip-List based Dictionary ADT (Multimap) supporting duplicate keys in
 * C++17.
 */

#pragma once

#include <cstddef>
#include <random>
#include <vector>

/**
 * @class SkipListMultimap
 * @brief Implements a dictionary that allows multiple entries with equal keys
 * using a skip-list.
 *
 * @tparam Key The key type.
 * @tparam Value The value type.
 */
template <typename Key, typename Value> class SkipListMultimap {
public:
  struct Entry {
    Key key;
    Value value;
  };

private:
  struct Node {
    Entry entry;
    std::vector<Node *> forward;

    Node(Key k, Value v, int level) : entry{k, v}, forward(level, nullptr) {}
  };

  size_t max_level_;
  double p_;
  size_t size_;
  Node *head_;
  mutable std::mt19937 rng_;
  mutable std::uniform_real_distribution<double> dist_;

  /**
   * @brief Generates a random height level for a new node.
   */
  int randomLevel() const {
    int lvl = 1;
    while (dist_(rng_) < p_ && lvl < static_cast<int>(max_level_)) {
      lvl++;
    }
    return lvl;
  }

public:
  /**
   * @brief Construct a new Skip List Multimap object.
   * @param max_level Maximum height of node towers.
   * @param p Probability factor for coin-tossing level generation.
   */
  explicit SkipListMultimap(size_t max_level = 32, double p = 0.5)
      : max_level_(max_level), p_(p), size_(0), rng_(1337), dist_(0.0, 1.0) {
    head_ = new Node(Key{}, Value{}, max_level_);
  }

  /**
   * @brief Destroy the Skip List Multimap object and free all nodes.
   */
  ~SkipListMultimap() {
    Node *current = head_;
    while (current != nullptr) {
      Node *next = current->forward[0];
      delete current;
      current = next;
    }
  }

  // Disable copy constructors for simplicity of raw pointer management
  SkipListMultimap(const SkipListMultimap &) = delete;
  SkipListMultimap &operator=(const SkipListMultimap &) = delete;

  /**
   * @brief Inserts a key-value entry into the multimap (allows duplicate keys).
   *
   * @param key The key.
   * @param value The value.
   *
   * @section complexity Complexity Analysis
   * - **Time Complexity:** $O(\log n)$ expected time.
   * - **Space Complexity:** $O(1)$ auxiliary space per insertion (excluding
   * node allocation).
   */
  void insert(const Key &key, const Value &value) {
    std::vector<Node *> update(max_level_, nullptr);
    Node *curr = head_;

    // Search phase: move right only if curr->forward[i]->key < key (strict
    // inequality for duplicates)
    for (int i = static_cast<int>(max_level_) - 1; i >= 0; --i) {
      while (curr->forward[i] != nullptr && curr->forward[i]->entry.key < key) {
        curr = curr->forward[i];
      }
      update[i] = curr;
    }

    int new_level = randomLevel();
    Node *newNode = new Node(key, value, new_level);

    // Splice new node into skip-list levels
    for (int i = 0; i < new_level; ++i) {
      newNode->forward[i] = update[i]->forward[i];
      update[i]->forward[i] = newNode;
    }

    size_++;
  }

  /**
   * @brief Finds the first occurrence of an entry with the given key.
   *
   * @param key The target key.
   * @param out_value Reference to store the found value.
5      * @return true if found, false otherwise.
   */
  [[nodiscard]] bool find(const Key &key, Value &out_value) const {
    Node *curr = head_;
    for (int i = static_cast<int>(max_level_) - 1; i >= 0; --i) {
      while (curr->forward[i] != nullptr && curr->forward[i]->entry.key < key) {
        curr = curr->forward[i];
      }
    }
    curr = curr->forward[0];
    if (curr != nullptr && curr->entry.key == key) {
      out_value = curr->entry.value;
      return true;
    }
    return false;
  }

  /**
   * @brief Finds all values associated with a given key.
   *
   * @param key The target key.
   * @return std::vector<Value> Collection of all matching values.
   *
   * @section complexity Complexity Analysis
   * - **Time Complexity:** $O(\log n + c)$ expected time, where $c$ is the
   * number of duplicates.
   */
  [[nodiscard]] std::vector<Value> findAll(const Key &key) const {
    std::vector<Value> results;
    Node *curr = head_;
    for (int i = static_cast<int>(max_level_) - 1; i >= 0; --i) {
      while (curr->forward[i] != nullptr && curr->forward[i]->entry.key < key) {
        curr = curr->forward[i];
      }
    }
    curr = curr->forward[0];
    // Linear scan across base level for duplicate keys
    while (curr != nullptr && curr->entry.key == key) {
      results.push_back(curr->entry.value);
      curr = curr->forward[0];
    }
    return results;
  }

  /**
   * @brief Removes all entries matching the given key.
   *
   * @param key The target key.
   * @return size_t The number of entries removed.
   */
  size_t remove(const Key &key) {
    std::vector<Node *> update(max_level_, nullptr);
    Node *curr = head_;

    for (int i = static_cast<int>(max_level_) - 1; i >= 0; --i) {
      while (curr->forward[i] != nullptr && curr->forward[i]->entry.key < key) {
        curr = curr->forward[i];
      }
      update[i] = curr;
    }

    curr = curr->forward[0];
    size_t removed_count = 0;

    // Unlink and delete all consecutive nodes matching the key
    while (curr != nullptr && curr->entry.key == key) {
      Node *next_node = curr->forward[0];
      for (size_t i = 0; i < max_level_; ++i) {
        if (update[i]->forward[i] == curr) {
          update[i]->forward[i] = curr->forward[i];
        }
      }
      delete curr;
      size_--;
      removed_count++;
      curr = next_node;
    }
    return removed_count;
  }

  /** @brief Returns the number of entries in the multimap. */
  [[nodiscard]] size_t size() const { return size_; }

  /** @brief Checks if the multimap is empty. */
  [[nodiscard]] bool empty() const { return size_ == 0; }
};