/**
 * @file distinct_hash_multimap.hpp
 * @brief Hash table dictionary for n entries with distinct key hash codes.
 */

#pragma once

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

/**
 * @class DistinctHashMultimap
 * @brief Implements a dictionary/multimap where distinct keys have distinct
 * hash codes, supporting findAll in O(1 + s) expected time and O(1) expected
 * time for other operations.
 *
 * @tparam Key Key type (must be hashable via std::hash).
 * @tparam Value Value type.
 */
template <typename Key, typename Value> class DistinctHashMultimap {
private:
  struct Node {
    Key key;
    Value value;
    Node *next;
  };

  struct Bucket {
    Node *head = nullptr;
  };

  std::vector<Bucket> table_;
  size_t size_;
  size_t distinct_keys_;
  size_t capacity_;

  /**
   * @brief Computes the bucket index for a given key.
   */
  size_t getBucketIndex(const Key &key) const {
    return std::hash<Key>{}(key) % capacity_;
  }

  /**
   * @brief Rehashes the table when load factor exceeds threshold.
   */
  void rehash(size_t new_capacity) {
    std::vector<Bucket> old_table = std::move(table_);
    capacity_ = new_capacity;
    table_.clear();
    table_.resize(capacity_);
    size_ = 0;
    distinct_keys_ = 0;

    for (auto &bucket : old_table) {
      Node *curr = bucket.head;
      while (curr != nullptr) {
        insertInternal(curr->key, curr->value);
        Node *next = curr->next;
        delete curr;
        curr = next;
      }
    }
  }

  void insertInternal(const Key &key, const Value &value) {
    size_t idx = getBucketIndex(key);
    Node *curr = table_[idx].head;

    bool key_exists = false;
    while (curr != nullptr) {
      if (curr->key == key) {
        key_exists = true;
        break;
      }
      curr = curr->next;
    }

    Node *newNode = new Node{key, value, table_[idx].head};
    table_[idx].head = newNode;
    size_++;

    if (!key_exists) {
      distinct_keys_++;
    }
  }

public:
  /**
   * @brief Construct a new Distinct Hash Multimap object.
   * @param initial_capacity Starting bucket capacity (default 16).
   */
  explicit DistinctHashMultimap(size_t initial_capacity = 16)
      : size_(0), distinct_keys_(0),
        capacity_(std::max(size_t(4), initial_capacity)) {
    table_.resize(capacity_);
  }

  /**
   * @brief Destructor. Frees all allocated nodes.
   */
  ~DistinctHashMultimap() { clear(); }

  // Disable copy semantics
  DistinctHashMultimap(const DistinctHashMultimap &) = delete;
  DistinctHashMultimap &operator=(const DistinctHashMultimap &) = delete;

  /**
   * @brief Clears all entries from the multimap.
   */
  void clear() {
    for (auto &bucket : table_) {
      Node *curr = bucket.head;
      while (curr != nullptr) {
        Node *next = curr->next;
        delete curr;
        curr = next;
      }
      bucket.head = nullptr;
    }
    size_ = 0;
    distinct_keys_ = 0;
  }

  /**
   * @brief Inserts a key-value entry.
   *
   * @param key The key.
   * @param value The value.
   *
   * @section complexity Complexity Analysis
   * - **Time Complexity:** $\mathcal{O}(1)$ expected time (amortized).
   */
  void insert(const Key &key, const Value &value) {
    if (static_cast<double>(distinct_keys_ + 1) / capacity_ > 0.75) {
      rehash(capacity_ * 2);
    }
    insertInternal(key, value);
  }

  /**
   * @brief Finds and returns all entries associated with the given key.
   *
   * @param key The target key.
   * @return std::vector<Value> Collection of all matching values ($s$ entries).
   *
   * @section complexity Complexity Analysis
   * - **Time Complexity:** $\mathcal{O}(1 + s)$ expected time, where $s$ is the
   * number of entries returned.
   */
  [[nodiscard]] std::vector<Value> findAll(const Key &key) const {
    std::vector<Value> result;
    size_t idx = getBucketIndex(key);
    Node *curr = table_[idx].head;

    while (curr != nullptr) {
      if (curr->key == key) {
        result.push_back(curr->value);
      }
      curr = curr->next;
    }
    return result;
  }

  /**
   * @brief Removes all entries matching the given key.
   *
   * @param key The key to remove.
   * @return size_t Number of entries removed.
   *
   * @section complexity Complexity Analysis
   * - **Time Complexity:** $\mathcal{O}(1)$ expected time.
   */
  size_t remove(const Key &key) {
    if (capacity_ == 0)
      return 0;

    size_t idx = getBucketIndex(key);
    Node *curr = table_[idx].head;
    Node *prev = nullptr;
    size_t removed_count = 0;

    while (curr != nullptr) {
      if (curr->key == key) {
        Node *to_delete = curr;
        if (prev == nullptr) {
          table_[idx].head = curr->next;
          curr = table_[idx].head;
        } else {
          prev->next = curr->next;
          curr = curr->next;
        }
        delete to_delete;
        size_--;
        removed_count++;
      } else {
        prev = curr;
        curr = curr->next;
      }
    }

    if (removed_count > 0) {
      distinct_keys_--;
    }
    return removed_count;
  }

  /** @brief Returns total number of entries $n$. */
  [[nodiscard]] size_t size() const { return size_; }

  /** @brief Returns number of distinct keys $r$. */
  [[nodiscard]] size_t distinctKeysCount() const { return distinct_keys_; }

  /** @brief Checks if the multimap is empty. */
  [[nodiscard]] bool empty() const { return size_ == 0; }
};