/**
 * @file OrderedListMap.hpp
 * @brief Implementation of the Map ADT using an ordered doubly linked list in
 * C++17.
 */

#pragma once

#include <cstddef>
#include <optional>
#include <optional>
#include <utility>
/**
 * @class OrderedListMap
 * @brief A Map ADT implemented using an ordered doubly linked list with
 * sentinel nodes.
 *
 * @tparam Key Key type (must support operator< and operator==).
 * @tparam Value Value type.
 */
template <typename Key, typename Value> class OrderedListMap {
public:
  using entry_type = std::pair<Key, Value>;

private:
  struct Node {
    entry_type entry;
    Node *prev = nullptr;
    Node *next = nullptr;

    Node() = default;
    Node(const Key &k, const Value &v, Node *p = nullptr, Node *n = nullptr)
        : entry(k, v), prev(p), next(n) {}
  };

  Node *header_;  ///< Dummy head sentinel
  Node *trailer_; ///< Dummy tail sentinel
  size_t size_;   ///< Number of key-value entries stored

public:
  /**
   * @brief Construct a new Ordered List Map object with header and trailer
   * sentinels.
   */
  OrderedListMap() : size_(0) {
    header_ = new Node();
    trailer_ = new Node();
    header_->next = trailer_;
    trailer_->prev = header_;
  }

  /**
   * @brief Destructor. Deallocates all list nodes including sentinels.
   */
  ~OrderedListMap() {
    clear();
    delete header_;
    delete trailer_;
  }

  // Disable copy constructor and copy assignment for raw pointer safety
  OrderedListMap(const OrderedListMap &) = delete;
  OrderedListMap &operator=(const OrderedListMap &) = delete;

  /**
   * @brief Returns the number of entries in the map.
   * @return size_t Number of entries.
   */
  [[nodiscard]] size_t size() const { return size_; }

  /**
   * @brief Checks if the map is empty.
   * @return true if empty, false otherwise.
   */
  [[nodiscard]] bool empty() const { return size_ == 0; }

  /**
   * @brief Clears all entries from the map.
   */
  void clear() {
    Node *curr = header_->next;
    while (curr != trailer_) {
      Node *nextNode = curr->next;
      delete curr;
      curr = nextNode;
    }
    header_->next = trailer_;
    trailer_->prev = header_;
    size_ = 0;
  }

  /**
   * @brief Searches for the value associated with key.
   *
   * @param key Target search key.
   * @return std::optional<Value> Value if found, std::nullopt otherwise.
   *
   * @section complexity Complexity Analysis
   * - **Time Complexity:** $O(n)$ worst case. Features early termination when a
   * node's key strictly exceeds the target key.
   * - **Space Complexity:** $O(1)$ auxiliary space.
   */
  [[nodiscard]] std::optional<Value> find(const Key &key) const {
    Node *curr = header_->next;
    while (curr != trailer_) {
      if (curr->entry.first == key) {
        return curr->entry.second;
      }
      // Early termination: since list is ordered, if current key > target, it
      // does not exist
      if (curr->entry.first > key) {
        break;
      }
      curr = curr->next;
    }
    return std::nullopt;
  }

  /**
   * @brief Inserts a key-value entry, or updates value if key exists.
   *
   * @param key Key to insert/update.
   * @param val Value associated with the key.
   *
   * @section complexity Complexity Analysis
   * - **Time Complexity:** $O(n)$ to locate position via linear scan, $O(1)$
   * pointer splice.
   * - **Space Complexity:** $O(1)$ auxiliary space.
   */
  void insert(const Key &key, const Value &val) {
    Node *curr = header_->next;

    // Linear scan to locate insertion point or existing entry
    while (curr != trailer_ && curr->entry.first < key) {
      curr = curr->next;
    }

    // Case 1: Key already exists, update value
    if (curr != trailer_ && curr->entry.first == key) {
      curr->entry.second = val;
      return;
    }

    // Case 2: Key does not exist, insert new node before curr
    Node *newNode = new Node(key, val, curr->prev, curr);
    curr->prev->next = newNode;
    curr->prev = newNode;
    size_++;
  }

  /**
   * @brief Removes the entry matching key.
   *
   * @param key Key to erase.
   * @return true if an entry was found and erased, false otherwise.
   *
   * @section complexity Complexity Analysis
   * - **Time Complexity:** $O(n)$ to locate node, $O(1)$ to update adjacent
   * node pointers.
   * - **Space Complexity:** $O(1)$ auxiliary space.
   */
  bool erase(const Key &key) {
    Node *curr = header_->next;
    while (curr != trailer_) {
      if (curr->entry.first == key) {
        // Unlink node
        curr->prev->next = curr->next;
        curr->next->prev = curr->prev;
        delete curr;
        size_--;
        return true;
      }
      if (curr->entry.first > key) {
        break; // Early termination
      }
      curr = curr->next;
    }
    return false;
  }

  /**
   * @brief Returns entry with the minimum key in O(1) time.
   * @return std::optional<entry_type> First entry if non-empty, std::nullopt
   * otherwise.
   */
  [[nodiscard]] std::optional<entry_type> min() const {
    if (empty())
      return std::nullopt;
    return header_->next->entry;
  }

  /**
   * @brief Returns entry with the maximum key in O(1) time.
   * @return std::optional<entry_type> Last entry if non-empty, std::nullopt
   * otherwise.
   */
  [[nodiscard]] std::optional<entry_type> max() const {
    if (empty())
      return std::nullopt;
    return trailer_->prev->entry;
  }
};