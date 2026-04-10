# cpp-orderbook-sim

A limit order book matching engine written in C++20, with a full unit test suite and Google Benchmark microbenchmarks.

## Features

- Price-time priority (FIFO) matching for limit orders (bid/ask)
- Market order execution that walks the book across multiple price levels
- Three limit order types:
  - **GTC (Good-Till-Cancel):** rests in the book until filled or cancelled (default)
  - **IOC (Immediate-Or-Cancel):** fills as much as possible immediately; any unfilled remainder is dropped without resting
  - **FOK (Fill-Or-Kill):** fills entirely in one shot or the whole order is killed with zero partial fills
- Order cancellation
- Quantity reduction (reduce the resting quantity of an order without losing its position)
- Cancel-replace (atomically cancel and re-submit an order at a new price/quantity)
- Trade log with full execution reports (aggressor/resting IDs, price, qty, timestamp)
- 116 Google Test unit tests covering limit, market, IOC, FOK, cancel, reduce, and cancel-replace scenarios
- Google Benchmark microbenchmarks for limit inserts, market fills, mixed workloads, IOC/FOK fills, cancel, reduce, and cancel-replace

## Project Structure

```
include/
  order.hpp            # LimitOrder, OrderSide, LimitType (GTC/IOC/FOK), Price/Quantity/OrderID types
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
  orderbook_test.cpp   # 116 Google Test cases
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

## API

```cpp
MatchingEngine engine;

// Submit a GTC limit order (default — rests if not filled)
engine.submitLimitOrder(OrderSide::Bid, /*qty*/ 10, /*id*/ 1, /*price*/ 100);

// Submit an IOC limit order — fills immediately, remainder dropped
engine.submitLimitOrder(OrderSide::Bid, /*qty*/ 10, /*id*/ 2, /*price*/ 100, LimitType::IOC);

// Submit a FOK limit order — fills entirely or is killed with no partial fills
engine.submitLimitOrder(OrderSide::Ask, /*qty*/ 10, /*id*/ 3, /*price*/ 100, LimitType::FOK);

// Submit a market order
engine.submitMarketOrder(OrderSide::Ask, /*qty*/ 10, /*id*/ 4);

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

**IOC** orders share the same fill loop as GTC but skip the final `book.addBid/addAsk` call, so any unfilled remainder is silently dropped.

**FOK** orders perform an upfront volume check (`FOKVolumeCheck`) before consuming any liquidity. If the full quantity cannot be filled at crossable prices the entire order is rejected atomically — no partial fills are ever recorded.
