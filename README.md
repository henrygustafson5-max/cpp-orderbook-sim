This project is a single-symbol limit order book and matching engine prototype written in modern C++ (C++20).
It supports market and limit orders with correct price–time priority, partial fills, and trade generation.
The primary goal of this stage is correctness, clarity, and invariant-driven design, not performance optimization or production readiness.
This is an educational and exploratory implementation intended to model how real matching engines behave internally and to serve as a foundation for future extensions.
Features (Current)
Order Types
Market Buy
Market Sell
Limit Buy (fill-and-rest)
Limit Sell (fill-and-rest)
Matching Behavior
Best ask = lowest ask price
Best bid = highest bid price
FIFO (price–time priority) enforced within each price level
Partial and full fills supported
A single order may generate multiple trades
Unfilled limit quantity is rested on the book
Order Book
Separate bid and ask books
Price levels stored in ordered containers
FIFO queues at each price level
Empty price levels automatically removed
Atomic consume operations return execution details
Trades
One trade generated per execution
Trade fields include:
Price
Quantity
Aggressor order ID
Resting order ID
Aggressor side
Timestamp
Trades recorded in an in-memory trade log
Design Decisions
Correctness over cleverness
Matching logic is written explicitly with clear control flow and guard conditions so that invariants are easy to reason about and verify.
Separation of responsibilities
OrderBook manages price levels and resting liquidity
MatchingEngine handles matching logic and trade generation
Trade recording is isolated in a TradeLog
Execution details are passed explicitly via an ExecutionReport
Single-threaded, single-symbol
The engine focuses purely on matching logic.
Concurrency, locking, and multi-symbol routing are intentionally out of scope.
Explicit ownership and lifetimes
Resting orders are owned by the order book via std::unique_ptr
Execution data is copied out safely before orders are erased
No shared ownership or hidden lifetimes
Multi-file, modular structure
The codebase is split into headers and source files with a clean include/ and src/ layout, enforcing clear interfaces and compilation boundaries.
Project Structure
include/
├── order.hpp
├── order_book.hpp
├── matching_engine.hpp
└── trade.hpp

src/
├── main.cpp
├── order.cpp
├── order_book.cpp
├── matching_engine.cpp
└── trade.cpp
The project is built using CMake with strict compiler warnings enabled.
Out of Scope (Intentional)
This prototype does not currently support:
Order cancellation or modification
Self-trade prevention
Input validation (e.g., zero or negative quantities)
Concurrency or locking
Persistence or recovery
Networking, APIs, or FIX integration
Multi-symbol support
Performance optimizations
These features are intentionally deferred to keep the current focus on correctness and design clarity.
Usage
At this stage, the engine is intended to be driven by:
A simple test harness
Manual calls from main()
Typical usage:
Create a MatchingEngine instance
Submit limit orders to populate the book
Submit market or limit orders
Observe generated trades and remaining book state
A formal test suite will be added in a future iteration.
Current Focus
Verifying matching correctness
Maintaining order book invariants
Ensuring safe ownership and lifetime semantics
Preparing the codebase for extensibility
Planned Next Steps
Add order cancellation and modification by OrderID
Introduce an order index for efficient lookup
Refactor matching logic to reduce duplication
Add unit tests for matching behavior and invariants
Improve invariant documentation and assertions
Explore alternative data structures for performance
