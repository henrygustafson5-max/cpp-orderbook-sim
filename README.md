# cpp-orderbook-sim

A multi-symbol limit order book matching engine written in C++20, with a full unit test suite and a custom cycle-counter latency benchmark.

## Features

- Multi-symbol engine: an independent order book per symbol, indexed by `SymbolID`
- Price-time priority (FIFO) matching for limit orders (bid/ask)
- Market order execution that walks the book across multiple price levels
- Three limit order types:
  - **GTC (Good-Till-Cancel):** rests in the book until filled or cancelled (default)
  - **IOC (Immediate-Or-Cancel):** fills as much as possible immediately; any unfilled remainder is dropped without resting
  - **FOK (Fill-Or-Kill):** fills entirely in one shot or the whole order is killed with zero partial fills
- Order cancellation
- Quantity reduction (reduce the resting quantity of an order without losing its position)
- Cancel-replace (atomically cancel and re-submit an order at a new price/quantity)
- Trade log with full execution reports (aggressor/resting IDs, price, qty)
- 107 Google Test unit tests (12 suites) covering limit, market, IOC, FOK, cancel, reduce, and cancel-replace scenarios
- Custom microbenchmark that measures per-operation latency percentiles using the hardware cycle counter (`rdtsc` on x86-64, `cntvct_el0` on arm64)

## Project Structure

```
include/
  order.hpp            # LimitOrder, OrderSide, LimitType (GTC/IOC/FOK), OrderIDGenerator, Price/Quantity/OrderID types
  order_book.hpp       # OrderBook — std::map price levels, std::list-based queues with O(1) iterator lookup
  matching_engine.hpp  # MatchingEngine public API (multi-symbol)
  trade.hpp            # Trade and TradeLog
  timersetup.hpp       # cross-arch cycle-counter timing helpers used by the benchmark

src/
  order.cpp
  order_book.cpp
  matching_engine.cpp
  trade.cpp
  benchmark.cpp        # benchmark entry point (main)

tests/
  orderbook_test.cpp   # 107 Google Test cases
```

## Build

Requires CMake 3.20+ and a C++20 compiler. Dependencies (Google Test, Google Benchmark) are fetched automatically via `FetchContent`. The release flags use `-O3 -march=native`.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Run Tests

```bash
./build/orderbook_tests
```

## Run Benchmark

```bash
./build/orderbook_bench
```

## Benchmark

The benchmark builds a randomized workload of **500,000 operations** spread across **200 symbols** and replays it through the engine, timing every individual operation with the hardware cycle counter and reporting latency percentiles averaged over **10 runs**.

The workload is mixed and skewed to approximate real market traffic:

- **60%** limit orders (GTC / IOC / FOK chosen at random), **30%** cancels/quantity-reductions, **10%** market orders
- A "hot" set of ~50 symbols receives ~80% of the traffic; prices and quantities cluster around a mid band

Latest results (AMD Ryzen 7 PRO 8700GE, x86-64, TSC ≈ 3.65 GHz, `-O3 -march=native`):

| Metric          | Value     |
| --------------- | --------- |
| P90 latency     | 142.2 ns  |
| P99 latency     | 274.1 ns  |
| P99.9 latency   | 963.1 ns  |
| Cycles per op   | 276.6     |

> Numbers are hardware-dependent. See [PERFORMANCE.md](PERFORMANCE.md) for the running log of optimizations and how these figures change over time.

## API

```cpp
// One book per symbol; constructed with the number of symbols to support.
MatchingEngine engine(200);
SymbolID ticker = 7;

// Submit a GTC limit order (default — rests if not filled)
engine.submitLimitOrder(ticker, OrderSide::Bid, /*qty*/ 10, /*id*/ 1, /*price*/ 100);

// Submit an IOC limit order — fills immediately, remainder dropped
engine.submitLimitOrder(ticker, OrderSide::Bid, /*qty*/ 10, /*id*/ 2, /*price*/ 100, LimitType::IOC);

// Submit a FOK limit order — fills entirely or is killed with no partial fills
engine.submitLimitOrder(ticker, OrderSide::Ask, /*qty*/ 10, /*id*/ 3, /*price*/ 100, LimitType::FOK);

// Submit a market order
engine.submitMarketOrder(ticker, OrderSide::Ask, /*qty*/ 10, /*id*/ 4);

// Cancel a resting order — returns false if not found (looks up the owning book by ID)
engine.cancelOrder(id);

// Reduce the resting quantity — newQty must be > 0 and < current qty
// Order retains its price-time priority position
engine.reduceOrder(id, /*newQty*/ 5);

// Cancel-replace — atomically cancels the order and re-submits at a new
// price/quantity. The new order is treated as fresh (loses prior priority).
// Returns false if the original order is not found.
engine.cancelReplace(id, /*newQty*/ 10, /*newPrice*/ 105);

// Inspect a symbol's book
engine.bestBid(ticker);   // std::optional<Price>
engine.bestAsk(ticker);   // std::optional<Price>
engine.hasBid(ticker);
engine.hasAsk(ticker);
engine.getLogSize();
```

## Design

`OrderBook` stores bids in a `std::map<Price, PriceLevel, std::greater>` (highest first) and asks in a `std::map<Price, PriceLevel, std::less>` (lowest first). Each `PriceLevel` holds a `std::list<unique_ptr<LimitOrder>>` so that individual orders can be erased in O(1) using stored iterators. A per-book `unordered_map<OrderID, LookUp>` maps every live order ID to its iterator, side, and price, enabling O(1) cancel, reduce, and cancel-replace without scanning queues.

`MatchingEngine` holds a `std::vector<OrderBook>` indexed by `SymbolID`, so each symbol matches in isolation. Because cancel/reduce/cancel-replace are addressed only by `OrderID`, the engine keeps an `unordered_map<OrderID, SymbolID>` to route those requests to the correct book. Every fill is recorded as a `Trade` in a single shared `TradeLog`. Market orders and limit orders that cross walk the book level by level, consuming resting orders FIFO, until the incoming quantity is exhausted or no crossable liquidity remains.

**IOC** orders share the same fill loop as GTC but skip the final `book.addBid/addAsk` call, so any unfilled remainder is silently dropped.

**FOK** orders perform an upfront volume check (`FOKVolumeCheck`) before consuming any liquidity. If the full quantity cannot be filled at crossable prices the entire order is rejected atomically — no partial fills are ever recorded.
