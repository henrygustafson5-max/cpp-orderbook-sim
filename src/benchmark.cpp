#include <benchmark/benchmark.h>
#include "matching_engine.hpp"
#include <random>
#include <vector>

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

// IOC: measure throughput of immediate-or-cancel limit bids that each partially
// fill one resting ask before their remainder is dropped.
static void BM_IOCFill(benchmark::State& state)
{
    const auto ops = static_cast<std::size_t>(state.range(0));
    for (auto _ : state)
    {
        state.PauseTiming();
        MatchingEngine engine;
        // Seed the ask side with large resting orders at a spread of prices
        for (std::size_t i = 0; i < 50; ++i)
        {
            const Price    price = static_cast<Price>(100 + static_cast<Price>(i));
            engine.submitLimitOrder(OrderSide::Ask, 1000000, OrderIDGenerator::next(), price);
        }
        state.ResumeTiming();

        for (std::size_t i = 0; i < ops; ++i)
        {
            const Price price = static_cast<Price>(100 + static_cast<Price>(i % 50));
            engine.submitLimitOrder(OrderSide::Bid, 10, OrderIDGenerator::next(), price, LimitType::IOC);
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(ops));
}
BENCHMARK(BM_IOCFill)->Arg(100000);

// FOK: measure throughput of fill-or-kill bids — half succeed (sufficient volume),
// half fail (price too low to reach enough liquidity).
static void BM_FOKFill(benchmark::State& state)
{
    const auto ops = static_cast<std::size_t>(state.range(0));
    for (auto _ : state)
    {
        state.PauseTiming();
        MatchingEngine engine;
        // Seed the ask side — 100 units at each of 50 price levels
        for (std::size_t i = 0; i < 50; ++i)
        {
            const Price price = static_cast<Price>(110 + static_cast<Price>(i));
            engine.submitLimitOrder(OrderSide::Ask, 100, OrderIDGenerator::next(), price);
        }
        state.ResumeTiming();

        for (std::size_t i = 0; i < ops; ++i)
        {
            // Alternate: aggressive price (crosses, FOK passes) vs. passive (killed)
            const Price    price = (i % 2 == 0)
                ? static_cast<Price>(160)   // crosses many levels — FOK passes
                : static_cast<Price>(105);  // below all asks — FOK killed
            engine.submitLimitOrder(OrderSide::Bid, 10, OrderIDGenerator::next(), price, LimitType::FOK);
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(ops));
}
BENCHMARK(BM_FOKFill)->Arg(100000);

// Cancel: measure the cost of cancelling a resting order by ID.
static void BM_CancelOrder(benchmark::State& state)
{
    const auto numOrders = static_cast<std::size_t>(state.range(0));
    for (auto _ : state)
    {
        state.PauseTiming();
        MatchingEngine engine;
        std::vector<OrderID> ids;
        ids.reserve(numOrders);
        for (std::size_t i = 0; i < numOrders; ++i)
        {
            OrderID id = OrderIDGenerator::next();
            ids.push_back(id);
            const Price price = static_cast<Price>(100 + static_cast<Price>(i % 50));
            engine.submitLimitOrder(OrderSide::Bid, 100, id, price);
        }
        state.ResumeTiming();

        for (OrderID id : ids)
        {
            engine.cancelOrder(id);
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(numOrders));
}
BENCHMARK(BM_CancelOrder)->Arg(10000)->Arg(100000);

// ReduceOrder: measure the cost of reducing a resting order's quantity by ID.
static void BM_ReduceOrder(benchmark::State& state)
{
    const auto numOrders = static_cast<std::size_t>(state.range(0));
    for (auto _ : state)
    {
        state.PauseTiming();
        MatchingEngine engine;
        std::vector<OrderID> ids;
        ids.reserve(numOrders);
        for (std::size_t i = 0; i < numOrders; ++i)
        {
            OrderID id = OrderIDGenerator::next();
            ids.push_back(id);
            const Price price = static_cast<Price>(100 + static_cast<Price>(i % 50));
            engine.submitLimitOrder(OrderSide::Bid, 100, id, price);
        }
        state.ResumeTiming();

        for (OrderID id : ids)
        {
            engine.reduceOrder(id, 50); // reduce every order to half quantity
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(numOrders));
}
BENCHMARK(BM_ReduceOrder)->Arg(10000)->Arg(100000);

// CancelReplace: measure the cost of atomically cancelling and re-submitting
// an order at a new price. Each replace moves the bid up by 1 tick, so the
// new order immediately becomes the best bid and rests.
static void BM_CancelReplace(benchmark::State& state)
{
    const auto numOrders = static_cast<std::size_t>(state.range(0));
    for (auto _ : state)
    {
        state.PauseTiming();
        MatchingEngine engine;
        std::vector<OrderID> ids;
        ids.reserve(numOrders);
        for (std::size_t i = 0; i < numOrders; ++i)
        {
            OrderID id = OrderIDGenerator::next();
            ids.push_back(id);
            const Price price = static_cast<Price>(50 + static_cast<Price>(i % 40));
            engine.submitLimitOrder(OrderSide::Bid, 100, id, price);
        }
        state.ResumeTiming();

        for (std::size_t i = 0; i < numOrders; ++i)
        {
            const Price newPrice = static_cast<Price>(90 + static_cast<Price>(i % 10));
            engine.cancelReplace(ids[i], 100, newPrice);
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(numOrders));
}
BENCHMARK(BM_CancelReplace)->Arg(10000)->Arg(100000);

// ─────────────────────────────────────────────────────────────────────────────
// Realistic Market Benchmark
//
// Simulates a live continuous-trading session across all order types and
// modification features.  A midpoint price drifts on a slow random walk so
// the spread stays economically meaningful throughout the run.
//
// Operation mix (approximate):
//   35% — GTC passive limit (1–10 ticks away from mid, both sides)
//   10% — market order (small qty, both sides)
//   15% — IOC aggressive limit (0–2 ticks through the spread)
//    5% — FOK all-or-nothing (0–5 ticks aggressive; ~half fill, ~half kill)
//   15% — cancel   (oldest resting GTC orders consumed first)
//   10% — reduce   (trim qty of a resting order, retains queue position)
//   10% — cancel-replace (reprice a resting order ±1–3 ticks)
//
// A live-order pool (vector<OrderID>) is fed by every GTC submission and
// drained by cancels and cancel-replaces so modification ops are never
// wasted against already-consumed IDs.
// ─────────────────────────────────────────────────────────────────────────────
static void BM_RealisticMarket(benchmark::State& state)
{
    const auto ops = static_cast<std::size_t>(state.range(0));

    // Distributions are constructed once; the RNG is reseeded each iteration
    // so every benchmark iteration sees the exact same event sequence.
    std::mt19937 rng;

    // op selector [0, 99]
    std::uniform_int_distribution<int> opDist(0, 99);

    // passive GTC offset from mid: 1–10 ticks
    std::uniform_int_distribution<int> passiveOffset(1, 10);

    // IOC/FOK offset from mid: 0–5 ticks (aggressors sit at or through the spread)
    std::uniform_int_distribution<int> aggressiveOffset(0, 5);

    // qty distributions
    std::uniform_int_distribution<Quantity> passiveQty(50, 500);
    std::uniform_int_distribution<Quantity> aggressiveQty(10, 150);

    // cancel-replace reprice offset: 1–3 ticks
    std::uniform_int_distribution<int> repriceOffset(1, 3);

    // mid-price random walk: ±1 tick, updated every 100 ops
    std::uniform_int_distribution<int> drift(-1, 1);

    for (auto _ : state)
    {
        state.PauseTiming();

        rng.seed(12345);
        MatchingEngine engine;

        // Seed 20 price levels on each side so the book has realistic depth
        // from the very first timed operation.
        Price mid = 1000;
        std::vector<OrderID> livePool;
        livePool.reserve(ops / 2);

        for (int i = 1; i <= 20; ++i)
        {
            OrderID bidID = OrderIDGenerator::next();
            engine.submitLimitOrder(OrderSide::Bid, 200, bidID, mid - i);
            livePool.push_back(bidID);

            OrderID askID = OrderIDGenerator::next();
            engine.submitLimitOrder(OrderSide::Ask, 200, askID, mid + i);
            livePool.push_back(askID);
        }

        state.ResumeTiming();

        for (std::size_t i = 0; i < ops; ++i)
        {
            // Slowly drift the midpoint every 100 operations
            if (i % 100 == 0)
            {
                mid += static_cast<Price>(drift(rng));
                if (mid < 100) mid = 100;
            }

            const int roll = opDist(rng);

            if (roll < 35)
            {
                // ── Passive GTC limit ────────────────────────────────────────
                const bool isBid = (roll % 2 == 0);
                const int  offset = passiveOffset(rng);
                const Price price  = isBid ? mid - offset : mid + offset;
                const OrderID id   = OrderIDGenerator::next();
                engine.submitLimitOrder(
                    isBid ? OrderSide::Bid : OrderSide::Ask,
                    passiveQty(rng), id, price, LimitType::GTC);
                livePool.push_back(id);
            }
            else if (roll < 45)
            {
                // ── Market order ─────────────────────────────────────────────
                engine.submitMarketOrder(
                    (roll % 2 == 0) ? OrderSide::Bid : OrderSide::Ask,
                    aggressiveQty(rng),
                    OrderIDGenerator::next());
            }
            else if (roll < 60)
            {
                // ── IOC aggressive limit ─────────────────────────────────────
                // Price sits at or through the opposing best, so it will
                // partially or fully fill then have its remainder dropped.
                const bool isBid  = (roll % 2 == 0);
                const int  offset = aggressiveOffset(rng);
                const Price price  = isBid ? mid + offset : mid - offset;
                engine.submitLimitOrder(
                    isBid ? OrderSide::Bid : OrderSide::Ask,
                    aggressiveQty(rng),
                    OrderIDGenerator::next(),
                    (price > 0 ? price : 1),
                    LimitType::IOC);
            }
            else if (roll < 65)
            {
                // ── FOK all-or-nothing ────────────────────────────────────────
                // Uses a qty of 50–150: sometimes the resting book can satisfy
                // it (fill), sometimes not (kill), giving a realistic mix of
                // both paths through the FOK volume-check.
                const bool isBid  = (roll % 2 == 0);
                const int  offset = aggressiveOffset(rng);
                const Price price  = isBid ? mid + offset : mid - offset;
                engine.submitLimitOrder(
                    isBid ? OrderSide::Bid : OrderSide::Ask,
                    aggressiveQty(rng),
                    OrderIDGenerator::next(),
                    (price > 0 ? price : 1),
                    LimitType::FOK);
            }
            else if (roll < 80)
            {
                // ── Cancel ───────────────────────────────────────────────────
                if (livePool.empty()) continue;
                std::uniform_int_distribution<std::size_t> pick(0, livePool.size() - 1);
                const std::size_t idx = pick(rng);
                engine.cancelOrder(livePool[idx]); // no-op if already filled
                livePool[idx] = livePool.back();
                livePool.pop_back();
            }
            else if (roll < 90)
            {
                // ── Reduce ───────────────────────────────────────────────────
                // Trims resting qty to 10; no-ops silently if the order has
                // already been fully consumed or its qty is already <= 10.
                if (livePool.empty()) continue;
                std::uniform_int_distribution<std::size_t> pick(0, livePool.size() - 1);
                engine.reduceOrder(livePool[pick(rng)], 10);
            }
            else
            {
                // ── Cancel-Replace ───────────────────────────────────────────
                // Reprices a resting order by ±1–3 ticks relative to mid.
                // The old ID is removed from the pool; the replacement order
                // issued by cancelReplace gets a fresh ID not tracked here.
                if (livePool.empty()) continue;
                std::uniform_int_distribution<std::size_t> pick(0, livePool.size() - 1);
                const std::size_t idx   = pick(rng);
                const int         off   = repriceOffset(rng);
                const Price       newP  = (roll % 2 == 0) ? mid - off : mid + off;
                engine.cancelReplace(livePool[idx], passiveQty(rng), (newP > 0 ? newP : 1));
                livePool[idx] = livePool.back();
                livePool.pop_back();
            }
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(ops));
}
BENCHMARK(BM_RealisticMarket)->Arg(100000)->Arg(500000);

BENCHMARK_MAIN();
