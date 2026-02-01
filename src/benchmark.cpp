#include <chrono>
#include "matching_engine.hpp"
#include <iostream>

using Clock = std::chrono::steady_clock;

using Nanoseconds = std::chrono::nanoseconds;

void benchmarkLimitInsert(std::size_t numOrders)
{
    MatchingEngine engine;

    auto start = Clock::now();

    for (std::size_t i = 0; i < numOrders; ++i)
    {
        Price price = 100 + (i % 50);
        engine.submitLimitOrder(
            OrderSide::Bid,
            100,
            OrderIDGenerator::next(),
            price
        );
    }

    auto end = Clock::now();
    const Nanoseconds elapsed = std::chrono::duration_cast<Nanoseconds>(end - start);

    const double total_ms =
    static_cast<double>(elapsed.count()) / 1'000'000.0;

    const double avg_ns =
    static_cast<double>(elapsed.count()) /
    static_cast<double>(numOrders);


    std::cout << "Limit inserts: " << numOrders << "\n";
    std::cout << "Total time (ms): " << total_ms << "\n";
    std::cout << "Avg per insert (ns): " << avg_ns << "\n\n";
}

void benchmarkMarketFill(std::size_t depth)
{
    MatchingEngine engine;

    // Build ask side of the book
    for (std::size_t i = 0; i < depth; ++i)
    {
        const Price price =
            static_cast<Price>(100 + static_cast<Price>(i));

        const Quantity qty =
            static_cast<Quantity>(100);

        engine.submitLimitOrder(
            OrderSide::Ask,
            qty,
            OrderIDGenerator::next(),
            price
        );
    }

    const Quantity marketQty =
        static_cast<Quantity>(depth) *
        static_cast<Quantity>(100);

    const auto start = Clock::now();

    engine.submitMarketOrder(
        OrderSide::Bid,
        marketQty,
        OrderIDGenerator::next()
    );

    const auto end = Clock::now();

    const auto elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    const double total_ms =
        static_cast<double>(elapsed.count()) / 1'000'000.0;

    const double avg_ns_per_level =
        static_cast<double>(elapsed.count()) /
        static_cast<double>(depth);

    std::cout << "Market fill depth: " << depth << "\n";
    std::cout << "Total time (ms): " << total_ms << "\n";
    std::cout << "Avg per price level (ns): " << avg_ns_per_level << "\n\n";
}


void benchmarkMixed(std::size_t ops)
{
    MatchingEngine engine;

    const auto start = Clock::now();

    for (std::size_t i = 0; i < ops; ++i)
    {
        const bool isMarket =
            (i % static_cast<std::size_t>(3)) == static_cast<std::size_t>(0);

        if (isMarket)
        {
            const Quantity marketQty =
                static_cast<Quantity>(50);

            engine.submitMarketOrder(
                OrderSide::Bid,
                marketQty,
                OrderIDGenerator::next()
            );
        }
        else
        {
            const Price price =
                static_cast<Price>(100 + static_cast<Price>(i % 20));

            const Quantity qty =
                static_cast<Quantity>(100);

            engine.submitLimitOrder(
                OrderSide::Ask,
                qty,
                OrderIDGenerator::next(),
                price
            );
        }
    }

    const auto end = Clock::now();

    const auto elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    const double total_ms =
        static_cast<double>(elapsed.count()) / 1'000'000.0;

    const double avg_ns_per_op =
        static_cast<double>(elapsed.count()) /
        static_cast<double>(ops);

    std::cout << "Mixed workload ops: " << ops << "\n";
    std::cout << "Total time (ms): " << total_ms << "\n";
    std::cout << "Avg per operation (ns): " << avg_ns_per_op << "\n\n";
}

