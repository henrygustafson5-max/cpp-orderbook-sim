MATCHING ENGINE (PROTOTYPE)
OVERVIEW
This project is a single-symbol limit order book and matching engine prototype written in modern C++.
It supports market orders and limit orders with correct price–time priority, partial fills, and trade generation.
The purpose of this stage is correctness and clarity of matching logic, not performance optimization or production readiness.
This is an educational and exploratory implementation intended to model how real matching engines behave internally.

FEATURES (CURRENT)
Order Types
Market Buy
Market Sell
Limit Buy (fill and rest)
Limit Sell (fill and rest)
Matching Behavior
Best ask is the lowest ask price
Best bid is the highest bid price
FIFO (price–time priority) enforced within each price level
Partial and full fills supported
A single order may generate multiple trades
Unfilled limit quantity is rested on the book
Order Book
Separate bid and ask books
Price levels stored in ordered containers
FIFO queues at each price level
Empty price levels are automatically removed
Trades
One trade generated per execution
Trade fields include price, quantity, aggressor order ID, resting order ID, aggressor side, and timestamp
Trades are recorded in an in-memory trade log

DESIGN DECISIONS
Correctness over cleverness
Logic is written explicitly with clear control flow and guard conditions so that invariants are easy to reason about.
Single-threaded, single-symbol
The engine focuses on matching logic only. Concurrency and multi-symbol routing are intentionally out of scope.
Value semantics
Orders and trades are stored by value. Ownership and lifetimes are explicit and easy to reason about.
Single-file implementation
All code currently lives in one file to allow rapid iteration and easy reasoning during the design phase.
The code will be split into multiple translation units once the design stabilizes.

OUT OF SCOPE (INTENTIONAL)
This prototype does not currently support:
Order cancellation or modification
Self-trade prevention
Input validation (zero or negative quantities, invalid prices)
Concurrency or locking
Persistence or recovery
Networking, APIs, or FIX integration
Multi-symbol support
Performance optimizations
These features are planned for future iterations.

CODE CONTENTS (CURRENT)
MarketOrder and LimitOrder types
OrderBook with bid and ask sides
MatchingEngine implementing market and limit order matching
Trade and TradeLog for execution recording
USAGE
At this stage, the engine is intended to be driven by a simple test harness or manual calls from main().
Typical usage:
Create a MatchingEngine instance
Add limit orders to the book
Submit market or limit orders
Observe generated trades and remaining book state
A formal test harness will be added later.

CURRENT FOCUS
Verifying matching correctness
Maintaining order book invariants
Ensuring safe ownership and lifetime semantics
Preparing the codebase for refactoring and extension

PLANNED NEXT STEPS
Add order cancellation support
Refactor matching logic to reduce duplication
Split code into headers and source files
Add assertions and invariant checks
Introduce a basic test harness
Explore performance-oriented data structures

DISCLAIMER
This project is not intended for live trading.
It is a learning-oriented implementation designed to explore matching engine mechanics.
