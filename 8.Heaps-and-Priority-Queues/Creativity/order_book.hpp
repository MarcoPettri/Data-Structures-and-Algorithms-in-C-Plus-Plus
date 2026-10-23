/**
 * @file order_book.hpp
 * @brief Order book for exercises C-8.1 and C-8.2: matching stock orders in
 * O(log n).
 *
 * @section scheme_c81 C-8.1 - the scheme
 *
 * Keep two priority queues:
 *  - **bids**: pending buy orders, a max-heap on price (best bid on top);
 *  - **asks**: pending sell orders, a min-heap on price (best ask on top).
 *
 * Invariant: *at rest, the best bid is strictly below the best ask*; otherwise
 * those two orders could have been processed.
 *
 * Entering a buy order at price x:
 *  1. Peek at the best ask y (O(1)).
 *  2. If y <= x, remove it from the asks (O(log n)) and report a trade. The new
 *     order is consumed and is never inserted.
 *  3. Otherwise insert the new order into the bids (O(log n)).
 *
 * A sell order is symmetric. Since every order is for exactly 100 shares, an
 * order is matched with *at most one* counterpart, so each entry performs one
 * peek plus either one removal or one insertion: O(log n) whether or not the
 * order can be processed immediately. Only the top of the opposite heap needs
 * to be tested: if the best ask is too expensive, every other ask is as well.
 *
 * Ties on price are resolved by arrival time (price-time priority). A trade
 * executes at the price of the order that was already resting in the book.
 *
 * @section scheme_c82 C-8.2 - updating pending orders
 *
 * Replace the priority queues with *adaptable* priority queues (see
 * OrderHeap): each heap keeps a hash map from order id to its array position,
 * so any pending order can be located in O(1) expected time and then removed
 * or re-keyed in O(log n) by the usual swap-and-sift.
 *
 * Updating the price of order @c id:
 *  1. Locate it in the bids or asks via its id (O(1) expected). If it is not
 *     there it was already processed (or cancelled): reject.
 *  2. Treat the new price like a fresh entry: if it now crosses the best
 *     opposite order, execute the trade and delete the order (O(log n));
 *     otherwise re-key it in place and sift (O(log n)).
 *
 * Changing the price gives the order a new arrival stamp (it goes to the back
 * of the queue at its new price); re-submitting the *same* price is a no-op
 * that keeps its place. Cancellation is the same operation without step 2.
 *
 * @section usage Example
 * @code
 * stock::OrderBook book;
 * auto sell = book.submit(stock::Side::Sell, 10100);        // $101.00, rests
 * auto buy  = book.submit(stock::Side::Buy,  10000);        // $100.00, rests
 * auto r    = book.updatePrice(buy.id, 10100);              // now crosses
 * // r.trade -> { buyId = buy.id, sellId = sell.id, price = 10100, quantity =
 * 100 }
 * @endcode
 */
#ifndef STOCK_ORDER_BOOK_HPP
#define STOCK_ORDER_BOOK_HPP

#include <cstddef>
#include <optional>

#include "order.hpp"
#include "order_heap.hpp"

namespace stock {

/**
 * @brief Matching engine for 100-share limit orders.
 *
 * Not thread-safe: callers must serialise access externally.
 * All operations are O(log n) (expected, because of the id hash map), where n
 * is the number of pending orders.
 */
class OrderBook {
public:
  /**
   * @brief Outcome of submit() / updatePrice().
   */
  struct Result {
    OrderId id;    ///< Id of the order the call concerned.
    bool accepted; ///< false only if updatePrice() found no pending order with
                   ///< that id.
    std::optional<Trade> trade; ///< Set if the order was executed immediately.
  };

  /// @brief Create an empty book.
  OrderBook() = default;

  /**
   * @brief Enter a new order for kOrderQuantity shares.
   *
   * If it can be processed against the best opposite order it is executed at
   * once; otherwise it rests in the book until a later order matches it.
   *
   * @param side  Buy or Sell.
   * @param price Limit price in cents; must be positive.
   * @return The new order's id and, if it traded, the Trade.
   * @throws std::invalid_argument if @p price <= 0 (the book is left
   * unchanged).
   * @par Complexity O(log n).
   */
  Result submit(Side side, Price price);

  /**
   * @brief Change the price of a pending order (C-8.2).
   *
   * @param id       Id previously returned by submit().
   * @param newPrice New limit price in cents; must be positive.
   * @return @c accepted == false if no pending order has this id (never
   * existed, already executed, or cancelled). Otherwise @c accepted == true,
   * and
   *         @c trade is set if the new price made the order immediately
   * executable.
   * @throws std::invalid_argument if @p newPrice <= 0 (the order is left
   * unchanged).
   * @par Complexity O(log n) expected.
   */
  Result updatePrice(OrderId id, Price newPrice);

  /**
   * @brief Withdraw a pending order.
   * @return true if the order was pending and has been removed.
   * @par Complexity O(log n) expected.
   */
  bool cancel(OrderId id);

  /// @brief Highest pending buy price, if any. O(1).
  [[nodiscard]] std::optional<Price> bestBid() const;

  /// @brief Lowest pending sell price, if any. O(1).
  [[nodiscard]] std::optional<Price> bestAsk() const;

  /// @brief Number of pending buy orders. O(1).
  [[nodiscard]] std::size_t pendingBuys() const noexcept {
    return bids_.size();
  }

  /// @brief Number of pending sell orders. O(1).
  [[nodiscard]] std::size_t pendingSells() const noexcept {
    return asks_.size();
  }

  /// @brief Copy of the pending order with this id, if there is one. O(1)
  /// expected.
  [[nodiscard]] std::optional<Order> find(OrderId id) const;

  /**
   * @brief Check internal invariants (both heaps well-formed, best bid < best
   * ask).
   * @note Intended for tests and debugging.
   * @par Complexity O(n).
   */
  [[nodiscard]] bool checkInvariants() const;

private:
  /**
   * @brief Try to execute @p incoming against the best opposite order.
   * @return The trade (opposite order already removed), or nullopt if no match.
   *         Never touches the heap that @p incoming belongs to.
   */
  std::optional<Trade> match(const Order &incoming);

  /**
   * @brief Re-price @p current, which is stored in @p own, to @p newPrice.
   * @tparam Heap Type of the heap that currently stores the order.
   */
  template <typename Heap>
  Result reprice(Heap &own, Order current, Price newPrice);

  OrderHeap<BuyPriority> bids_;  ///< Pending buys, best (highest) price on top.
  OrderHeap<SellPriority> asks_; ///< Pending sells, best (lowest) price on top.
  OrderId nextId_{1};            ///< Next id to hand out.
  Sequence nextSequence_{0};     ///< Next arrival stamp to hand out.
};

} // namespace stock

#endif // STOCK_ORDER_BOOK_HPP