#include <benchmark/benchmark.h>
#include "matching_engine.hpp"

static void BM_LimitInsert(benchmark::State& state)
{
    const auto numOrders = static_cast<std::size_t>(state.range(0));
    for (auto _ : state)
    {
        MatchingEngine engine;
        for (std::size_t i = 0; i < numOrders; ++i)
        {
            const Price price = 100 + static_cast<Price>(i % 50);
            engine.submitLimitOrder(OrderSide::Bid, 100, OrderIDGenerator::next(), price);
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(numOrders));
}
BENCHMARK(BM_LimitInsert)->Arg(1000)->Arg(100000);

static void BM_MarketFill(benchmark::State& state)
{
    const auto depth = static_cast<std::size_t>(state.range(0));
    for (auto _ : state)
    {
        state.PauseTiming();
        MatchingEngine engine;
        for (std::size_t i = 0; i < depth; ++i)
        {
            const Price    price = static_cast<Price>(100 + static_cast<Price>(i));
            const Quantity qty   = static_cast<Quantity>(100);
            engine.submitLimitOrder(OrderSide::Ask, qty, OrderIDGenerator::next(), price);
        }
        const Quantity marketQty =
            static_cast<Quantity>(depth) * static_cast<Quantity>(100);
        state.ResumeTiming();

        engine.submitMarketOrder(OrderSide::Bid, marketQty, OrderIDGenerator::next());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(depth));
}
BENCHMARK(BM_MarketFill)->Arg(10000);

static void BM_Mixed(benchmark::State& state)
{
    const auto ops = static_cast<std::size_t>(state.range(0));
    for (auto _ : state)
    {
        MatchingEngine engine;
        for (std::size_t i = 0; i < ops; ++i)
        {
            if ((i % 3) == 0)
            {
                engine.submitMarketOrder(
                    OrderSide::Bid, static_cast<Quantity>(50), OrderIDGenerator::next());
            }
            else
            {
                const Price    price = static_cast<Price>(100 + static_cast<Price>(i % 20));
                const Quantity qty   = static_cast<Quantity>(100);
                engine.submitLimitOrder(OrderSide::Ask, qty, OrderIDGenerator::next(), price);
            }
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(ops));
}
BENCHMARK(BM_Mixed)->Arg(200000);

BENCHMARK_MAIN();
