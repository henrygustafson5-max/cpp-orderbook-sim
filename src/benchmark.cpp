#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>
#include "matching_engine.hpp"

using Clock = std::chrono::steady_clock;
using Nanoseconds = std::chrono::nanoseconds;

static constexpr int RUNS = 5;

static long long medianOf(std::vector<long long>& v)
{
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}

void benchmarkLimitInsert(std::size_t numOrders)
{
    std::vector<long long> samples;
    samples.reserve(RUNS);

    for (int run = 0; run < RUNS; ++run)
    {
        MatchingEngine engine;
        const auto start = Clock::now();
        for (std::size_t i = 0; i < numOrders; ++i)
        {
            Price price = 100 + static_cast<Price>(i % 50);
            engine.submitLimitOrder(OrderSide::Bid, 100, OrderIDGenerator::next(), price);
        }
        const auto end = Clock::now();
        samples.push_back(std::chrono::duration_cast<Nanoseconds>(end - start).count());
    }

    const long long minNs  = *std::min_element(samples.begin(), samples.end());
    const long long medNs  = medianOf(samples);

    std::cout << "Limit inserts: " << numOrders << "  (runs=" << RUNS << ")\n";
    std::cout << "  Min per insert (ns):    "
              << static_cast<double>(minNs) / static_cast<double>(numOrders) << "\n";
    std::cout << "  Median per insert (ns): "
              << static_cast<double>(medNs) / static_cast<double>(numOrders) << "\n\n";
}

void benchmarkMarketFill(std::size_t depth)
{
    std::vector<long long> samples;
    samples.reserve(RUNS);

    for (int run = 0; run < RUNS; ++run)
    {
        MatchingEngine engine;

        for (std::size_t i = 0; i < depth; ++i)
        {
            const Price price    = static_cast<Price>(100 + static_cast<Price>(i));
            const Quantity qty   = static_cast<Quantity>(100);
            engine.submitLimitOrder(OrderSide::Ask, qty, OrderIDGenerator::next(), price);
        }

        const Quantity marketQty =
            static_cast<Quantity>(depth) * static_cast<Quantity>(100);

        const auto start = Clock::now();
        engine.submitMarketOrder(OrderSide::Bid, marketQty, OrderIDGenerator::next());
        const auto end = Clock::now();

        samples.push_back(std::chrono::duration_cast<Nanoseconds>(end - start).count());
    }

    const long long minNs  = *std::min_element(samples.begin(), samples.end());
    const long long medNs  = medianOf(samples);

    std::cout << "Market fill depth: " << depth << "  (runs=" << RUNS << ")\n";
    std::cout << "  Min per price level (ns):    "
              << static_cast<double>(minNs) / static_cast<double>(depth) << "\n";
    std::cout << "  Median per price level (ns): "
              << static_cast<double>(medNs) / static_cast<double>(depth) << "\n\n";
}

void benchmarkMixed(std::size_t ops)
{
    std::vector<long long> samples;
    samples.reserve(RUNS);

    for (int run = 0; run < RUNS; ++run)
    {
        MatchingEngine engine;
        const auto start = Clock::now();

        for (std::size_t i = 0; i < ops; ++i)
        {
            const bool isMarket = (i % static_cast<std::size_t>(3)) == static_cast<std::size_t>(0);
            if (isMarket)
            {
                const Quantity marketQty = static_cast<Quantity>(50);
                engine.submitMarketOrder(OrderSide::Bid, marketQty, OrderIDGenerator::next());
            }
            else
            {
                const Price price  = static_cast<Price>(100 + static_cast<Price>(i % 20));
                const Quantity qty = static_cast<Quantity>(100);
                engine.submitLimitOrder(OrderSide::Ask, qty, OrderIDGenerator::next(), price);
            }
        }

        const auto end = Clock::now();
        samples.push_back(std::chrono::duration_cast<Nanoseconds>(end - start).count());
    }

    const long long minNs  = *std::min_element(samples.begin(), samples.end());
    const long long medNs  = medianOf(samples);

    std::cout << "Mixed workload ops: " << ops << "  (runs=" << RUNS << ")\n";
    std::cout << "  Min per operation (ns):    "
              << static_cast<double>(minNs) / static_cast<double>(ops) << "\n";
    std::cout << "  Median per operation (ns): "
              << static_cast<double>(medNs) / static_cast<double>(ops) << "\n\n";
}
