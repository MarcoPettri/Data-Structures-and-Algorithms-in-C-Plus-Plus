/**
 * @file order_book_test.cpp
 * @brief Self-contained tests for OrderHeap and OrderBook (no external
 * framework).
 *
 */
#include <algorithm>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <random>
#include <stdexcept>
#include <vector>

#include "order.hpp"
#include "order_book.hpp"
#include "order_heap.hpp"

namespace order_book_tests {

using namespace stock;

// ---------------------------------------------------------------------------
// Minimal test harness
// ---------------------------------------------------------------------------

int g_checks = 0;   ///< Number of CHECKs evaluated.
int g_failures = 0; ///< Number of CHECKs that failed.

/// Record a failed check with its location.
void reportFailure(const char *file, int line, const char *what) {
  ++g_failures;
  std::cerr << file << ':' << line << ": check failed: " << what << '\n';
}

#define CHECK(cond)                                                            \
  do {                                                                         \
    ++g_checks;                                                                \
    if (!(cond))                                                               \
      reportFailure(__FILE__, __LINE__, #cond);                                \
  } while (false)

#define CHECK_THROWS(expr, ExType)                                             \
  do {                                                                         \
    ++g_checks;                                                                \
    bool thrown_ = false;                                                      \
    try {                                                                      \
      (void)(expr);                                                            \
    } catch (const ExType &) {                                                 \
      thrown_ = true;                                                          \
    } catch (...) {                                                            \
    }                                                                          \
    if (!thrown_)                                                              \
      reportFailure(__FILE__, __LINE__, #expr " did not throw " #ExType);      \
  } while (false)

struct TestCase {
  const char *name;
  void (*fn)();
};

std::vector<TestCase> &registry() {
  static std::vector<TestCase> tests;
  return tests;
}

struct Registrar {
  Registrar(const char *name, void (*fn)()) {
    registry().push_back({name, fn});
  }
};

#define TEST(name)                                                             \
  void test_##name();                                                          \
  const Registrar registrar_##name{#name, &test_##name};                       \
  void test_##name()

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Whole dollars -> cents.
constexpr Price usd(Price dollars) noexcept { return dollars * 100; }

/// Expected trade, as an optional for direct comparison with Result::trade.
std::optional<Trade> tradeOf(OrderId buy, OrderId sell, Price price) {
  return Trade{buy, sell, price, kOrderQuantity};
}

// ---------------------------------------------------------------------------
// OrderHeap tests
// ---------------------------------------------------------------------------

TEST(heap_buy_side_pops_highest_price_then_earliest) {
  OrderHeap<BuyPriority> heap;
  heap.push({1, Side::Buy, usd(100), 0});
  heap.push({2, Side::Buy, usd(102), 1});
  heap.push({3, Side::Buy, usd(101), 2});
  heap.push({4, Side::Buy, usd(102), 3}); // same price as #2 but later
  CHECK(heap.validate());
  CHECK(heap.size() == 4);
  CHECK(heap.pop().id == 2);
  CHECK(heap.pop().id == 4);
  CHECK(heap.pop().id == 3);
  CHECK(heap.pop().id == 1);
  CHECK(heap.empty());
}

TEST(heap_sell_side_pops_lowest_price_then_earliest) {
  OrderHeap<SellPriority> heap;
  heap.push({1, Side::Sell, usd(105), 0});
  heap.push({2, Side::Sell, usd(101), 1});
  heap.push({3, Side::Sell, usd(103), 2});
  heap.push({4, Side::Sell, usd(101), 3});
  CHECK(heap.pop().id == 2);
  CHECK(heap.pop().id == 4);
  CHECK(heap.pop().id == 3);
  CHECK(heap.pop().id == 1);
}

TEST(heap_erase_update_find_and_errors) {
  OrderHeap<SellPriority> heap;
  heap.push({1, Side::Sell, usd(105), 0});
  heap.push({2, Side::Sell, usd(101), 1});
  heap.push({3, Side::Sell, usd(103), 2});
  heap.push({4, Side::Sell, usd(102), 3});

  CHECK(heap.contains(3));
  CHECK(!heap.contains(99));
  CHECK(heap.find(3) != nullptr && heap.find(3)->price == usd(103));
  CHECK(heap.find(99) == nullptr);

  CHECK(heap.erase(2)); // erase the current top
  CHECK(!heap.erase(2));
  CHECK(heap.top().id == 4);
  CHECK(heap.validate());

  CHECK(
      heap.update({1, Side::Sell, usd(100), 4})); // move the worst to the best
  CHECK(heap.top().id == 1);
  CHECK(heap.update({1, Side::Sell, usd(110), 5})); // and back to the worst
  CHECK(heap.top().id == 4);
  CHECK(!heap.update({99, Side::Sell, usd(1), 6}));
  CHECK(heap.validate());

  CHECK_THROWS(heap.push({4, Side::Sell, usd(1), 7}), std::invalid_argument);

  OrderHeap<SellPriority> empty;
  CHECK_THROWS(empty.top(), std::out_of_range);
  CHECK_THROWS(empty.pop(), std::out_of_range);
}

/// Best order in a reference map, according to BuyPriority.
Order bestOf(const std::map<OrderId, Order> &ref) {
  return std::min_element(ref.begin(), ref.end(),
                          [](const auto &a, const auto &b) {
                            return BuyPriority{}(a.second, b.second);
                          })
      ->second;
}

TEST(heap_random_operations_match_reference) {
  std::mt19937_64 rng(42);
  std::uniform_int_distribution<int> opDist(0, 3);
  std::uniform_int_distribution<Price> priceDist(1, 50);

  OrderHeap<BuyPriority> heap;
  std::map<OrderId, Order> ref;
  OrderId nextId = 1;
  Sequence seq = 0;

  for (int step = 0; step < 5000; ++step) {
    const int op = ref.empty() ? 0 : opDist(rng);
    if (op == 0) {
      const Order o{nextId++, Side::Buy, priceDist(rng), seq++};
      heap.push(o);
      ref.emplace(o.id, o);
    } else if (op == 3) {
      const Order expected = bestOf(ref);
      CHECK(heap.pop().id == expected.id);
      ref.erase(expected.id);
    } else {
      auto it = ref.begin();
      std::advance(it, static_cast<std::ptrdiff_t>(rng() % ref.size()));
      if (op == 1) {
        CHECK(heap.erase(it->first));
        ref.erase(it);
      } else {
        Order o = it->second;
        o.price = priceDist(rng);
        o.sequence = seq++;
        CHECK(heap.update(o));
        it->second = o;
      }
    }
    CHECK(heap.validate());
    CHECK(heap.size() == ref.size());
    if (!ref.empty())
      CHECK(heap.top().id == bestOf(ref).id);
  }
}

// ---------------------------------------------------------------------------
// OrderBook tests - C-8.1 (entering orders)
// ---------------------------------------------------------------------------

TEST(book_order_rests_when_nothing_to_match) {
  OrderBook book;
  const auto buy = book.submit(Side::Buy, usd(100));
  const auto sell = book.submit(Side::Sell, usd(101));
  CHECK(buy.accepted && !buy.trade.has_value());
  CHECK(sell.accepted && !sell.trade.has_value());
  CHECK(buy.id != sell.id);
  CHECK(book.pendingBuys() == 1 && book.pendingSells() == 1);
  CHECK(book.bestBid() == usd(100));
  CHECK(book.bestAsk() == usd(101));
  CHECK(book.checkInvariants());
}

TEST(book_empty_book_has_no_best_prices) {
  OrderBook book;
  CHECK(!book.bestBid().has_value());
  CHECK(!book.bestAsk().has_value());
  CHECK(book.checkInvariants());
}

TEST(book_buy_matches_cheapest_sell_at_resting_price) {
  OrderBook book;
  const auto s1 = book.submit(Side::Sell, usd(102));
  const auto s2 = book.submit(Side::Sell, usd(101));
  const auto s3 = book.submit(Side::Sell, usd(103));
  const auto buy = book.submit(Side::Buy, usd(104));

  CHECK(buy.trade ==
        tradeOf(buy.id, s2.id, usd(101))); // cheapest ask, at ITS price
  CHECK(book.pendingSells() == 2 && book.pendingBuys() == 0);
  CHECK(book.bestAsk() == usd(102));
  CHECK(book.find(s1.id).has_value() && book.find(s3.id).has_value());
  CHECK(!book.find(s2.id).has_value());
  CHECK(!book.find(buy.id).has_value()); // an executed order never rests
}

TEST(book_sell_matches_highest_buy_at_resting_price) {
  OrderBook book;
  const auto b1 = book.submit(Side::Buy, usd(100));
  const auto b2 = book.submit(Side::Buy, usd(99));
  const auto sell = book.submit(Side::Sell, usd(98));

  CHECK(sell.trade == tradeOf(b1.id, sell.id, usd(100)));
  CHECK(book.bestBid() == usd(99));
  CHECK(book.find(b2.id).has_value());
}

TEST(book_price_boundary_equal_prices_match_one_cent_apart_do_not) {
  OrderBook book;
  const auto sell = book.submit(Side::Sell, usd(100));
  const auto low = book.submit(Side::Buy, usd(100) - 1); // $99.99 < $100.00
  CHECK(!low.trade.has_value());
  CHECK(book.pendingBuys() == 1 && book.pendingSells() == 1);

  const auto exact =
      book.submit(Side::Buy, usd(100)); // y <= x holds with equality
  CHECK(exact.trade == tradeOf(exact.id, sell.id, usd(100)));
  CHECK(book.pendingSells() == 0);
}

TEST(book_same_price_is_served_in_arrival_order) {
  OrderBook book;
  const auto first = book.submit(Side::Sell, usd(100));
  const auto second = book.submit(Side::Sell, usd(100));
  const auto buy1 = book.submit(Side::Buy, usd(100));
  const auto buy2 = book.submit(Side::Buy, usd(100));
  CHECK(buy1.trade == tradeOf(buy1.id, first.id, usd(100)));
  CHECK(buy2.trade == tradeOf(buy2.id, second.id, usd(100)));
}

TEST(book_each_order_consumes_exactly_one_counterpart) {
  OrderBook book;
  for (int i = 0; i < 3; ++i)
    (void)book.submit(Side::Sell, usd(100));
  const auto buy = book.submit(Side::Buy, usd(100));
  CHECK(buy.trade.has_value() && buy.trade->quantity == kOrderQuantity);
  CHECK(book.pendingSells() == 2); // 100 shares in, 100 shares out
  CHECK(book.pendingBuys() == 0);
}

TEST(book_rejects_non_positive_prices) {
  OrderBook book;
  CHECK_THROWS(book.submit(Side::Buy, 0), std::invalid_argument);
  CHECK_THROWS(book.submit(Side::Sell, -5), std::invalid_argument);
  CHECK(book.pendingBuys() == 0 && book.pendingSells() == 0);
}

// ---------------------------------------------------------------------------
// OrderBook tests - C-8.2 (updating and cancelling)
// ---------------------------------------------------------------------------

TEST(book_update_to_crossing_price_executes_buy) {
  OrderBook book;
  const auto sell = book.submit(Side::Sell, usd(105));
  const auto buy = book.submit(Side::Buy, usd(100));

  const auto r = book.updatePrice(buy.id, usd(105));
  CHECK(r.accepted && r.id == buy.id);
  CHECK(r.trade == tradeOf(buy.id, sell.id, usd(105)));
  CHECK(book.pendingBuys() == 0 && book.pendingSells() == 0);
  CHECK(!book.find(buy.id).has_value());
}

TEST(book_update_to_crossing_price_executes_sell) {
  OrderBook book;
  const auto buy = book.submit(Side::Buy, usd(100));
  const auto sell = book.submit(Side::Sell, usd(105));

  const auto r = book.updatePrice(sell.id, usd(99));
  CHECK(r.trade == tradeOf(buy.id, sell.id, usd(100))); // resting bid's price
  CHECK(book.pendingBuys() == 0 && book.pendingSells() == 0);
}

TEST(book_update_hits_best_opposite_order_only) {
  OrderBook book;
  const auto s1 = book.submit(Side::Sell, usd(102));
  const auto s2 = book.submit(Side::Sell, usd(101));
  const auto buy = book.submit(Side::Buy, usd(90));

  const auto r = book.updatePrice(buy.id, usd(110));
  CHECK(r.trade == tradeOf(buy.id, s2.id, usd(101)));
  CHECK(book.pendingSells() == 1 && book.find(s1.id).has_value());
}

TEST(book_update_repositions_order_inside_the_book) {
  OrderBook book;
  const auto a = book.submit(Side::Buy, usd(100));
  const auto b = book.submit(Side::Buy, usd(101));
  CHECK(book.bestBid() == usd(101));

  const auto up = book.updatePrice(a.id, usd(102));
  CHECK(up.accepted && !up.trade.has_value());
  CHECK(book.bestBid() == usd(102));
  CHECK(book.find(a.id)->price == usd(102));

  const auto down = book.updatePrice(a.id, usd(90));
  CHECK(down.accepted && !down.trade.has_value());
  CHECK(book.bestBid() == usd(101));
  CHECK(book.find(b.id).has_value());
  CHECK(book.pendingBuys() == 2);
  CHECK(book.checkInvariants());
}

TEST(book_update_same_price_keeps_time_priority) {
  OrderBook book;
  const auto a = book.submit(Side::Buy, usd(100));
  const auto b = book.submit(Side::Buy, usd(100));
  const Sequence before = book.find(a.id)->sequence;

  const auto r = book.updatePrice(a.id, usd(100));
  CHECK(r.accepted && !r.trade.has_value());
  CHECK(book.find(a.id)->sequence == before);

  const auto sell = book.submit(Side::Sell, usd(100));
  CHECK(sell.trade == tradeOf(a.id, sell.id, usd(100))); // a is still first
  CHECK(book.find(b.id).has_value());
}

TEST(book_changing_price_sends_order_to_back_of_queue) {
  OrderBook book;
  const auto a = book.submit(Side::Buy, usd(100));
  const auto b = book.submit(Side::Buy, usd(100));
  (void)book.updatePrice(a.id, usd(99));
  (void)book.updatePrice(a.id, usd(100)); // same price as before, but re-queued

  const auto sell = book.submit(Side::Sell, usd(100));
  CHECK(sell.trade == tradeOf(b.id, sell.id, usd(100)));
  CHECK(book.find(a.id).has_value());
}

TEST(book_update_of_unknown_executed_or_cancelled_order_is_rejected) {
  OrderBook book;
  const auto sell = book.submit(Side::Sell, usd(100));
  const auto buy = book.submit(Side::Buy, usd(100)); // executes against sell
  const auto live = book.submit(Side::Buy, usd(50));
  CHECK(book.cancel(live.id));

  CHECK(!book.updatePrice(9999, usd(10)).accepted);
  CHECK(!book.updatePrice(sell.id, usd(10)).accepted);
  CHECK(!book.updatePrice(buy.id, usd(10)).accepted);
  CHECK(!book.updatePrice(live.id, usd(10)).accepted);
  CHECK(book.pendingBuys() == 0 && book.pendingSells() == 0);
}

TEST(book_update_with_invalid_price_leaves_order_untouched) {
  OrderBook book;
  const auto a = book.submit(Side::Buy, usd(100));
  CHECK_THROWS(book.updatePrice(a.id, 0), std::invalid_argument);
  CHECK(book.find(a.id)->price == usd(100));
  CHECK(book.checkInvariants());
}

TEST(book_cancel_removes_pending_orders_once) {
  OrderBook book;
  const auto a = book.submit(Side::Buy, usd(100));
  const auto b = book.submit(Side::Sell, usd(110));
  CHECK(book.cancel(a.id));
  CHECK(!book.cancel(a.id));
  CHECK(book.cancel(b.id));
  CHECK(!book.cancel(12345));
  CHECK(book.pendingBuys() == 0 && book.pendingSells() == 0);
  CHECK(!book.bestBid().has_value() && !book.bestAsk().has_value());
}

// ---------------------------------------------------------------------------
// Randomized differential test against a naive reference model
// ---------------------------------------------------------------------------

/**
 * @brief Deliberately simple O(n) implementation of the same specification.
 *
 * Shares no code with OrderBook other than the plain data types, so agreement
 * between the two is meaningful.
 */
class ReferenceBook {
public:
  OrderBook::Result submit(Side side, Price price) {
    const Order order{nextId_++, side, price, nextSequence_++};
    auto trade = tryMatch(order);
    if (!trade)
      pending_.push_back(order);
    return {order.id, true, trade};
  }

  OrderBook::Result updatePrice(OrderId id, Price newPrice) {
    const auto it = locate(id);
    if (it == pending_.end())
      return {id, false, std::nullopt};
    if (it->price == newPrice)
      return {id, true, std::nullopt};
    Order order = *it;
    pending_.erase(it);
    order.price = newPrice;
    order.sequence = nextSequence_++;
    auto trade = tryMatch(order);
    if (!trade)
      pending_.push_back(order);
    return {id, true, trade};
  }

  bool cancel(OrderId id) {
    const auto it = locate(id);
    if (it == pending_.end())
      return false;
    pending_.erase(it);
    return true;
  }

  std::optional<Price> bestBid() const {
    std::optional<Price> best;
    for (const Order &o : pending_) {
      if (o.side == Side::Buy && (!best || o.price > *best))
        best = o.price;
    }
    return best;
  }

  std::optional<Price> bestAsk() const {
    std::optional<Price> best;
    for (const Order &o : pending_) {
      if (o.side == Side::Sell && (!best || o.price < *best))
        best = o.price;
    }
    return best;
  }

  std::size_t count(Side side) const {
    return static_cast<std::size_t>(
        std::count_if(pending_.begin(), pending_.end(),
                      [side](const Order &o) { return o.side == side; }));
  }

private:
  std::vector<Order>::iterator locate(OrderId id) {
    return std::find_if(pending_.begin(), pending_.end(),
                        [id](const Order &o) { return o.id == id; });
  }

  /// Linear scan for the best eligible opposite order; removes it if found.
  std::optional<Trade> tryMatch(const Order &in) {
    const bool buying = in.side == Side::Buy;
    auto best = pending_.end();
    for (auto it = pending_.begin(); it != pending_.end(); ++it) {
      if (it->side == in.side)
        continue;
      if (buying ? it->price > in.price : it->price < in.price)
        continue; // not eligible
      if (best == pending_.end()) {
        best = it;
        continue;
      }
      const bool betterPrice =
          buying ? it->price < best->price : it->price > best->price;
      if (betterPrice ||
          (it->price == best->price && it->sequence < best->sequence))
        best = it;
    }
    if (best == pending_.end())
      return std::nullopt;
    const Order resting = *best;
    pending_.erase(best);
    return buying ? Trade{in.id, resting.id, resting.price, kOrderQuantity}
                  : Trade{resting.id, in.id, resting.price, kOrderQuantity};
  }

  std::vector<Order> pending_;
  OrderId nextId_{1};
  Sequence nextSequence_{0};
};

bool sameResult(const OrderBook::Result &a, const OrderBook::Result &b) {
  return a.id == b.id && a.accepted == b.accepted && a.trade == b.trade;
}

TEST(book_randomized_differential_against_reference) {
  std::mt19937_64 rng(20261023);
  std::uniform_int_distribution<int> opDist(0, 9);
  std::uniform_int_distribution<Price> dollarDist(
      80, 120); // narrow range => many ties/crossings

  OrderBook book;
  ReferenceBook ref;
  std::vector<OrderId> issued;
  std::size_t trades = 0;

  for (int step = 0; step < 30000; ++step) {
    const int op = opDist(rng);
    if (op <= 5 || issued.empty()) {
      const Side side = (rng() & 1U) != 0 ? Side::Buy : Side::Sell;
      const Price price = usd(dollarDist(rng));
      const auto got = book.submit(side, price);
      const auto want = ref.submit(side, price);
      CHECK(sameResult(got, want));
      issued.push_back(got.id);
      trades += got.trade.has_value() ? 1U : 0U;
    } else if (op <= 8) {
      // Mostly existing ids (pending or already executed), sometimes a bogus
      // one.
      const OrderId id =
          (rng() % 20 == 0) ? 1'000'000 : issued[rng() % issued.size()];
      const Price price = usd(dollarDist(rng));
      const auto got = book.updatePrice(id, price);
      const auto want = ref.updatePrice(id, price);
      CHECK(sameResult(got, want));
      trades += got.trade.has_value() ? 1U : 0U;
    } else {
      const OrderId id = issued[rng() % issued.size()];
      CHECK(book.cancel(id) == ref.cancel(id));
    }

    CHECK(book.bestBid() == ref.bestBid());
    CHECK(book.bestAsk() == ref.bestAsk());
    CHECK(book.pendingBuys() == ref.count(Side::Buy));
    CHECK(book.pendingSells() == ref.count(Side::Sell));
    CHECK(book.checkInvariants());
  }
  CHECK(trades > 1000); // sanity: the workload really exercised matching
}

} // namespace order_book_tests
