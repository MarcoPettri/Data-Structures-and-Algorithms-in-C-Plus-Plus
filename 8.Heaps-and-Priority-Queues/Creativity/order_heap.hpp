/**
 * @file order_heap.hpp
 * @brief Adaptable priority queue of orders (binary heap + id-to-position
 * index).
 *
 * A plain @c std::priority_queue can only look at and remove its top element.
 * Exercise C-8.2 needs to change the price of an arbitrary pending order, so
 * this heap additionally keeps a hash map from OrderId to the order's current
 * array position (the textbook "locator"). With that map, removing or
 * re-keying any order is one hash lookup plus one sift: O(log n).
 *
 */
#ifndef STOCK_ORDER_HEAP_HPP
#define STOCK_ORDER_HEAP_HPP

#include <cstddef>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include "order.hpp"

namespace stock {

/**
 * @brief Binary heap of Order objects supporting O(log n) removal and update by
 * id.
 *
 * @tparam Better Comparator; <tt>Better{}(a, b)</tt> returns true iff @p a must
 *         be closer to the top than @p b. It must be a strict weak ordering.
 *         Use BuyPriority for a bid book and SellPriority for an ask book.
 *
 * @invariant For every node i > 0, <tt>!better(heap[i], heap[(i-1)/2])</tt>.
 * @invariant @c index_ maps every stored id to its exact array position.
 */
template <typename Better> class OrderHeap {
public:
  /// @brief Construct an empty heap using the given comparator.
  explicit OrderHeap(Better better = Better{}) : better_(std::move(better)) {}

  /// @brief True if the heap holds no orders. O(1).
  [[nodiscard]] bool empty() const noexcept { return heap_.empty(); }

  /// @brief Number of orders stored. O(1).
  [[nodiscard]] std::size_t size() const noexcept { return heap_.size(); }

  /// @brief True if an order with this id is stored. O(1) expected.
  [[nodiscard]] bool contains(OrderId id) const {
    return index_.count(id) != 0;
  }

  /**
   * @brief Look up a stored order by id.
   * @return Pointer to the order, or @c nullptr if absent. The pointer is
   *         invalidated by any mutating call.
   * @par Complexity O(1) expected.
   */
  [[nodiscard]] const Order *find(OrderId id) const {
    const auto it = index_.find(id);
    return it == index_.end() ? nullptr : &heap_[it->second];
  }

  /**
   * @brief Highest-priority order.
   * @throws std::out_of_range if the heap is empty.
   * @par Complexity O(1).
   */
  [[nodiscard]] const Order &top() const {
    if (heap_.empty())
      throw std::out_of_range("OrderHeap::top: heap is empty");
    return heap_.front();
  }

  /**
   * @brief Insert an order.
   * @throws std::invalid_argument if an order with the same id is already
   * stored.
   * @par Complexity O(log n) expected.
   */
  void push(const Order &order) {
    if (index_.count(order.id) != 0) {
      throw std::invalid_argument("OrderHeap::push: duplicate order id");
    }
    index_.emplace(order.id, heap_.size());
    try {
      heap_.push_back(order);
    } catch (...) {
      index_.erase(order.id); // roll back so both containers stay consistent
      throw;
    }
    siftUp(heap_.size() - 1);
  }

  /**
   * @brief Remove and return the highest-priority order.
   * @throws std::out_of_range if the heap is empty.
   * @par Complexity O(log n).
   */
  Order pop() {
    Order best = top();
    removeAt(0);
    return best;
  }

  /**
   * @brief Remove the order with the given id, wherever it sits in the heap.
   * @return true if an order was removed, false if the id was not stored.
   * @par Complexity O(log n) expected.
   */
  bool erase(OrderId id) {
    const auto it = index_.find(id);
    if (it == index_.end())
      return false;
    removeAt(it->second);
    return true;
  }

  /**
   * @brief Replace a stored order (matched by <tt>order.id</tt>) with new
   * contents and restore the heap property.
   *
   * Typically used to change the price (and arrival stamp) of a pending order.
   *
   * @return true if the order was found and replaced, false otherwise.
   * @par Complexity O(log n) expected.
   */
  bool update(const Order &order) {
    const auto it = index_.find(order.id);
    if (it == index_.end())
      return false;
    const std::size_t i = it->second;
    heap_[i] = order;
    siftDown(siftUp(i)); // at most one of the two sifts actually moves the node
    return true;
  }

  /**
   * @brief Verify the heap and index invariants (for tests / debugging).
   * @par Complexity O(n).
   */
  [[nodiscard]] bool validate() const {
    if (index_.size() != heap_.size())
      return false;
    for (std::size_t i = 0; i < heap_.size(); ++i) {
      const auto it = index_.find(heap_[i].id);
      if (it == index_.end() || it->second != i)
        return false;
      if (i > 0 && better_(heap_[i], heap_[(i - 1) / 2]))
        return false;
    }
    return true;
  }

private:
  /// Swap two array slots and keep the index in sync.
  void swapNodes(std::size_t i, std::size_t j) {
    std::swap(heap_[i], heap_[j]);
    index_[heap_[i].id] = i;
    index_[heap_[j].id] = j;
  }

  /// Move node @p i up while it beats its parent; returns its final position.
  std::size_t siftUp(std::size_t i) {
    while (i > 0) {
      const std::size_t parent = (i - 1) / 2;
      if (!better_(heap_[i], heap_[parent]))
        break;
      swapNodes(i, parent);
      i = parent;
    }
    return i;
  }

  /// Move node @p i down while a child beats it.
  void siftDown(std::size_t i) {
    const std::size_t n = heap_.size();
    for (;;) {
      std::size_t best = i;
      const std::size_t left = 2 * i + 1;
      const std::size_t right = left + 1;
      if (left < n && better_(heap_[left], heap_[best]))
        best = left;
      if (right < n && better_(heap_[right], heap_[best]))
        best = right;
      if (best == i)
        return;
      swapNodes(i, best);
      i = best;
    }
  }

  /// Remove the node at array position @p i (precondition: i < size()).
  void removeAt(std::size_t i) {
    const OrderId removed = heap_[i].id;
    const std::size_t last = heap_.size() - 1;
    if (i != last)
      swapNodes(i, last);
    heap_.pop_back();
    index_.erase(removed);
    if (i != last)
      siftDown(
          siftUp(i)); // the node moved into slot i may belong higher or lower
  }

  Better better_;           ///< Priority comparator.
  std::vector<Order> heap_; ///< Array-backed binary heap.
  std::unordered_map<OrderId, std::size_t>
      index_; ///< id -> position in @c heap_.
};

} // namespace stock

#endif // STOCK_ORDER_HEAP_HPP