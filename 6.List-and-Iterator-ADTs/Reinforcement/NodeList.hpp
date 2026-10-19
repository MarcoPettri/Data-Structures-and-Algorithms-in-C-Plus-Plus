
/**
 * @file NodeList.hpp
 * @brief Defines a generic doubly-linked list container in C++17.
 */
#pragma once

#include "Node.hpp"
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace datastructures {

/**
 * @brief A generic doubly-linked list container.
 * @tparam T The element type stored in the list.
 */
template <typename T> class NodeList {
public:
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = T &;
  using const_reference = const T &;
  using pointer = T *;
  using const_pointer = const T *;

  // Forward declarations for iterators
  class Iterator;
  class ConstIterator;

  using iterator = Iterator;
  using const_iterator = ConstIterator;

  /**
   * @brief Default constructor. Creates an empty list.
   */
  NodeList() noexcept : head_(nullptr), tail_(nullptr), size_(0) {}

  /**
   * @brief Destructor. Clears all nodes.
   */
  ~NodeList() { clear(); }

  // Copy Constructor
  NodeList(const NodeList &other) {
    for (const auto &val : other) {
      push_back(val);
    }
  }

  // Copy Assignment Operator
  NodeList &operator=(const NodeList &other) {
    if (this != &other) {
      NodeList temp(other);
      swap(temp);
    }
    return *this;
  }

  // Move Constructor
  NodeList(NodeList &&other) noexcept
      : head_(other.head_), tail_(other.tail_), size_(other.size_) {
    other.head_ = nullptr;
    other.tail_ = nullptr;
    other.size_ = 0;
  }

  // Move Assignment Operator
  NodeList &operator=(NodeList &&other) noexcept {
    if (this != &other) {
      clear();
      head_ = other.head_;
      tail_ = other.tail_;
      size_ = other.size_;
      other.head_ = nullptr;
      other.tail_ = nullptr;
      other.size_ = 0;
    }
    return *this;
  }

  /**
   * @brief Checks if the list is empty.
   * @return true if empty, false otherwise.
   */
  [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

  /**
   * @brief Gets the number of elements in the list.
   * @return The size of the list.
   */
  [[nodiscard]] size_type size() const noexcept { return size_; }

  /**
   * @brief Clears all elements from the list.
   */
  void clear() noexcept {
    Node<T> *current = head_;
    while (current != nullptr) {
      Node<T> *next_node = current->next;
      delete current;
      current = next_node;
    }
    head_ = nullptr;
    tail_ = nullptr;
    size_ = 0;
  }

  /**
   * @brief Adds an element to the front of the list.
   * @param value The value to add.
   */
  void push_front(const T &value) {
    Node<T> *new_node = new Node<T>(value);
    if (empty()) {
      head_ = tail_ = new_node;
    } else {
      new_node->next = head_;
      head_->prev = new_node;
      head_ = new_node;
    }
    ++size_;
  }

  /**
   * @brief Adds an element to the back of the list.
   * @param value The value to add.
   */
  void push_back(const T &value) {
    Node<T> *new_node = new Node<T>(value);
    if (empty()) {
      head_ = tail_ = new_node;
    } else {
      tail_->next = new_node;
      new_node->prev = tail_;
      tail_ = new_node;
    }
    ++size_;
  }

  /**
   * @brief Removes the element at the front of the list.
   */
  void pop_front() {
    if (empty()) {
      throw std::out_of_range("pop_front() on empty NodeList");
    }
    Node<T> *node_to_delete = head_;
    head_ = head_->next;
    if (head_) {
      head_->prev = nullptr;
    } else {
      tail_ = nullptr;
    }
    delete node_to_delete;
    --size_;
  }

  /**
   * @brief Removes the element at the back of the list.
   */
  void pop_back() {
    if (empty()) {
      throw std::out_of_range("pop_back() on empty NodeList");
    }
    Node<T> *node_to_delete = tail_;
    tail_ = tail_->prev;
    if (tail_) {
      tail_->next = nullptr;
    } else {
      head_ = nullptr;
    }
    delete node_to_delete;
    --size_;
  }

  /**
   * @brief Access the first element.
   */
  [[nodiscard]] reference front() {
    if (empty())
      throw std::out_of_range("front() on empty NodeList");
    return head_->value;
  }

  [[nodiscard]] const_reference front() const {
    if (empty())
      throw std::out_of_range("front() on empty NodeList");
    return head_->value;
  }

  /**
   * @brief Access the last element.
   */
  [[nodiscard]] reference back() {
    if (empty())
      throw std::out_of_range("back() on empty NodeList");
    return tail_->value;
  }

  [[nodiscard]] const_reference back() const {
    if (empty())
      throw std::out_of_range("back() on empty NodeList");
    return tail_->value;
  }

  // --- Iterator Implementation ---
  class Iterator {
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T *;
    using reference = T &;

    explicit Iterator(Node<T> *ptr) noexcept : current_(ptr) {}

    reference operator*() const { return current_->value; }
    pointer operator->() const { return &(current_->value); }

    Iterator &operator++() noexcept {
      if (current_)
        current_ = current_->next;
      return *this;
    }

    Iterator operator++(int) noexcept {
      Iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    Iterator &operator--() noexcept {
      if (current_)
        current_ = current_->prev;
      return *this;
    }

    Iterator operator--(int) noexcept {
      Iterator tmp = *this;
      --(*this);
      return tmp;
    }

    friend bool operator==(const Iterator &a, const Iterator &b) noexcept {
      return a.current_ == b.current_;
    }

    friend bool operator!=(const Iterator &a, const Iterator &b) noexcept {
      return a.current_ != b.current_;
    }

  private:
    Node<T> *current_;
  };

  class ConstIterator {
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = const T *;
    using reference = const T &;

    explicit ConstIterator(const Node<T> *ptr) noexcept : current_(ptr) {}

    reference operator*() const { return current_->value; }
    pointer operator->() const { return &(current_->value); }

    ConstIterator &operator++() noexcept {
      if (current_)
        current_ = current_->next;
      return *this;
    }

    ConstIterator operator++(int) noexcept {
      ConstIterator tmp = *this;
      ++(*this);
      return tmp;
    }

    ConstIterator &operator--() noexcept {
      if (current_)
        current_ = current_->prev;
      return *this;
    }

    ConstIterator operator--(int) noexcept {
      ConstIterator tmp = *this;
      --(*this);
      return tmp;
    }

    friend bool operator==(const ConstIterator &a,
                           const ConstIterator &b) noexcept {
      return a.current_ == b.current_;
    }

    friend bool operator!=(const ConstIterator &a,
                           const ConstIterator &b) noexcept {
      return a.current_ != b.current_;
    }

  private:
    const Node<T> *current_;
  };

  // Iterator factory methods
  [[nodiscard]] iterator begin() noexcept { return iterator(head_); }
  [[nodiscard]] iterator end() noexcept { return iterator(nullptr); }
  [[nodiscard]] const_iterator begin() const noexcept {
    return const_iterator(head_);
  }
  [[nodiscard]] const_iterator end() const noexcept {
    return const_iterator(nullptr);
  }
  [[nodiscard]] const_iterator cbegin() const noexcept {
    return const_iterator(head_);
  }
  [[nodiscard]] const_iterator cend() const noexcept {
    return const_iterator(nullptr);
  }

  /**
   * @brief Swaps the contents of this list with another.
   * @param other The other list to swap with.
   */
  void swap(NodeList &other) noexcept {
    std::swap(head_, other.head_);
    std::swap(tail_, other.tail_);
    std::swap(size_, other.size_);
  }

private:
  Node<T> *head_;
  Node<T> *tail_;
  size_type size_;
};

} // namespace datastructures