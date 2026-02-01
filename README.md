C++ Order Book & Matching Engine
Overview
This project implements a single-threaded limit order book and matching engine in modern C++, inspired by real-world electronic trading systems.
The goal of this project is not to simulate an exchange UI, but to demonstrate correctness, performance-aware design, and clear ownership semantics in a non-trivial stateful system.
The engine supports:
Price–time priority matching
Market and limit orders
Partial fills across multiple price levels
Deterministic trade generation
High-Level Architecture
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
| - Timestamp       |
+-------------------+

 Benchmarks
All benchmarks were run on a single-threaded build using std::chrono::steady_clock on an Apple M-series MacBook Pro. Results represent wall-clock time and include all order book logic (price lookup, FIFO queue operations, and matching).
Limit Order Inserts
Orders	Total Time (ms)	Avg per Insert (ns)
1,000	0.673	673
100,000	39.6	396
Larger runs benefit from cache warming and amortized allocation overhead.
Market Order Sweep
A market buy order was submitted against a pre-populated ask book.
Price Levels	Total Time (ms)	Avg per Level (ns)
10,000	4.99	499
Mixed Workload
Workload consisted of 2/3 limit orders and 1/3 market orders, simulating a realistic trading pattern.
Operations	Total Time (ms)	Avg per Operation (ns)
200,000	45.8	229
Notes
Benchmarks are intended to validate algorithmic behavior and data structure choices, not to compete with production-grade trading systems.
Performance reflects:
Price-time priority enforcement
FIFO queues at each price level
Dynamic order matching
Results are deterministic and reproducible.

Trade execution events are recorded in a TradeLog owned by the MatchingEngine.
Design Principles
Separation of concerns:
The OrderBook manages state only. All matching decisions live in MatchingEngine.
Explicit ownership:
Orders are owned by the book using std::unique_ptr.
No hidden copies:
Orders are never copied, only moved or mutated in place.
Deterministic behavior:
Matching follows strict price–time priority.
Data Structures
Order Storage
std::map<Price, std::deque<std::unique_ptr<LimitOrder>>>
std::map
Maintains sorted price levels.
Bids: highest price first (std::greater)
Asks: lowest price first (std::less)
std::deque
Preserves FIFO order at each price level.
std::unique_ptr<LimitOrder>
Makes ownership explicit and prevents accidental copies.
This structure closely mirrors how real matching engines model price levels.
Matching Rules
Market Orders
Consume liquidity from the opposite side of the book
Fill across multiple price levels if necessary
Stop when:
Quantity is fully filled, or
The book is empty
Limit Orders
Attempt immediate execution if the order crosses the spread
Match against the best available price levels
Any unfilled quantity is rested on the book
Matching respects price priority first, then FIFO time priority
Partial Fills
Supported for both market and limit orders
Resting orders are reduced in place
Orders are removed only when quantity reaches zero
Trade Representation
Each execution generates a Trade record containing:
Trade ID
Execution price
Executed quantity
Aggressor order ID
Resting order ID
Aggressor side
Timestamp
Trades are appended to a TradeLog owned by the matching engine.
Complexity Analysis
Operation	Time Complexity
Limit order insert	O(log N)
Best bid / ask lookup	O(1)
Single fill execution	O(1)
Multi-level market fill	O(K) (levels consumed)
Where N is the number of price levels and K is the number of levels crossed.
Example Usage
MatchingEngine engine;

engine.submitLimitOrder(OrderSide::Ask, 100, OrderIDGenerator::next(), 101);
engine.submitLimitOrder(OrderSide::Bid, 150, OrderIDGenerator::next(), 102);

engine.submitMarketOrder(OrderSide::Bid, 200, OrderIDGenerator::next());

engine.printTrade(0);
Design Tradeoffs & Limitations
Single-threaded by design
No order cancellation or modification
No persistence layer
No concurrency or locking
Intended as a core matching engine, not a full exchange simulator
These choices were made intentionally to focus on correctness, clarity, and system design.
Future Work
Planned improvements:
Deterministic event replay
Benchmarking and latency measurement
Unit tests for edge cases
Optional concurrency extensions
Why This Project
This project was built to demonstrate:
Strong C++ ownership and lifetime management
Non-trivial data structure design
Correct handling of partial state transitions
Performance-aware system design
It is intentionally minimal in scope but realistic in behavior.
