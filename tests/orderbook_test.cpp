#include <gtest/gtest.h>
#include "matching_engine.hpp"
#include "order.hpp"


static OrderID nextID() { return OrderIDGenerator::next(); }

// ─────────────────────────────────────────────────────────────────────────────
// Limit Bid Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(LimitBidTest, InsertNoAskShouldRest)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Bid, 50, nextID(), 100);

    ASSERT_TRUE(engine.bestBid().has_value());
    EXPECT_EQ(engine.bestBid().value(), 100);
    EXPECT_FALSE(engine.hasAsk());
    EXPECT_EQ(engine.getLogSize(), 0u);
}

TEST(LimitBidTest, InsertBelowBestAskShouldRest)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 50, nextID(), 105);
    engine.submitLimitOrder(OrderSide::Bid, 50, nextID(), 100); // below ask, no cross

    ASSERT_TRUE(engine.bestBid().has_value());
    EXPECT_EQ(engine.bestBid().value(), 100);
    EXPECT_TRUE(engine.hasAsk());
    EXPECT_EQ(engine.getLogSize(), 0u);
}

TEST(LimitBidTest, NewHighestBidGoesToFrontOfBook)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(),  99);
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), 101); // new best bid

    ASSERT_TRUE(engine.bestBid().has_value());
    EXPECT_EQ(engine.bestBid().value(), 101);
    EXPECT_EQ(engine.getLogSize(), 0u);
}

TEST(LimitBidTest, PartialFillRestsRemainder)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask,  5, nextID(), 100); // ask qty 5
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), 100); // bid qty 10, crosses

    // ask fully consumed, 5 units of bid rest
    EXPECT_EQ(engine.getLogSize(), 1u);
    EXPECT_FALSE(engine.hasAsk());
    ASSERT_TRUE(engine.bestBid().has_value());
    EXPECT_EQ(engine.bestBid().value(), 100);
}

TEST(LimitBidTest, ConsumeMultiplePriceLevels)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 5, nextID(), 100); // best ask
    engine.submitLimitOrder(OrderSide::Ask, 5, nextID(), 101);
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), 102); // crosses both levels

    EXPECT_EQ(engine.getLogSize(), 2u); // one trade per price level
    EXPECT_FALSE(engine.hasAsk());
    EXPECT_FALSE(engine.hasBid());
}

TEST(LimitBidTest, MultipleBidsSamePriceFIFOEnforced)
{
    MatchingEngine engine;
    // First bid qty 10, second bid qty 5 — both at same price
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Bid,  5, nextID(), 100);

    // Market sell qty 10 — FIFO: fully consumes first order (qty 10), second untouched
    // LIFO would generate 2 trades (5 from second + 5 from first); FIFO generates 1
    engine.submitMarketOrder(OrderSide::Ask, 10, nextID());

    EXPECT_EQ(engine.getLogSize(), 1u);
    ASSERT_TRUE(engine.hasBid());
    EXPECT_EQ(engine.bestBid().value(), 100);
}

TEST(LimitBidTest, NewLowestBidRestsAtBottom)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(),  99); // below current best

    ASSERT_TRUE(engine.bestBid().has_value());
    EXPECT_EQ(engine.bestBid().value(), 100); // best bid unchanged
    EXPECT_EQ(engine.getLogSize(), 0u);
}

TEST(LimitBidTest, PriceZeroRejects)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), 0);

    EXPECT_FALSE(engine.hasBid());
    EXPECT_EQ(engine.getLogSize(), 0u);
}

TEST(LimitBidTest, PriceNegativeRejects)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), -1);

    EXPECT_FALSE(engine.hasBid());
    EXPECT_EQ(engine.getLogSize(), 0u);
}

TEST(LimitBidTest, QuantityZeroRejects)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Bid, 0, nextID(), 100);

    EXPECT_FALSE(engine.hasBid());
    EXPECT_EQ(engine.getLogSize(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Limit Sell Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(LimitSellTest, InsertNoBidsShouldRest)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 50, nextID(), 100);

    ASSERT_TRUE(engine.bestAsk().has_value());
    EXPECT_EQ(engine.bestAsk().value(), 100);
    EXPECT_FALSE(engine.hasBid());
    EXPECT_EQ(engine.getLogSize(), 0u);
}

TEST(LimitSellTest, InsertAboveBestBidShouldRest)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Bid, 50, nextID(),  95);
    engine.submitLimitOrder(OrderSide::Ask, 50, nextID(), 100); // above best bid, no cross

    ASSERT_TRUE(engine.bestAsk().has_value());
    EXPECT_EQ(engine.bestAsk().value(), 100);
    EXPECT_TRUE(engine.hasBid());
    EXPECT_EQ(engine.getLogSize(), 0u);
}

TEST(LimitSellTest, NewBestAskGoesToFrontOfBook)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 105);
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 103); // new best ask

    ASSERT_TRUE(engine.bestAsk().has_value());
    EXPECT_EQ(engine.bestAsk().value(), 103);
    EXPECT_EQ(engine.getLogSize(), 0u);
}

TEST(LimitSellTest, PartialFillRestsRemainder)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Bid,  5, nextID(), 100); // bid qty 5
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 100); // ask qty 10, crosses

    // bid fully consumed, 5 units of ask rest
    EXPECT_EQ(engine.getLogSize(), 1u);
    EXPECT_FALSE(engine.hasBid());
    ASSERT_TRUE(engine.bestAsk().has_value());
    EXPECT_EQ(engine.bestAsk().value(), 100);
}

TEST(LimitSellTest, ConsumeMultipleBidLevels)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Bid, 5, nextID(), 101); // best bid
    engine.submitLimitOrder(OrderSide::Bid, 5, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 99); // crosses both levels

    EXPECT_EQ(engine.getLogSize(), 2u);
    EXPECT_FALSE(engine.hasBid());
    EXPECT_FALSE(engine.hasAsk());
}

TEST(LimitSellTest, MultipleAsksSamePriceFIFOEnforced)
{
    MatchingEngine engine;
    // First ask qty 10, second ask qty 5 — both at same price
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Ask,  5, nextID(), 100);

    // Market buy qty 10 — FIFO: fully consumes first order (qty 10), second untouched
    // LIFO would generate 2 trades (5 from second + 5 from first); FIFO generates 1
    engine.submitMarketOrder(OrderSide::Bid, 10, nextID());

    EXPECT_EQ(engine.getLogSize(), 1u);
    ASSERT_TRUE(engine.hasAsk());
    EXPECT_EQ(engine.bestAsk().value(), 100);
}

TEST(LimitSellTest, NewHighestAskRestsAtBottom)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 110); // above current best

    ASSERT_TRUE(engine.bestAsk().has_value());
    EXPECT_EQ(engine.bestAsk().value(), 100); // best ask unchanged
    EXPECT_EQ(engine.getLogSize(), 0u);
}

TEST(LimitSellTest, PriceZeroRejects)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 0);

    EXPECT_FALSE(engine.hasAsk());
    EXPECT_EQ(engine.getLogSize(), 0u);
}

TEST(LimitSellTest, PriceNegativeRejects)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), -1);

    EXPECT_FALSE(engine.hasAsk());
    EXPECT_EQ(engine.getLogSize(), 0u);
}

TEST(LimitSellTest, QuantityZeroRejects)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 0, nextID(), 100);

    EXPECT_FALSE(engine.hasAsk());
    EXPECT_EQ(engine.getLogSize(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Market Buy Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(MarketBuyTest, NoAsksSilentlyDropped)
{
    MatchingEngine engine;
    engine.submitMarketOrder(OrderSide::Bid, 10, nextID());

    EXPECT_EQ(engine.getLogSize(), 0u);
    EXPECT_FALSE(engine.hasBid());
    EXPECT_FALSE(engine.hasAsk());
}

TEST(MarketBuyTest, QuantityLessThanBestAskPartiallyFillsAsk)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 20, nextID(), 100);
    engine.submitMarketOrder(OrderSide::Bid,  5, nextID()); // wants 5, ask has 20

    EXPECT_EQ(engine.getLogSize(), 1u);
    ASSERT_TRUE(engine.hasAsk());
    EXPECT_EQ(engine.bestAsk().value(), 100); // ask still resting with 15 remaining
}

TEST(MarketBuyTest, ConsumeMultiplePriceLevels)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 5, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Ask, 5, nextID(), 101);
    engine.submitMarketOrder(OrderSide::Bid, 10, nextID()); // consumes both levels

    EXPECT_EQ(engine.getLogSize(), 2u);
    EXPECT_FALSE(engine.hasAsk());
    EXPECT_FALSE(engine.hasBid());
}

TEST(MarketBuyTest, ExceedsAvailableFillsAllDropsRemainder)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 5, nextID(), 100);
    engine.submitMarketOrder(OrderSide::Bid, 20, nextID()); // wants 20, only 5 exists

    EXPECT_EQ(engine.getLogSize(), 1u);
    EXPECT_FALSE(engine.hasAsk());
    EXPECT_FALSE(engine.hasBid()); // unfilled remainder is dropped, not rested
}

TEST(MarketBuyTest, QuantityZeroRejects)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 100);
    engine.submitMarketOrder(OrderSide::Bid, 0, nextID());

    EXPECT_EQ(engine.getLogSize(), 0u);
    ASSERT_TRUE(engine.hasAsk());
    EXPECT_EQ(engine.bestAsk().value(), 100); // ask untouched
}

// ─────────────────────────────────────────────────────────────────────────────
// Market Sell Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(MarketSellTest, NoBidsSilentlyDropped)
{
    MatchingEngine engine;
    engine.submitMarketOrder(OrderSide::Ask, 10, nextID());

    EXPECT_EQ(engine.getLogSize(), 0u);
    EXPECT_FALSE(engine.hasBid());
    EXPECT_FALSE(engine.hasAsk());
}

TEST(MarketSellTest, QuantityLessThanBestBidPartiallyFillsBid)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Bid, 20, nextID(), 100);
    engine.submitMarketOrder(OrderSide::Ask,  5, nextID()); // wants 5, bid has 20

    EXPECT_EQ(engine.getLogSize(), 1u);
    ASSERT_TRUE(engine.hasBid());
    EXPECT_EQ(engine.bestBid().value(), 100); // bid still resting with 15 remaining
}

TEST(MarketSellTest, ConsumeMultipleBidLevels)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Bid, 5, nextID(), 101); // best bid
    engine.submitLimitOrder(OrderSide::Bid, 5, nextID(), 100);
    engine.submitMarketOrder(OrderSide::Ask, 10, nextID()); // consumes both levels

    EXPECT_EQ(engine.getLogSize(), 2u);
    EXPECT_FALSE(engine.hasBid());
    EXPECT_FALSE(engine.hasAsk());
}

TEST(MarketSellTest, ExceedsAvailableFillsAllDropsRemainder)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Bid, 5, nextID(), 100);
    engine.submitMarketOrder(OrderSide::Ask, 20, nextID()); // wants 20, only 5 exists

    EXPECT_EQ(engine.getLogSize(), 1u);
    EXPECT_FALSE(engine.hasBid());
    EXPECT_FALSE(engine.hasAsk()); // unfilled remainder is dropped, not rested
}

TEST(MarketSellTest, QuantityZeroRejects)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), 100);
    engine.submitMarketOrder(OrderSide::Ask, 0, nextID());

    EXPECT_EQ(engine.getLogSize(), 0u);
    ASSERT_TRUE(engine.hasBid());
    EXPECT_EQ(engine.bestBid().value(), 100); // bid untouched
}

// ─────────────────────────────────────────────────────────────────────────────
// Additional Edge Cases
// ─────────────────────────────────────────────────────────────────────────────

// Bid qty exactly equals ask qty — both sides fully consumed, book empty
TEST(EdgeCaseTest, LimitBidExactMatchLeavesEmptyBook)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), 100);

    EXPECT_EQ(engine.getLogSize(), 1u);
    EXPECT_FALSE(engine.hasAsk());
    EXPECT_FALSE(engine.hasBid());
}

// Ask qty exactly equals bid qty — both sides fully consumed, book empty
TEST(EdgeCaseTest, LimitAskExactMatchLeavesEmptyBook)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 100);

    EXPECT_EQ(engine.getLogSize(), 1u);
    EXPECT_FALSE(engine.hasBid());
    EXPECT_FALSE(engine.hasAsk());
}

// Limit bid price above ask price — crosses and executes at the ask (resting) price
TEST(EdgeCaseTest, LimitBidAboveAskStillCrossesAndExecutes)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), 110); // bids well above ask

    EXPECT_EQ(engine.getLogSize(), 1u);
    EXPECT_FALSE(engine.hasAsk());
    EXPECT_FALSE(engine.hasBid());
}

// Limit ask price below bid price — crosses and executes at the bid (resting) price
TEST(EdgeCaseTest, LimitAskBelowBidStillCrossesAndExecutes)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(),  90); // asks well below bid

    EXPECT_EQ(engine.getLogSize(), 1u);
    EXPECT_FALSE(engine.hasBid());
    EXPECT_FALSE(engine.hasAsk());
}

// Limit bid at exact ask price — at-touch crosses and executes
TEST(EdgeCaseTest, LimitBidAtAskPriceCrosses)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), 100); // bid == ask

    EXPECT_EQ(engine.getLogSize(), 1u);
    EXPECT_FALSE(engine.hasAsk());
    EXPECT_FALSE(engine.hasBid());
}

// Limit ask at exact bid price — at-touch crosses and executes
TEST(EdgeCaseTest, LimitAskAtBidPriceCrosses)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 100); // ask == bid

    EXPECT_EQ(engine.getLogSize(), 1u);
    EXPECT_FALSE(engine.hasBid());
    EXPECT_FALSE(engine.hasAsk());
}

// Book correctly empty after market order drains last resting order
TEST(EdgeCaseTest, MarketBuyDrainsEntireAskSide)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 5, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Ask, 5, nextID(), 102);
    engine.submitLimitOrder(OrderSide::Ask, 5, nextID(), 104);
    engine.submitMarketOrder(OrderSide::Bid, 15, nextID());

    EXPECT_EQ(engine.getLogSize(), 3u);
    EXPECT_FALSE(engine.hasAsk());
}

TEST(EdgeCaseTest, MarketSellDrainsEntireBidSide)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Bid, 5, nextID(), 104);
    engine.submitLimitOrder(OrderSide::Bid, 5, nextID(), 102);
    engine.submitLimitOrder(OrderSide::Bid, 5, nextID(), 100);
    engine.submitMarketOrder(OrderSide::Ask, 15, nextID());

    EXPECT_EQ(engine.getLogSize(), 3u);
    EXPECT_FALSE(engine.hasBid());
}

// ─────────────────────────────────────────────────────────────────────────────
// Cancel Order Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(CancelOrderTest, CancelRestingBidSucceeds)
{
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Bid, 10, id, 100);

    ASSERT_TRUE(engine.hasBid());
    EXPECT_TRUE(engine.cancelOrder(id));
    EXPECT_FALSE(engine.hasBid());
}

TEST(CancelOrderTest, CancelRestingAskSucceeds)
{
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Ask, 10, id, 100);

    ASSERT_TRUE(engine.hasAsk());
    EXPECT_TRUE(engine.cancelOrder(id));
    EXPECT_FALSE(engine.hasAsk());
}

TEST(CancelOrderTest, CancelNonExistentOrderReturnsFalse)
{
    MatchingEngine engine;
    EXPECT_FALSE(engine.cancelOrder(99999));
}

TEST(CancelOrderTest, CancelAlreadyFilledOrderReturnsFalse)
{
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Ask, 10, id, 100);
    engine.submitMarketOrder(OrderSide::Bid, 10, nextID()); // fully fills the ask

    EXPECT_FALSE(engine.hasAsk());
    EXPECT_FALSE(engine.cancelOrder(id)); // already consumed, not in lookup
}

TEST(CancelOrderTest, CancelBestBidPromotesNextLevel)
{
    MatchingEngine engine;
    OrderID bestID = nextID();
    engine.submitLimitOrder(OrderSide::Bid, 10, bestID, 105); // best bid
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(),  100);

    ASSERT_EQ(engine.bestBid().value(), 105);
    engine.cancelOrder(bestID);

    ASSERT_TRUE(engine.hasBid());
    EXPECT_EQ(engine.bestBid().value(), 100);
}

TEST(CancelOrderTest, CancelBestAskPromotesNextLevel)
{
    MatchingEngine engine;
    OrderID bestID = nextID();
    engine.submitLimitOrder(OrderSide::Ask, 10, bestID, 100); // best ask
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 105);

    ASSERT_EQ(engine.bestAsk().value(), 100);
    engine.cancelOrder(bestID);

    ASSERT_TRUE(engine.hasAsk());
    EXPECT_EQ(engine.bestAsk().value(), 105);
}

TEST(CancelOrderTest, CancelOnlyOrderEmptiesBook)
{
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Bid, 10, id, 100);

    engine.cancelOrder(id);

    EXPECT_FALSE(engine.hasBid());
    EXPECT_FALSE(engine.bestBid().has_value());
}

TEST(CancelOrderTest, CancelMiddleOrderInQueuePreservesOthers)
{
    MatchingEngine engine;
    OrderID first  = nextID();
    OrderID middle = nextID();
    OrderID last   = nextID();
    // Three bids at the same price — FIFO queue: first, middle, last
    engine.submitLimitOrder(OrderSide::Bid, 5, first,  100);
    engine.submitLimitOrder(OrderSide::Bid, 5, middle, 100);
    engine.submitLimitOrder(OrderSide::Bid, 5, last,   100);

    engine.cancelOrder(middle);

    // Market sell qty 5 — should consume 'first' (FIFO), leaving only 'last'
    engine.submitMarketOrder(OrderSide::Ask, 5, nextID());
    EXPECT_EQ(engine.getLogSize(), 1u);
    ASSERT_TRUE(engine.hasBid()); // 'last' still resting
}

TEST(CancelOrderTest, CancelDoesNotGenerateTrade)
{
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Bid, 10, id, 100);
    engine.cancelOrder(id);

    EXPECT_EQ(engine.getLogSize(), 0u);
}

TEST(CancelOrderTest, CancelledBidNoLongerMatchesIncomingAsk)
{
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Bid, 10, id, 100);
    engine.cancelOrder(id);

    // Incoming ask at same price — nothing to cross against
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 100);

    EXPECT_EQ(engine.getLogSize(), 0u);
    EXPECT_TRUE(engine.hasAsk());
    EXPECT_FALSE(engine.hasBid());
}

TEST(CancelOrderTest, CancelledAskNoLongerMatchesIncomingBid)
{
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Ask, 10, id, 100);
    engine.cancelOrder(id);

    // Incoming bid at same price — nothing to cross against
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), 100);

    EXPECT_EQ(engine.getLogSize(), 0u);
    EXPECT_TRUE(engine.hasBid());
    EXPECT_FALSE(engine.hasAsk());
}
