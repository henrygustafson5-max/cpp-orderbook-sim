# C++ Order Book & Matching Engine

## Overview

This project implements a single-threaded limit order book and matching engine in modern C++, inspired by real-world electronic trading systems.

The goal is not to simulate an exchange UI, but to demonstrate correctness, performance-aware design, and clear ownership semantics in a non-trivial stateful system.

The engine supports:
- Price–time priority matching
- Market and limit orders
- Partial fills across multiple price levels
- Deterministic trade generation

---

## Build

Requires CMake 3.20+ and a C++20 compiler (clang++ or g++).

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/orderbook_sim
```

The default build type is `Release` (`-O3 -march=native`). To build without optimization for debugging:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

---

## High-Level Architecture

```
+-------------------+
|  MatchingEngine   |
|-------------------|
| - Owns OrderBook  |
| - Owns TradeLog   |
| - Matching logic  |
+---------+---------+
          |
          v
+-------------------+
|    OrderBook      |
|-------------------|
| - Bid side        |
| - Ask side        |
| - FIFO queues     |
| - No trade logic  |
+---------+---------+
          |
          v
+-------------------+
|   LimitOrder      |
|-------------------|
| - Price           |
| - Quantity        |
| - Order ID        |
+-------------------+
```

---

## Benchmarks

All benchmarks were run on an Apple M-series MacBook Pro, compiled with `-O3 -march=native`. Timings use `std::chrono::steady_clock` and each benchmark runs 5 times; min and median are reported.

### Limit Order Inserts

| Orders  | Min per Insert (ns) | Median per Insert (ns) |
|---------|--------------------:|----------------------:|
| 1,000   | 64.9                | 76.6                  |
| 100,000 | 36.7                | 42.2                  |

Larger runs benefit from cache warming.

### Market Order Sweep

A market buy order submitted against a pre-populated ask book.

| Price Levels | Min per Level (ns) | Median per Level (ns) |
|-------------|-------------------:|----------------------:|
| 10,000      | 171.9              | 212.8                 |

### Mixed Workload

2/3 limit orders and 1/3 market orders, simulating a realistic trading pattern.

| Operations | Min per Op (ns) | Median per Op (ns) |
|------------|----------------:|-------------------:|
| 200,000    | 40.1            | 40.9               |

**Notes:** Benchmarks validate algorithmic behavior and data structure choices, not production-grade throughput. Results reflect price-time priority enforcement, FIFO queue operations, and dynamic matching across price levels.

---

## Design Principles

**Separation of concerns**
`OrderBook` manages state only. All matching decisions live in `MatchingEngine`.

**Explicit ownership**
Orders are owned by the book using `std::unique_ptr`. No hidden copies — orders are only moved or mutated in place.

**Deterministic behavior**
Matching follows strict price–time priority with no non-determinism.

---

## Data Structures

### Order Storage

```
std::map<Price, std::deque<std::unique_ptr<LimitOrder>>>
```

- `std::map` — maintains sorted price levels: bids descending (`std::greater`), asks ascending (`std::less`)
- `std::deque` — preserves FIFO order at each price level
- `std::unique_ptr<LimitOrder>` — makes ownership explicit and prevents accidental copies

This structure closely mirrors how real matching engines model price levels.

---

## Matching Rules

### Market Orders
- Consume liquidity from the opposite side of the book
- Fill across multiple price levels if necessary
- Stop when quantity is fully filled or the book is empty

### Limit Orders
- Attempt immediate execution if the order crosses the spread
- Match against the best available price levels
- Any unfilled quantity is rested on the book at its limit price
- Matching respects price priority first, then FIFO time priority

### Partial Fills
- Supported for both market and limit orders
- Resting orders are reduced in place
- Orders are removed only when quantity reaches zero

---

## Trade Representation

Each execution generates a `Trade` record containing:

| Field             | Description                        |
|-------------------|------------------------------------|
| Trade ID          | Monotonically increasing identifier |
| Execution price   | Price at which the trade occurred  |
| Executed quantity | Quantity matched in this execution |
| Aggressor ID      | Order ID of the incoming order     |
| Resting ID        | Order ID of the matched book order |
| Aggressor side    | Bid or Ask                         |
| Timestamp         | `std::chrono::steady_clock` time   |

Trades are appended to a `TradeLog` owned by the matching engine.

---

## Complexity Analysis

| Operation              | Time Complexity            |
|------------------------|----------------------------|
| Limit order insert     | O(log N)                   |
| Best bid / ask lookup  | O(1)                       |
| Single fill execution  | O(1)                       |
| Multi-level market fill| O(K) — levels consumed     |

Where N is the number of distinct price levels and K is the number of levels crossed by a market order.

---

## Example Usage

```cpp
MatchingEngine engine;

// Rest a sell at 101
engine.submitLimitOrder(OrderSide::Ask, 100, OrderIDGenerator::next(), 101);

// Incoming buy limit at 102 — crosses the spread, executes immediately
engine.submitLimitOrder(OrderSide::Bid, 150, OrderIDGenerator::next(), 102);

// Market buy sweeps remaining book
engine.submitMarketOrder(OrderSide::Bid, 200, OrderIDGenerator::next());

engine.printTrade(0);
```

---

## Design Tradeoffs & Limitations

- **Single-threaded by design** — eliminates locking complexity to focus on core logic
- **No order cancellation or modification** — simplifies state management
- **No persistence layer** — orders live only in memory
- **Input validation** — zero-quantity and non-positive-price orders are rejected at submission

These choices were made intentionally to focus on correctness, clarity, and system design.

---

## Future Work

- Unit tests for correctness (FIFO ordering, partial fills, edge cases)
- Order cancellation and modification
- Deterministic event replay
- Optional concurrency extensions

---

## Why This Project

Built to demonstrate:

- Strong C++ ownership and lifetime management (`unique_ptr`, move semantics)
- Non-trivial data structure design suited to the domain
- Correct handling of partial state transitions
- Performance-aware system design with measurable results

Intentionally minimal in scope but realistic in behavior.
