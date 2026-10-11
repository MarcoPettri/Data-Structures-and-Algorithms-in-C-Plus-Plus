// File: CircularlyLinkedList.tpp
// IMPLEMENTATION — do not compile on its own; included by
// CircularlyLinkedList.hpp.

#ifndef CIRCULARLY_LINKED_LIST_TPP
#define CIRCULARLY_LINKED_LIST_TPP

#include <stdexcept> // std::out_of_range
#include <utility>   // std::swap, std::move

// =====================================================================
//                           IMPLEMENTATION
// =====================================================================

// ---- construction / destruction / assignment ----

// The constructors below delegate to the default constructor first. Once
// that has finished, the object counts as constructed, so if push_back
// throws, the destructor runs and already-allocated nodes are freed.

template <typename T>
CircularlyLinkedList<T>::CircularlyLinkedList(std::initializer_list<T> init)
    : CircularlyLinkedList() {
  for (const T &v : init)
    push_back(v);
}

template <typename T>
CircularlyLinkedList<T>::CircularlyLinkedList(const CircularlyLinkedList &other)
    : CircularlyLinkedList() {
  if (other.empty())
    return;
  // Traverse the circle: start at head (tail_->next), stop after size_ nodes.
  Node *cur = other.tail_->next;
  for (size_type i = 0; i < other.size_; ++i, cur = cur->next)
    push_back(cur->data);
}

template <typename T>
CircularlyLinkedList<T>::CircularlyLinkedList(
    CircularlyLinkedList &&other) noexcept
    : tail_(other.tail_), size_(other.size_) {
  other.tail_ = nullptr;
  other.size_ = 0;
}

template <typename T>
CircularlyLinkedList<T> &
CircularlyLinkedList<T>::operator=(CircularlyLinkedList other) noexcept {
  swap(other);
  return *this;
}

template <typename T> CircularlyLinkedList<T>::~CircularlyLinkedList() {
  clear();
}

template <typename T>
void CircularlyLinkedList<T>::swap(CircularlyLinkedList &other) noexcept {
  std::swap(tail_, other.tail_);
  std::swap(size_, other.size_);
}

// ---- element access ----

template <typename T> T &CircularlyLinkedList<T>::front() {
  if (empty())
    throw std::out_of_range("front() on empty list");
  return tail_->next->data; // tail_->next is the head
}

template <typename T> const T &CircularlyLinkedList<T>::front() const {
  if (empty())
    throw std::out_of_range("front() on empty list");
  return tail_->next->data;
}

template <typename T> T &CircularlyLinkedList<T>::back() {
  if (empty())
    throw std::out_of_range("back() on empty list");
  return tail_->data;
}

template <typename T> const T &CircularlyLinkedList<T>::back() const {
  if (empty())
    throw std::out_of_range("back() on empty list");
  return tail_->data;
}

// ---- modifiers ----

// push_front: insert a new node between tail_ and the current head.
// If `new` or T's constructor throws, the list is left unchanged.
template <typename T> void CircularlyLinkedList<T>::push_front(const T &v) {
  Node *node = new Node(v);
  if (empty()) {
    node->next = node; // points to itself
    tail_ = node;
  } else {
    node->next = tail_->next; // new node points to old head
    tail_->next = node;       // tail points to new head
  }
  ++size_;
}

template <typename T> void CircularlyLinkedList<T>::push_front(T &&v) {
  Node *node = new Node(std::move(v));
  if (empty()) {
    node->next = node;
    tail_ = node;
  } else {
    node->next = tail_->next;
    tail_->next = node;
  }
  ++size_;
}

// push_back: insert at the front, then advance tail_ to that node.
template <typename T> void CircularlyLinkedList<T>::push_back(const T &v) {
  push_front(v);
  tail_ = tail_->next;
}

template <typename T> void CircularlyLinkedList<T>::push_back(T &&v) {
  push_front(std::move(v));
  tail_ = tail_->next;
}

// pop_front: remove the head node (tail_->next).
template <typename T> void CircularlyLinkedList<T>::pop_front() {
  if (empty())
    throw std::out_of_range("pop_front() on empty list");
  Node *head = tail_->next;
  if (size_ == 1) {
    tail_ = nullptr;
  } else {
    tail_->next = head->next; // bypass the head
  }
  delete head;
  --size_;
}

// rotate: old front becomes new back.
// The guard is required: with an empty list tail_ is nullptr, and with a
// single element rotating would change nothing.
template <typename T> void CircularlyLinkedList<T>::rotate() noexcept {
  if (size_ > 1)
    tail_ = tail_->next;
}

// Iterative clear: frees every node without recursion.
template <typename T> void CircularlyLinkedList<T>::clear() noexcept {
  if (empty())
    return;
  Node *cur = tail_->next; // head
  tail_->next = nullptr;   // break the circle to allow linear traversal
  while (cur) {
    Node *nxt = cur->next;
    delete cur;
    cur = nxt;
  }
  tail_ = nullptr;
  size_ = 0;
}

// ---- non-member functions ----

template <typename T>
void swap(CircularlyLinkedList<T> &a, CircularlyLinkedList<T> &b) noexcept {
  a.swap(b);
}

// Equality: same size and same elements in the same order, read from the
// head of each list (so {1,2,3} != {2,3,1}). O(n), no allocations.
template <typename T>
bool operator==(const CircularlyLinkedList<T> &a,
                const CircularlyLinkedList<T> &b) {
  if (a.size_ != b.size_)
    return false;
  if (a.size_ == 0)
    return true;

  using Node = typename CircularlyLinkedList<T>::Node;
  Node *pa = a.tail_->next; // head of a
  Node *pb = b.tail_->next; // head of b
  for (typename CircularlyLinkedList<T>::size_type i = 0; i < a.size_;
       ++i, pa = pa->next, pb = pb->next) {
    if (!(pa->data == pb->data))
      return false; // only requires T::operator==
  }
  return true;
}

template <typename T>
bool operator!=(const CircularlyLinkedList<T> &a,
                const CircularlyLinkedList<T> &b) {
  return !(a == b);
}

#endif // CIRCULARLY_LINKED_LIST_TPP