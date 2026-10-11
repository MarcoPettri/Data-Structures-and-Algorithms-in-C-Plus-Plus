// File: CircularlyLinkedList.hpp
// R-3.15: Fully generic circularly linked list (C++17 template class).
// Interface only — implementation lives in CircularlyLinkedList.tpp.

#ifndef CIRCULARLY_LINKED_LIST_HPP
#define CIRCULARLY_LINKED_LIST_HPP

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <utility>

// =====================================================================
//                              INTERFACE
// =====================================================================

// A singly-linked, circularly linked list holding a raw pointer to its
// tail node. tail_->next always points back to the head (front).
// Operations at the front or back (except pop_back) run in O(1).
//
// Iterators: forward iterators that walk from the front to the back once
// (they do NOT loop around forever). Any structural modification of the
// list (push_*, pop_front, rotate, clear) invalidates them.

// Forward declarations (needed so the friend declaration below can name
// the matching specialisation).
template <typename T> class CircularlyLinkedList;

template <typename T>
bool operator==(const CircularlyLinkedList<T> &a,
                const CircularlyLinkedList<T> &b);

template <typename T> class CircularlyLinkedList {
private:
  struct Node {
    T data;
    Node *next = nullptr;

    template <typename... Args>
    explicit Node(Args &&...args) : data(std::forward<Args>(args)...) {}
  };

public:
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = T &;
  using const_reference = const T &;

  // ---- iterators ----
  template <bool IsConst> class basic_iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = std::conditional_t<IsConst, const T *, T *>;
    using reference = std::conditional_t<IsConst, const T &, T &>;

    basic_iterator() = default;

    // iterator -> const_iterator conversion (not the other way round).
    template <bool OtherConst,
              typename = std::enable_if_t<IsConst && !OtherConst>>
    basic_iterator(const basic_iterator<OtherConst> &other) noexcept
        : node_(other.node_), list_(other.list_) {}

    reference operator*() const { return node_->data; }
    pointer operator->() const { return &node_->data; }

    basic_iterator &operator++() {
      // Leaving the tail means we have visited every element once.
      node_ = (node_ == list_->tail_) ? nullptr : node_->next;
      return *this;
    }
    basic_iterator operator++(int) {
      basic_iterator tmp = *this;
      ++*this;
      return tmp;
    }

    friend bool operator==(const basic_iterator &a, const basic_iterator &b) {
      return a.node_ == b.node_;
    }
    friend bool operator!=(const basic_iterator &a, const basic_iterator &b) {
      return a.node_ != b.node_;
    }

  private:
    friend class CircularlyLinkedList;
    template <bool> friend class basic_iterator;

    basic_iterator(Node *node, const CircularlyLinkedList *list) noexcept
        : node_(node), list_(list) {}

    Node *node_ = nullptr; // nullptr == end()
    const CircularlyLinkedList *list_ = nullptr;
  };

  using iterator = basic_iterator<false>;
  using const_iterator = basic_iterator<true>;

  // ---- construction / destruction / assignment ----
  CircularlyLinkedList() = default;
  CircularlyLinkedList(std::initializer_list<T> init);
  CircularlyLinkedList(const CircularlyLinkedList &other);
  CircularlyLinkedList(CircularlyLinkedList &&other) noexcept;
  CircularlyLinkedList &
  operator=(CircularlyLinkedList other) noexcept; // copy-and-swap
  ~CircularlyLinkedList();

  void swap(CircularlyLinkedList &other) noexcept;

  // ---- capacity ----
  bool empty() const noexcept { return size_ == 0; }
  size_type size() const noexcept { return size_; }

  // ---- element access (throw std::out_of_range if empty) ----
  T &front();
  const T &front() const;
  T &back();
  const T &back() const;

  // ---- iteration: front to back, once ----
  iterator begin() noexcept {
    return empty() ? iterator() : iterator(tail_->next, this);
  }
  iterator end() noexcept { return iterator(nullptr, this); }

  const_iterator begin() const noexcept {
    return empty() ? const_iterator() : const_iterator(tail_->next, this);
  }
  const_iterator end() const noexcept { return const_iterator(nullptr, this); }

  const_iterator cbegin() const noexcept { return begin(); }
  const_iterator cend() const noexcept { return end(); }

  // ---- modifiers ----
  void push_front(const T &v);
  void push_front(T &&v);
  void push_back(const T &v);
  void push_back(T &&v);

  void pop_front(); // throws std::out_of_range if empty

  // Rotate forward: the current front becomes the new back. O(1).
  // No-op on lists with fewer than two elements.
  void rotate() noexcept;

  void clear() noexcept;

private:
  Node *tail_ = nullptr; // tail_->next == head (front); null when empty
  size_type size_ = 0;

  // Gives operator== access to Node / tail_ / size_.
  friend bool operator==
      <T>(const CircularlyLinkedList<T> &, const CircularlyLinkedList<T> &);
};

// ---- non-member functions ----
template <typename T>
bool operator!=(const CircularlyLinkedList<T> &a,
                const CircularlyLinkedList<T> &b);

template <typename T>
void swap(CircularlyLinkedList<T> &a, CircularlyLinkedList<T> &b) noexcept;

#include "CircularlyLinkedList.tpp"

#endif // CIRCULARLY_LINKED_LIST_HPP