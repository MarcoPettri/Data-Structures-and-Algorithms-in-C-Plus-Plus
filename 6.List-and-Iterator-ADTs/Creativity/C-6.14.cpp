// File : C-6.14.cpp

/*
    C-6.14 Design a circular node list ADT that abstracts a circularly linked
   list in the same way that the node list ADT abstracts a doubly linked list
*/
#include <cstddef>
#include <stdexcept>
#include <utility>

/**
 * @brief Circular node list ADT, implemented as a singly circularly linked
 * list.
 * @tparam T Element type.
 *
 * Positions stay valid until the element they refer to is removed.
 * prev(), insertBefore() and remove() are O(n); everything else is O(1).
 */
template <typename T> class CircularNodeList {
  struct Node {
    T elem;
    Node *next;
  };

public:
  /// @brief Handle to a node. Valid until that element is removed.
  class Position {
  public:
    Position() = default;
    T &element() const { return node_->elem; }
    bool operator==(const Position &o) const { return node_ == o.node_; }
    bool operator!=(const Position &o) const { return node_ != o.node_; }
    bool is_null() const { return node_ == nullptr; }

  private:
    friend class CircularNodeList;
    explicit Position(Node *n) : node_(n) {}
    Node *node_ = nullptr;
  };

  CircularNodeList() = default;
  CircularNodeList(const CircularNodeList &) = delete;
  CircularNodeList &operator=(const CircularNodeList &) = delete;
  ~CircularNodeList() {
    while (!empty())
      remove(cursor());
  }

  // ---- accessors ----
  std::size_t size() const noexcept { return size_; }
  bool empty() const noexcept { return size_ == 0; }

  /// @throws std::logic_error if the list is empty.
  Position cursor() const {
    require_nonempty();
    return Position(cur_);
  }
  Position next(Position p) const { return Position(check(p)->next); }

  /// O(n): walks the ring to find the predecessor.
  Position prev(Position p) const { return Position(pred(check(p))); }

  // ---- cursor ----
  void advance() {
    require_nonempty();
    cur_ = cur_->next;
  }
  void setCursor(Position p) { cur_ = check(p); }

  // ---- updates ----
  /// @pre empty()
  Position insertFirstElement(T e) {
    if (!empty())
      throw std::logic_error("list is not empty");
    cur_ = new Node{std::move(e), nullptr};
    cur_->next = cur_;
    size_ = 1;
    return Position(cur_);
  }

  Position insertAfter(Position p, T e) {
    Node *n = check(p);
    Node *fresh = new Node{std::move(e), n->next};
    n->next = fresh;
    ++size_;
    return Position(fresh);
  }

  /// O(n)
  Position insertBefore(Position p, T e) {
    return insertAfter(Position(pred(check(p))), std::move(e));
  }

  /// O(n). If @p p was the cursor, the cursor moves to its successor.
  T remove(Position p) {
    Node *n = check(p);
    T result = std::move(n->elem);
    if (size_ == 1) {
      cur_ = nullptr;
    } else {
      pred(n)->next = n->next;
      if (cur_ == n)
        cur_ = n->next;
    }
    delete n;
    --size_;
    return result;
  }

  T replaceElement(Position p, T e) {
    Node *n = check(p);
    T old = std::move(n->elem);
    n->elem = std::move(e);
    return old;
  }

  void swapElements(Position p, Position q) {
    using std::swap;
    swap(check(p)->elem, check(q)->elem);
  }

private:
  Node *check(Position p) const {
    if (p.is_null())
      throw std::invalid_argument("invalid position");
    return p.node_;
  }
  void require_nonempty() const {
    if (empty())
      throw std::logic_error("list is empty");
  }
  Node *pred(Node *n) const {
    Node *q = n;
    while (q->next != n)
      q = q->next;
    return q;
  }

  Node *cur_ = nullptr;
  std::size_t size_ = 0;
};

int main() {
  CircularNodeList<int> ring;
  auto p = ring.insertFirstElement(1);
  for (int i = 2; i <= 7; ++i)
    p = ring.insertAfter(p, i); // ring: 1..7, cursor at 1

  const int k = 3;
  while (ring.size() > 1) {
    for (int i = 1; i < k; ++i)
      ring.advance();           // count off k people
    ring.remove(ring.cursor()); // cursor moves to successor
  }
  // ring.cursor().element() is the survivor
}