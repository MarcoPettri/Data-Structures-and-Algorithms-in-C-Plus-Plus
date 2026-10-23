/**
 * @file order.hpp
 * @brief Value types shared by the order-matching components (exercises C-8.1 /
 * C-8.2).
 *
 * This header is intentionally header-only: it contains plain data types and
 * tiny @c constexpr helpers, so there is no implementation file to pair with
 * it.
 */
#ifndef STOCK_ORDER_HPP
#define STOCK_ORDER_HPP

#include <cstdint>

namespace stock {

/// Unique, never-reused identifier of an order.
using OrderId = std::uint64_t;

/// Price in integer cents (avoids floating-point comparison pitfalls).
using Price = std::int64_t;

/// Monotonic arrival stamp; smaller means "arrived earlier".
using Sequence = std::uint64_t;

/// Every order in this exercise is for exactly this many shares.
inline constexpr int kOrderQuantity = 100;

/// Which side of the market an order is on.
enum class Side : std::uint8_t {
  Buy, ///< "buy 100 shares at $x each"
  Sell ///< "sell 100 shares at $y each"
};

/**
 * @brief A pending (not yet executed) order.
 */
struct Order {
  OrderId id{};         ///< Unique identifier.
  Side side{Side::Buy}; ///< Market side.
  Price price{};        ///< Limit price in cents.
  Sequence
      sequence{}; ///< Arrival stamp, used to break price ties (time priority).
};

/**
 * @brief The result of matching one buy order with one sell order.
 */
struct Trade {
  OrderId buyId{};  ///< Id of the buy order.
  OrderId sellId{}; ///< Id of the sell order.
  Price price{};    ///< Execution price in cents (the resting order's price).
  int quantity{kOrderQuantity}; ///< Number of shares exchanged.
};

/// @brief Equality for Trade (value semantics).
[[nodiscard]] constexpr bool operator==(const Trade &a,
                                        const Trade &b) noexcept {
  return a.buyId == b.buyId && a.sellId == b.sellId && a.price == b.price &&
         a.quantity == b.quantity;
}

/// @brief Inequality for Trade.
[[nodiscard]] constexpr bool operator!=(const Trade &a,
                                        const Trade &b) noexcept {
  return !(a == b);
}

/**
 * @brief Matching rule of the exercise: a buy at @p buyPrice and a sell at
 *        @p sellPrice can trade iff <tt>sellPrice <= buyPrice</tt>.
 */
[[nodiscard]] constexpr bool crosses(Price buyPrice, Price sellPrice) noexcept {
  return sellPrice <= buyPrice;
}

/**
 * @brief Strict weak ordering for the buy side ("bids").
 *
 * @c operator()(a, b) is true when @p a has *higher priority* than @p b:
 * higher price first, then earlier arrival. Because sequence numbers are
 * unique this is a strict total order over live orders.
 */
struct BuyPriority {
  [[nodiscard]] constexpr bool operator()(const Order &a,
                                          const Order &b) const noexcept {
    return a.price != b.price ? a.price > b.price : a.sequence < b.sequence;
  }
};

/**
 * @brief Strict weak ordering for the sell side ("asks").
 *
 * @c operator()(a, b) is true when @p a has *higher priority* than @p b:
 * lower price first, then earlier arrival.
 */
struct SellPriority {
  [[nodiscard]] constexpr bool operator()(const Order &a,
                                          const Order &b) const noexcept {
    return a.price != b.price ? a.price < b.price : a.sequence < b.sequence;
  }
};

} // namespace stock

#endif // STOCK_ORDER_HPP