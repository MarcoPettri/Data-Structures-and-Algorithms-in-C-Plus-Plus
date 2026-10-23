/**
 * @file order_book.cpp
 * @brief Implementation of stock::OrderBook.
 */
#include "order_book.hpp"

#include <stdexcept>

namespace stock {

namespace {

/// Reject non-positive prices before any state is modified.
void requireValidPrice(Price price) {
  if (price <= 0) {
    throw std::invalid_argument("OrderBook: price must be positive");
  }
}

} // namespace

std::optional<Trade> OrderBook::match(const Order &incoming) {
  if (incoming.side == Side::Buy) {
    if (asks_.empty() || !crosses(incoming.price, asks_.top().price))
      return std::nullopt;
    const Order resting = asks_.pop();
    return Trade{incoming.id, resting.id, resting.price, kOrderQuantity};
  }
  if (bids_.empty() || !crosses(bids_.top().price, incoming.price))
    return std::nullopt;
  const Order resting = bids_.pop();
  return Trade{resting.id, incoming.id, resting.price, kOrderQuantity};
}

OrderBook::Result OrderBook::submit(Side side, Price price) {
  requireValidPrice(price);

  const Order order{nextId_++, side, price, nextSequence_++};
  if (auto trade = match(order)) {
    return {order.id, true, trade};
  }

  if (side == Side::Buy) {
    bids_.push(order);
  } else {
    asks_.push(order);
  }
  return {order.id, true, std::nullopt};
}

template <typename Heap>
OrderBook::Result OrderBook::reprice(Heap &own, Order current, Price newPrice) {
  if (newPrice == current.price) {
    return {current.id, true,
            std::nullopt}; // nothing changes, keep time priority
  }

  current.price = newPrice;
  current.sequence =
      nextSequence_++; // a re-priced order queues behind existing ones

  if (auto trade = match(current)) {
    own.erase(current.id);
    return {current.id, true, trade};
  }
  own.update(current);
  return {current.id, true, std::nullopt};
}

OrderBook::Result OrderBook::updatePrice(OrderId id, Price newPrice) {
  requireValidPrice(newPrice);

  if (const Order *order = bids_.find(id))
    return reprice(bids_, *order, newPrice);
  if (const Order *order = asks_.find(id))
    return reprice(asks_, *order, newPrice);
  return {id, false, std::nullopt};
}

bool OrderBook::cancel(OrderId id) {
  return bids_.erase(id) || asks_.erase(id);
}

std::optional<Price> OrderBook::bestBid() const {
  if (bids_.empty())
    return std::nullopt;
  return bids_.top().price;
}

std::optional<Price> OrderBook::bestAsk() const {
  if (asks_.empty())
    return std::nullopt;
  return asks_.top().price;
}

std::optional<Order> OrderBook::find(OrderId id) const {
  if (const Order *order = bids_.find(id))
    return *order;
  if (const Order *order = asks_.find(id))
    return *order;
  return std::nullopt;
}

bool OrderBook::checkInvariants() const {
  if (!bids_.validate() || !asks_.validate())
    return false;
  if (bids_.empty() || asks_.empty())
    return true;
  return !crosses(bids_.top().price, asks_.top().price);
}

} // namespace stock