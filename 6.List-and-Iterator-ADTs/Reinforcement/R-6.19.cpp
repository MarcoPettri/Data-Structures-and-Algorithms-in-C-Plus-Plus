// File : R-6.19.cpp

/*
    R-6.19 Briefly describe how to perform a new sequence function makeFirst(p)
    that moves an element of a sequence S at position p to be the first element
    in S while keeping the relative ordering of the remaining elements in S
    unchanged. Your function should run in O(1) time if S is implemented
    with a doubly linked list.
*/

/**
 * @brief Moves an element at the given iterator position to the front of the
 * list.
 * @details Runs in O(1) time by adjusting adjacent node pointers.
 * @param pos Iterator pointing to the element to move.
 */
void makeFirst(iterator pos) {
  Node<T> *node = pos.node(); // Access the underlying node via iterator

  // Edge case: If the list is empty, or the node is already the head, do
  // nothing
  if (!node || node == head_) {
    return;
  }

  Node<T> *prev_node = node->prev;
  Node<T> *next_node = node->next;

  // 1. Unlink the node from its current position
  if (prev_node) {
    prev_node->next = next_node;
  }

  if (next_node) {
    next_node->prev = prev_node;
  } else {
    // If 'node' was the tail, update the tail pointer to the previous node
    tail_ = prev_node;
  }

  // 2. Move 'node' to the front (before the current head)
  node->next = head_;
  node->prev = nullptr;

  if (head_) {
    head_->prev = node;
  }

  head_ = node;
}