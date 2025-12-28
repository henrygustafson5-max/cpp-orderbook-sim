# C++ Order Book Simulator 

A single-threaded limit order book implemented in C++. This project focuses on data structures, price-time priority matching, and clean abstraction design. 

## Order Book Design

The order book maintains separate bid and ask containers with
price–time priority:

- Bids are ordered from highest price to lowest price
- Asks are ordered from lowest price to highest price
- Orders at the same price level preserve FIFO ordering

The order book is responsible only for maintaining valid market
state. All matching logic is handled by the matching engine.
