/**
 * @file Node.hpp
 * @brief Defines a generic doubly-linked Node structure.
 */
#pragma once

#include <type_traits>
#include <utility>

namespace datastructures {

/**
 * @brief A generic node template for doubly-linked lists.
 * @tparam T The type of data stored in the node.
 */
template <typename T> struct Node {
  T value;             ///< The stored data value
  Node *next{nullptr}; ///< Pointer to the next node
  Node *prev{nullptr}; ///< Pointer to the previous node

  /**
   * @brief Construct a new Node with a copy of the value.
   * @param val The value to initialize.
   */
  explicit Node(const T &val) : value(val) {}

  /**
   * @brief Construct a new Node by moving the value.
   * @param val The value to move.
   */
  explicit Node(T &&val) noexcept(std::is_nothrow_move_constructible_v<T>)
      : value(std::move(val)) {}
};

} // namespace datastructures