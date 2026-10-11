// File: DoublyLinkedList

#ifndef DOUBLY_LINKED_LIST_HPP
#define DOUBLY_LINKED_LIST_HPP

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
// =====================================================================
//                              INTERFACE
// =====================================================================

template <typename T> class DoublyLinkedList {

  template <bool IsConst> class BasicIterator;

public:
  using value_type = T;
  using size_type = std::size_t;
  using iterator = BasicIterator<false>;
  using const_iterator = BasicIterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  // ---- construction / destruction / assignment ----
  DoublyLinkedList() = default;
  DoublyLinkedList(std::initializer_list<T> init);
  DoublyLinkedList(const DoublyLinkedList &other);
  DoublyLinkedList(DoublyLinkedList &&other) noexcept;
  DoublyLinkedList &operator=(DoublyLinkedList other) noexcept; // copy-and-swap
  ~DoublyLinkedList();

  void swap(DoublyLinkedList &other) noexcept;

  // ---- capacity ----
  bool empty() const noexcept { return size_ == 0; }
  size_type size() const noexcept { return size_; }

  // ---- element access ----
  T &front();
  const T &front() const;
  T &back();
  const T &back() const;

  // ---- modifiers ----
  template <typename... Args> T &emplace_back(Args &&...args);
  template <typename... Args> T &emplace_front(Args &&...args);

  void push_back(const T &v) { emplace_back(v); }
  void push_back(T &&v) { emplace_back(std::move(v)); }
  void push_front(const T &v) { emplace_front(v); }
  void push_front(T &&v) { emplace_front(std::move(v)); }

  void pop_front();
  void pop_back();

  // Inserts a new element BEFORE pos; returns an iterator to it.
  template <typename... Args>
  iterator emplace(const_iterator pos, Args &&...args);

  iterator insert(const_iterator pos, const T &v) { return emplace(pos, v); }
  iterator insert(const_iterator pos, T &&v) {
    return emplace(pos, std::move(v));
  }

  // Removes the element at pos; returns an iterator to the following element.
  iterator erase(const_iterator pos);

  void clear() noexcept;
  void reverse() noexcept;

  // ---- iterators ----
  iterator begin() noexcept { return iterator(head_.get(), this); }
  iterator end() noexcept { return iterator(nullptr, this); }

  const_iterator begin() const noexcept {
    return const_iterator(head_.get(), this);
  }
  const_iterator end() const noexcept { return const_iterator(nullptr, this); }
  const_iterator cbegin() const noexcept { return begin(); }
  const_iterator cend() const noexcept { return end(); }

  reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }
  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }

private:
  struct Node {
    T data;
    std::unique_ptr<Node> next; // owning link
    Node *prev;                 // non-owning back link

    template <typename... Args>
    explicit Node(Args &&...args)
        : data(std::forward<Args>(args)...), next(nullptr), prev(nullptr) {}
  };

  std::unique_ptr<Node> head_;
  Node *tail_ = nullptr;
  std::size_t size_ = 0;
  // One iterator template serves both iterator and const_iterator.
  template <bool IsConst> class BasicIterator {
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = std::conditional_t<IsConst, const T *, T *>;
    using reference = std::conditional_t<IsConst, const T &, T &>;

    BasicIterator() = default;

    // Allow implicit conversion iterator -> const_iterator
    template <bool C = IsConst, typename = std::enable_if_t<C>>
    BasicIterator(const BasicIterator<false> &other)
        : node_(other.node_), list_(other.list_) {}

    reference operator*() const { return node_->data; }
    pointer operator->() const { return &node_->data; }

    BasicIterator &operator++() {
      node_ = node_->next.get();
      return *this;
    }
    BasicIterator operator++(int) {
      auto t = *this;
      ++*this;
      return t;
    }

    // Decrementing end() (node_ == nullptr) yields the last element.
    BasicIterator &operator--() {
      node_ = node_ ? node_->prev : list_->tail_;
      return *this;
    }
    BasicIterator operator--(int) {
      auto t = *this;
      --*this;
      return t;
    }

    friend bool operator==(const BasicIterator &a, const BasicIterator &b) {
      return a.node_ == b.node_;
    }
    friend bool operator!=(const BasicIterator &a, const BasicIterator &b) {
      return a.node_ != b.node_;
    }

  private:
    friend class DoublyLinkedList;
    friend class BasicIterator<!IsConst>;

    BasicIterator(Node *n, const DoublyLinkedList *l) : node_(n), list_(l) {}

    Node *node_ = nullptr;
    const DoublyLinkedList *list_ = nullptr;
  };
};

// ---- non-member functions ----
template <typename T>
void swap(DoublyLinkedList<T> &a, DoublyLinkedList<T> &b) noexcept;

template <typename T>
bool operator==(const DoublyLinkedList<T> &a, const DoublyLinkedList<T> &b);

template <typename T>
bool operator!=(const DoublyLinkedList<T> &a, const DoublyLinkedList<T> &b);

#include "DoublyLinkedList.tpp"

#endif // DOUBLY_LINKED_LIST_HPP