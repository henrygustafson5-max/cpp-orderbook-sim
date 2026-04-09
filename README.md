# cpp-orderbook-sim

A limit order book matching engine written in C++20, with a full unit test suite and Google Benchmark microbenchmarks.

## Features

- Price-time priority (FIFO) matching for limit orders (bid/ask)
- Market order execution that walks the book across multiple price levels
- Order cancellation
- Quantity reduction (reduce the resting quantity of an order without losing its position)
- Cancel-replace (atomically cancel and re-submit an order at a new price/quantity)
- Trade log with full execution reports (aggressor/resting IDs, price, qty, timestamp)
- 80 Google Test unit tests covering limit, market, cancel, reduce, and cancel-replace scenarios
- Google Benchmark microbenchmarks for limit inserts, market fills, and mixed workloads

## Project Structure

```
include/
  order.hpp            # LimitOrder, OrderSide, Price/Quantity/OrderID types
  order_book.hpp       # OrderBook — std::list-based price levels with O(1) iterator lookup
  matching_engine.hpp  # MatchingEngine public API
  trade.hpp            # Trade and TradeLog

src/
  order.cpp
  order_book.cpp
  matching_engine.cpp
  trade.cpp
  benchmark.cpp        # Google Benchmark entry point
  main.cpp

tests/
  orderbook_test.cpp   # 80 Google Test cases
```

## Build

Requires CMake 3.20+ and a C++20 compiler. Dependencies (Google Test, Google Benchmark) are fetched automatically via `FetchContent`.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Run Tests

```bash
./build/orderbook_tests
```

## Run Benchmarks

```bash
./build/orderbook_bench
```

Example output:

```
Benchmark                      Time             CPU   Iterations UserCounters...
--------------------------------------------------------------------------------
BM_LimitInsert/1000        50331 ns        50307 ns        13804 items_per_second=19.9M/s
BM_LimitInsert/100000    5166916 ns      5151619 ns          134 items_per_second=19.4M/s
BM_MarketFill/10000      1545051 ns      1543222 ns          495 items_per_second=6.48M/s
BM_Mixed/200000         10372902 ns     10361913 ns           69 items_per_second=19.3M/s
```

## API

```cpp
MatchingEngine engine;

// Submit orders
engine.submitLimitOrder(OrderSide::Bid, /*qty*/ 10, /*id*/ 1, /*price*/ 100);
engine.submitMarketOrder(OrderSide::Ask, /*qty*/ 10, /*id*/ 2);

// Cancel a resting order — returns false if not found
engine.cancelOrder(id);

// Reduce the resting quantity — newQty must be > 0 and < current qty
// Order retains its price-time priority position
engine.reduceOrder(id, /*newQty*/ 5);

// Cancel-replace — atomically cancels the order and re-submits at a new
// price/quantity. The new order is treated as fresh (loses prior priority).
// Returns false if the original order is not found.
engine.cancelReplace(id, /*newQty*/ 10, /*newPrice*/ 105);

// Inspect the book
engine.bestBid();   // std::optional<Price>
engine.bestAsk();   // std::optional<Price>
engine.hasBid();
engine.hasAsk();
engine.getLogSize();
```

## Design

`OrderBook` stores bids in a `std::map<Price, PriceLevel, std::greater>` (highest first) and asks in a `std::map<Price, PriceLevel, std::less>` (lowest first). Each `PriceLevel` holds a `std::list<unique_ptr<LimitOrder>>` so that individual orders can be erased in O(1) using stored iterators. A flat `unordered_map<OrderID, LookUp>` maps every live order ID to its iterator, side, and price, enabling O(1) cancel, reduce, and cancel-replace without scanning queues.

`MatchingEngine` wraps the book and records every fill as a `Trade` in a `TradeLog`. Market orders and limit orders that cross walk the book level by level, consuming resting orders FIFO, until the incoming quantity is exhausted or no crossable liquidity remains.
