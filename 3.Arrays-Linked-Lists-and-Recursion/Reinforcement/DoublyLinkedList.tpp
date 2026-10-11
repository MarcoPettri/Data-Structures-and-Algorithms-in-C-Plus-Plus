// File: DoublyLinkedList.cpp
// DoublyLinkedList.tpp : IMPLEMENTATION (definitions of everything declared in
// DoublyLinkedList.hpp) Do not compile this file on its own; it is included by
// DoublyLinkedList.hpp.
#ifndef DOUBLYLINKEDLIST_TPP
#define DOUBLYLINKEDLIST_TPP

// =====================================================================
//                           IMPLEMENTATION
// =====================================================================

// ---- construction / destruction / assignment ----

template <typename T>
DoublyLinkedList<T>::DoublyLinkedList(std::initializer_list<T> init) {
  for (const T &v : init)
    push_back(v);
}

template <typename T>
DoublyLinkedList<T>::DoublyLinkedList(const DoublyLinkedList &other) {
  for (const T &v : other)
    push_back(v);
}

template <typename T>
DoublyLinkedList<T>::DoublyLinkedList(DoublyLinkedList &&other) noexcept
    : head_(std::move(other.head_)), tail_(other.tail_), size_(other.size_) {
  other.tail_ = nullptr;
  other.size_ = 0;
}

template <typename T>
DoublyLinkedList<T> &
DoublyLinkedList<T>::operator=(DoublyLinkedList other) noexcept {
  swap(other);
  return *this;
}

template <typename T> DoublyLinkedList<T>::~DoublyLinkedList() { clear(); }

template <typename T>
void DoublyLinkedList<T>::swap(DoublyLinkedList &other) noexcept {
  head_.swap(other.head_);
  std::swap(tail_, other.tail_);
  std::swap(size_, other.size_);
}

// ---- element access ----

template <typename T> T &DoublyLinkedList<T>::front() {
  if (empty())
    throw std::out_of_range("front() on empty list");
  return head_->data;
}

template <typename T> const T &DoublyLinkedList<T>::front() const {
  if (empty())
    throw std::out_of_range("front() on empty list");
  return head_->data;
}

template <typename T> T &DoublyLinkedList<T>::back() {
  if (empty())
    throw std::out_of_range("back() on empty list");
  return tail_->data;
}

template <typename T> const T &DoublyLinkedList<T>::back() const {
  if (empty())
    throw std::out_of_range("back() on empty list");
  return tail_->data;
}

// ---- modifiers ----

template <typename T>
template <typename... Args>
T &DoublyLinkedList<T>::emplace_back(Args &&...args) {
  auto node = std::make_unique<Node>(std::forward<Args>(args)...);
  node->prev = tail_;
  Node *raw = node.get();
  if (tail_)
    tail_->next = std::move(node);
  else
    head_ = std::move(node);
  tail_ = raw;
  ++size_;
  return raw->data;
}

template <typename T>
template <typename... Args>
T &DoublyLinkedList<T>::emplace_front(Args &&...args) {
  auto node = std::make_unique<Node>(std::forward<Args>(args)...);
  if (head_)
    head_->prev = node.get();
  else
    tail_ = node.get();
  node->next = std::move(head_);
  head_ = std::move(node);
  ++size_;
  return head_->data;
}

template <typename T> void DoublyLinkedList<T>::pop_front() {
  if (empty())
    throw std::out_of_range("pop_front() on empty list");
  std::unique_ptr<Node> old = std::move(head_);
  head_ = std::move(old->next);
  if (head_)
    head_->prev = nullptr;
  else
    tail_ = nullptr;
  --size_;
}

template <typename T> void DoublyLinkedList<T>::pop_back() {
  if (empty())
    throw std::out_of_range("pop_back() on empty list");
  Node *before = tail_->prev;
  if (before) {
    before->next.reset();
    tail_ = before;
  } else {
    head_.reset();
    tail_ = nullptr;
  }
  --size_;
}

template <typename T>
template <typename... Args>
typename DoublyLinkedList<T>::iterator
DoublyLinkedList<T>::emplace(const_iterator pos, Args &&...args) {
  if (!pos.node_) { // pos == end()
    emplace_back(std::forward<Args>(args)...);
    return iterator(tail_, this);
  }
  if (pos.node_ == head_.get()) { // pos == begin()
    emplace_front(std::forward<Args>(args)...);
    return begin();
  }
  Node *cur = pos.node_;
  Node *before = cur->prev;
  auto node = std::make_unique<Node>(std::forward<Args>(args)...);
  Node *raw = node.get();
  node->prev = before;
  cur->prev = raw;
  node->next = std::move(before->next); // node now owns cur
  before->next = std::move(node);       // before owns node
  ++size_;
  return iterator(raw, this);
}

template <typename T>
typename DoublyLinkedList<T>::iterator
DoublyLinkedList<T>::erase(const_iterator pos) {
  if (!pos.node_)
    throw std::out_of_range("erase() at end()");
  Node *prv = pos.node_->prev;
  Node *nxt = pos.node_->next.get();

  std::unique_ptr<Node> &owner = prv ? prv->next : head_;
  std::unique_ptr<Node> victim = std::move(owner); // owns pos.node_
  owner = std::move(victim->next);                 // splice out

  if (nxt)
    nxt->prev = prv;
  else
    tail_ = prv;
  --size_;
  return iterator(nxt, this);
}

// Iterative clear: avoids deep recursive destructor calls on long lists.
template <typename T> void DoublyLinkedList<T>::clear() noexcept {
  while (head_)
    head_ = std::move(head_->next);
  tail_ = nullptr;
  size_ = 0;
}

// Reverses the list in place by relinking nodes (no data copies).
template <typename T> void DoublyLinkedList<T>::reverse() noexcept {
  std::unique_ptr<Node> cur = std::move(head_);
  Node *newTail = cur.get();
  std::unique_ptr<Node> prevOwned;
  while (cur) {
    std::unique_ptr<Node> nxt = std::move(cur->next);
    cur->next = std::move(prevOwned);
    if (cur->next)
      cur->next->prev = cur.get();
    cur->prev = nullptr;
    prevOwned = std::move(cur);
    cur = std::move(nxt);
  }
  head_ = std::move(prevOwned);
  tail_ = newTail;
}

// ---- non-member functions ----

template <typename T>
void swap(DoublyLinkedList<T> &a, DoublyLinkedList<T> &b) noexcept {
  a.swap(b);
}

template <typename T>
bool operator==(const DoublyLinkedList<T> &a, const DoublyLinkedList<T> &b) {
  return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

template <typename T>
bool operator!=(const DoublyLinkedList<T> &a, const DoublyLinkedList<T> &b) {
  return !(a == b);
}

#endif // DOUBLYLINKEDLIST_TPP