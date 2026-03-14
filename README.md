# cpp-orderbook-sim

A limit order book matching engine written in C++20, with a full unit test suite and Google Benchmark microbenchmarks.

## Features

- Price-time priority matching for limit orders (bid/ask)
- Market order execution that walks the book
- Trade log with execution reports
- 38 Google Test unit tests covering limit, market, and mixed scenarios
- Google Benchmark microbenchmarks for limit inserts, market fills, and mixed workloads

## Project Structure

```
include/
  order.hpp            # LimitOrder, OrderSide, Price/Quantity/OrderID types
  order_book.hpp       # OrderBook (std::map-based price level queues)
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
  orderbook_test.cpp   # 38 Google Test cases
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

## Design

The `OrderBook` stores bids in a `std::map<Price, deque<LimitOrder*>, std::greater>` (highest price first) and asks in a `std::map<Price, deque<LimitOrder*>, std::less>` (lowest price first). Matching happens at the best price level, consuming resting orders FIFO until the incoming order is fully filled or no crossable liquidity remains.

The `MatchingEngine` wraps the book and records every fill in a `TradeLog`.
