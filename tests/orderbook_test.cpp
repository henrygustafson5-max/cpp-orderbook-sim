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

// ─────────────────────────────────────────────────────────────────────────────
// Reduce Order Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(ReduceOrderTest, ReduceBidSucceeds)
{
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Bid, 10, id, 100);
    EXPECT_TRUE(engine.reduceOrder(id, 5));
}

TEST(ReduceOrderTest, ReduceAskSucceeds)
{
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Ask, 10, id, 100);
    EXPECT_TRUE(engine.reduceOrder(id, 5));
}

TEST(ReduceOrderTest, ReduceToZeroFails)
{
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Bid, 10, id, 100);
    EXPECT_FALSE(engine.reduceOrder(id, 0));
    EXPECT_TRUE(engine.hasBid()); // order unchanged
}

TEST(ReduceOrderTest, ReduceToSameQtyFails)
{
    // newQty == current qty is not a reduction
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Bid, 10, id, 100);
    EXPECT_FALSE(engine.reduceOrder(id, 10));
    EXPECT_TRUE(engine.hasBid());
}

TEST(ReduceOrderTest, ReduceAboveCurrentQtyFails)
{
    // Increasing quantity is not allowed
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Bid, 10, id, 100);
    EXPECT_FALSE(engine.reduceOrder(id, 20));
    EXPECT_TRUE(engine.hasBid());
}

TEST(ReduceOrderTest, ReduceNonExistentOrderFails)
{
    MatchingEngine engine;
    EXPECT_FALSE(engine.reduceOrder(99999, 5));
}

TEST(ReduceOrderTest, ReduceDoesNotGenerateTrade)
{
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Bid, 10, id, 100);
    engine.reduceOrder(id, 5);
    EXPECT_EQ(engine.getLogSize(), 0u);
}

TEST(ReduceOrderTest, ReducedQtyIsFilledOnNextMatch)
{
    // Market sell of qty 10 should only fill 5 after reduce — not more
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Bid, 10, id, 100);
    engine.reduceOrder(id, 5);

    engine.submitMarketOrder(OrderSide::Ask, 10, nextID()); // wants 10, only 5 available
    EXPECT_EQ(engine.getLogSize(), 1u);
    EXPECT_FALSE(engine.hasBid()); // reduced bid fully consumed
}

TEST(ReduceOrderTest, ReduceDoesNotChangePriceLevel)
{
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Bid, 10, id, 100);
    engine.reduceOrder(id, 3);
    ASSERT_TRUE(engine.bestBid().has_value());
    EXPECT_EQ(engine.bestBid().value(), 100);
}

TEST(ReduceOrderTest, ReducePreservesFIFOPosition)
{
    // Two bids at the same price. Reduce the first. A market sell of exactly
    // the reduced qty should consume the first (FIFO head), leaving the second.
    MatchingEngine engine;
    OrderID first  = nextID();
    OrderID second = nextID();
    engine.submitLimitOrder(OrderSide::Bid, 10, first,  100);
    engine.submitLimitOrder(OrderSide::Bid, 10, second, 100);
    engine.reduceOrder(first, 3);

    // Market sell 3 — must hit 'first' (FIFO), consuming it entirely
    engine.submitMarketOrder(OrderSide::Ask, 3, nextID());
    EXPECT_EQ(engine.getLogSize(), 1u);
    ASSERT_TRUE(engine.hasBid());

    // Market sell 10 — must now hit 'second'
    engine.submitMarketOrder(OrderSide::Ask, 10, nextID());
    EXPECT_EQ(engine.getLogSize(), 2u);
    EXPECT_FALSE(engine.hasBid());
}

TEST(ReduceOrderTest, ReduceBidToOneSucceeds)
{
    // Boundary: reduce to qty 1 (minimum valid)
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Bid, 100, id, 100);
    EXPECT_TRUE(engine.reduceOrder(id, 1));

    engine.submitMarketOrder(OrderSide::Ask, 5, nextID()); // only 1 available
    EXPECT_EQ(engine.getLogSize(), 1u);
    EXPECT_FALSE(engine.hasBid());
}

TEST(ReduceOrderTest, ReducePartiallyFilledOrder)
{
    // Partially fill a bid (10 → 7 remaining), then reduce the remainder to 4
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Bid, 10, id, 100);
    engine.submitMarketOrder(OrderSide::Ask, 3, nextID()); // 3 filled, 7 remaining
    ASSERT_EQ(engine.getLogSize(), 1u);
    ASSERT_TRUE(engine.hasBid());

    EXPECT_TRUE(engine.reduceOrder(id, 4));

    // Market sell 10 — should only fill 4
    engine.submitMarketOrder(OrderSide::Ask, 10, nextID());
    EXPECT_EQ(engine.getLogSize(), 2u);
    EXPECT_FALSE(engine.hasBid());
}

TEST(ReduceOrderTest, ReducePartiallyFilledOrderAboveRemainingFails)
{
    // 7 units remain after a partial fill; reducing to 8 (> remaining) must fail
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Bid, 10, id, 100);
    engine.submitMarketOrder(OrderSide::Ask, 3, nextID()); // 7 remaining

    EXPECT_FALSE(engine.reduceOrder(id, 8));
}

TEST(ReduceOrderTest, ReduceAskPreservesBestAskPrice)
{
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Ask, 20, id, 105);
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 110);
    engine.reduceOrder(id, 5);
    EXPECT_EQ(engine.bestAsk().value(), 105); // best ask price unchanged
}

// ─────────────────────────────────────────────────────────────────────────────
// Cancel Replace Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(CancelReplaceTest, ReplaceBidSucceeds)
{
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Bid, 10, id, 100);
    EXPECT_TRUE(engine.cancelReplace(id, 10, 105));
}

TEST(CancelReplaceTest, ReplaceAskSucceeds)
{
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Ask, 10, id, 110);
    EXPECT_TRUE(engine.cancelReplace(id, 10, 105));
}

TEST(CancelReplaceTest, NonExistentOrderFails)
{
    MatchingEngine engine;
    EXPECT_FALSE(engine.cancelReplace(99999, 10, 100));
}

TEST(CancelReplaceTest, OldPriceLevelRemovedWhenAlone)
{
    // Single bid at 100 replaced with 105 — old level must disappear
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Bid, 10, id, 100);
    engine.cancelReplace(id, 10, 105);

    ASSERT_TRUE(engine.hasBid());
    EXPECT_EQ(engine.bestBid().value(), 105);
}

TEST(CancelReplaceTest, NewOrderRestsAtNewPrice)
{
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Bid, 10, id, 100);
    engine.cancelReplace(id, 10, 95); // lower price, no ask to cross
    ASSERT_TRUE(engine.hasBid());
    EXPECT_EQ(engine.bestBid().value(), 95);
}

TEST(CancelReplaceTest, NoTradeWhenNewPriceDoesNotCross)
{
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 110);
    engine.submitLimitOrder(OrderSide::Bid, 10, id, 100);
    engine.cancelReplace(id, 10, 105); // new bid 105 still below ask 110

    EXPECT_EQ(engine.getLogSize(), 0u);
    EXPECT_TRUE(engine.hasBid());
    EXPECT_TRUE(engine.hasAsk());
    EXPECT_EQ(engine.bestBid().value(), 105);
}

TEST(CancelReplaceTest, CrossingNewPriceExecutesTrade)
{
    // Ask rests at 100. Bid at 90 doesn't cross. Replace bid at 100 — crosses.
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Bid, 10, id, 90);
    engine.cancelReplace(id, 10, 100);

    EXPECT_EQ(engine.getLogSize(), 1u);
    EXPECT_FALSE(engine.hasBid());
    EXPECT_FALSE(engine.hasAsk());
}

TEST(CancelReplaceTest, CrossingNewPricePartiallyExecutes)
{
    // Ask qty 5 at 100. Bid qty 10 at 90. Replace bid qty 10 at 100 — partial fill.
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Ask, 5, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Bid, 10, id, 90);
    engine.cancelReplace(id, 10, 100);

    EXPECT_EQ(engine.getLogSize(), 1u);
    EXPECT_FALSE(engine.hasAsk());       // ask fully consumed
    EXPECT_TRUE(engine.hasBid());        // 5 units of new bid rest
    EXPECT_EQ(engine.bestBid().value(), 100);
}

TEST(CancelReplaceTest, PreservesOtherOrdersAtOldPrice)
{
    // Two bids at 100. Replace the first — the second stays at 100.
    MatchingEngine engine;
    OrderID first  = nextID();
    OrderID second = nextID();
    engine.submitLimitOrder(OrderSide::Bid, 10, first,  100);
    engine.submitLimitOrder(OrderSide::Bid, 10, second, 100);
    engine.cancelReplace(first, 10, 105);

    ASSERT_TRUE(engine.hasBid());
    EXPECT_EQ(engine.bestBid().value(), 105);

    // 'second' at 100 still present — market sell 10 fills it
    engine.submitMarketOrder(OrderSide::Ask, 10, nextID());
    EXPECT_EQ(engine.getLogSize(), 1u);
    EXPECT_TRUE(engine.hasBid()); // replaced 'first' at 105 still resting
}

TEST(CancelReplaceTest, LosesPriorityAtSamePrice)
{
    // Two bids at 100 (first, second). Replace 'first' at the same price 100.
    // 'second' becomes FIFO head; replaced 'first' goes to the back.
    MatchingEngine engine;
    OrderID first  = nextID();
    OrderID second = nextID();
    engine.submitLimitOrder(OrderSide::Bid, 5, first,  100);
    engine.submitLimitOrder(OrderSide::Bid, 5, second, 100);
    engine.cancelReplace(first, 5, 100); // same price — loses priority

    // Market sell 5 fills 'second' (now FIFO head)
    engine.submitMarketOrder(OrderSide::Ask, 5, nextID());
    EXPECT_EQ(engine.getLogSize(), 1u);
    ASSERT_TRUE(engine.hasBid()); // replaced 'first' (now last) still resting
}

TEST(CancelReplaceTest, LosesPriorityBehindExistingAtNewPrice)
{
    // Bid A at 100, Bid B at 105. Replace A to 105 — A goes behind B at 105.
    MatchingEngine engine;
    OrderID a = nextID();
    OrderID b = nextID();
    engine.submitLimitOrder(OrderSide::Bid, 5, a, 100);
    engine.submitLimitOrder(OrderSide::Bid, 5, b, 105);
    engine.cancelReplace(a, 5, 105); // A moves to 105, goes behind B

    // Market sell 5: should fill B (FIFO head at 105), not A
    engine.submitMarketOrder(OrderSide::Ask, 5, nextID());
    EXPECT_EQ(engine.getLogSize(), 1u);
    ASSERT_TRUE(engine.hasBid()); // A (replaced) still resting at 105
    EXPECT_EQ(engine.bestBid().value(), 105);
}

TEST(CancelReplaceTest, DoesNotGenerateTradeForCancellation)
{
    // Replace bid with a lower non-crossing price — zero trades total
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Bid, 10, id, 100);
    engine.cancelReplace(id, 10, 90);
    EXPECT_EQ(engine.getLogSize(), 0u);
}

TEST(CancelReplaceTest, ReplaceWithNewQuantity)
{
    // Replaced order carries the new quantity
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Ask, 10, id, 100);
    engine.cancelReplace(id, 25, 105); // new qty 25

    engine.submitMarketOrder(OrderSide::Bid, 25, nextID());
    EXPECT_EQ(engine.getLogSize(), 1u);
    EXPECT_FALSE(engine.hasAsk());
}

TEST(CancelReplaceTest, ZeroNewQtyCancelsOldWithoutRestingNewOrder)
{
    // submitLimitOrder silently rejects qty==0; cancelReplace still returns true,
    // but the old order is gone and no new order is resting.
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Bid, 10, id, 100);
    EXPECT_TRUE(engine.cancelReplace(id, 0, 100));
    EXPECT_FALSE(engine.hasBid());
    EXPECT_EQ(engine.getLogSize(), 0u);
}

TEST(CancelReplaceTest, ZeroNewPriceCancelsOldWithoutRestingNewOrder)
{
    // submitLimitOrder silently rejects price==0; same behaviour as above.
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Bid, 10, id, 100);
    EXPECT_TRUE(engine.cancelReplace(id, 10, 0));
    EXPECT_FALSE(engine.hasBid());
    EXPECT_EQ(engine.getLogSize(), 0u);
}

TEST(CancelReplaceTest, ReplacePartiallyFilledOrder)
{
    // Partially fill a bid, then cancel-replace it. The remaining qty is
    // cancelled; a fresh new order rests at the new price.
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Bid, 10, id, 100);
    engine.submitMarketOrder(OrderSide::Ask, 3, nextID()); // 3 filled, 7 remaining
    ASSERT_EQ(engine.getLogSize(), 1u);

    EXPECT_TRUE(engine.cancelReplace(id, 10, 105));
    EXPECT_EQ(engine.getLogSize(), 1u); // replace itself generates no trade
    ASSERT_TRUE(engine.hasBid());
    EXPECT_EQ(engine.bestBid().value(), 105);
}

TEST(CancelReplaceTest, AskSideUnaffectedWhenBidReplacedToBelowAsk)
{
    // Replacing a bid to a price that still doesn't cross the ask
    // must leave the ask side completely untouched.
    MatchingEngine engine;
    OrderID id = nextID();
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 110);
    engine.submitLimitOrder(OrderSide::Bid, 10, id, 100);
    engine.cancelReplace(id, 10, 108); // still below ask of 110

    EXPECT_TRUE(engine.hasAsk());
    EXPECT_EQ(engine.bestAsk().value(), 110);
    EXPECT_TRUE(engine.hasBid());
    EXPECT_EQ(engine.bestBid().value(), 108);
    EXPECT_EQ(engine.getLogSize(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// IOC (Immediate-Or-Cancel) Bid Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(IOCBidTest, FullFillDoesNotRest)
{
    // IOC bid qty 10 against ask qty 10 — fully fills, never rests
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), 100, LimitType::IOC);

    EXPECT_EQ(engine.getLogSize(), 1u);
    EXPECT_FALSE(engine.hasAsk());
    EXPECT_FALSE(engine.hasBid()); // fully filled — not resting
}

TEST(IOCBidTest, PartialFillRemainderCancelled)
{
    // IOC bid qty 10 against ask qty 5 — partially fills, remainder silently dropped
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 5, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), 100, LimitType::IOC);

    EXPECT_EQ(engine.getLogSize(), 1u);
    EXPECT_FALSE(engine.hasAsk()); // ask fully consumed
    EXPECT_FALSE(engine.hasBid()); // unfilled remainder not rested
}

TEST(IOCBidTest, NoLiquidityCancelledImmediately)
{
    // No asks — IOC bid is killed immediately with no trade
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), 100, LimitType::IOC);

    EXPECT_EQ(engine.getLogSize(), 0u);
    EXPECT_FALSE(engine.hasBid());
}

TEST(IOCBidTest, PriceBelowBestAskCancelledWithNoFill)
{
    // Ask at 105; IOC bid at 100 — price doesn't cross, order immediately killed
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 105);
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), 100, LimitType::IOC);

    EXPECT_EQ(engine.getLogSize(), 0u);
    EXPECT_FALSE(engine.hasBid());
    EXPECT_TRUE(engine.hasAsk()); // resting ask untouched
}

TEST(IOCBidTest, SpansMultipleLevelsFilledCompletely)
{
    // IOC bid qty 10 spans two ask levels (5 + 5) — fills all, does not rest
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 5, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Ask, 5, nextID(), 101);
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), 102, LimitType::IOC);

    EXPECT_EQ(engine.getLogSize(), 2u);
    EXPECT_FALSE(engine.hasAsk());
    EXPECT_FALSE(engine.hasBid());
}

TEST(IOCBidTest, SpansMultipleLevelsPartialFillRemainderCancelled)
{
    // IOC bid qty 15 can only fill 10 (5+5); remaining 5 dropped
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 5, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Ask, 5, nextID(), 101);
    engine.submitLimitOrder(OrderSide::Bid, 15, nextID(), 102, LimitType::IOC);

    EXPECT_EQ(engine.getLogSize(), 2u);
    EXPECT_FALSE(engine.hasAsk());
    EXPECT_FALSE(engine.hasBid()); // 5 unfilled units not rested
}

TEST(IOCBidTest, DoesNotInterfereWithRestingGTCOrders)
{
    // A resting GTC bid should be completely unaffected by a separate IOC bid
    MatchingEngine engine;
    OrderID gtcID = nextID();
    engine.submitLimitOrder(OrderSide::Bid, 10, gtcID, 95); // GTC rests
    engine.submitLimitOrder(OrderSide::Ask,  5, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Bid,  5, nextID(), 100, LimitType::IOC); // IOC fills

    EXPECT_EQ(engine.getLogSize(), 1u);
    EXPECT_TRUE(engine.hasBid()); // GTC order still resting
    EXPECT_EQ(engine.bestBid().value(), 95);
}

// ─────────────────────────────────────────────────────────────────────────────
// IOC (Immediate-Or-Cancel) Ask Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(IOCAskTest, FullFillDoesNotRest)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 100, LimitType::IOC);

    EXPECT_EQ(engine.getLogSize(), 1u);
    EXPECT_FALSE(engine.hasBid());
    EXPECT_FALSE(engine.hasAsk());
}

TEST(IOCAskTest, PartialFillRemainderCancelled)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Bid, 5, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 100, LimitType::IOC);

    EXPECT_EQ(engine.getLogSize(), 1u);
    EXPECT_FALSE(engine.hasBid());
    EXPECT_FALSE(engine.hasAsk());
}

TEST(IOCAskTest, NoLiquidityCancelledImmediately)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 100, LimitType::IOC);

    EXPECT_EQ(engine.getLogSize(), 0u);
    EXPECT_FALSE(engine.hasAsk());
}

TEST(IOCAskTest, PriceAboveBestBidCancelledWithNoFill)
{
    // Bid at 95; IOC ask at 100 — doesn't cross, killed immediately
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), 95);
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 100, LimitType::IOC);

    EXPECT_EQ(engine.getLogSize(), 0u);
    EXPECT_FALSE(engine.hasAsk());
    EXPECT_TRUE(engine.hasBid()); // resting bid untouched
}

TEST(IOCAskTest, SpansMultipleBidLevelsFilledCompletely)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Bid, 5, nextID(), 101); // best bid
    engine.submitLimitOrder(OrderSide::Bid, 5, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 99, LimitType::IOC);

    EXPECT_EQ(engine.getLogSize(), 2u);
    EXPECT_FALSE(engine.hasBid());
    EXPECT_FALSE(engine.hasAsk());
}

// ─────────────────────────────────────────────────────────────────────────────
// FOK (Fill-Or-Kill) Bid Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(FOKBidTest, ExactVolumeAvailableExecutes)
{
    // Ask qty 10 at 100; FOK bid qty 10 at 100 — exact match, executes fully
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), 100, LimitType::FOK);

    EXPECT_EQ(engine.getLogSize(), 1u);
    EXPECT_FALSE(engine.hasAsk());
    EXPECT_FALSE(engine.hasBid());
}

TEST(FOKBidTest, InsufficientVolumeKilledNoTrade)
{
    // Ask qty 5 at 100; FOK bid qty 10 — not enough volume, entire order killed
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 5, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), 100, LimitType::FOK);

    EXPECT_EQ(engine.getLogSize(), 0u); // no partial fills
    EXPECT_TRUE(engine.hasAsk());       // resting ask untouched
    EXPECT_FALSE(engine.hasBid());
}

TEST(FOKBidTest, NoLiquidityKilledImmediately)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), 100, LimitType::FOK);

    EXPECT_EQ(engine.getLogSize(), 0u);
    EXPECT_FALSE(engine.hasBid());
}

TEST(FOKBidTest, PriceBelowBestAskKilledNoTrade)
{
    // Ask at 105; FOK bid at 100 — price doesn't cross, killed immediately
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 105);
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), 100, LimitType::FOK);

    EXPECT_EQ(engine.getLogSize(), 0u);
    EXPECT_FALSE(engine.hasBid());
    EXPECT_TRUE(engine.hasAsk());
    EXPECT_EQ(engine.bestAsk().value(), 105);
}

TEST(FOKBidTest, SpansMultipleLevelsSufficientVolumeExecutes)
{
    // Two ask levels: 5 at 100, 5 at 101. FOK bid qty 10 at 102 — enough total, executes.
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 5, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Ask, 5, nextID(), 101);
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), 102, LimitType::FOK);

    EXPECT_EQ(engine.getLogSize(), 2u);
    EXPECT_FALSE(engine.hasAsk());
    EXPECT_FALSE(engine.hasBid());
}

TEST(FOKBidTest, SpansMultipleLevelsInsufficientVolumeKilled)
{
    // Two ask levels: 5 at 100, 5 at 101. FOK bid qty 15 — only 10 crossable, killed.
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 5, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Ask, 5, nextID(), 101);
    engine.submitLimitOrder(OrderSide::Bid, 15, nextID(), 102, LimitType::FOK);

    EXPECT_EQ(engine.getLogSize(), 0u); // no partial fills — FOK is atomic
    EXPECT_TRUE(engine.hasAsk());       // resting asks untouched
    EXPECT_FALSE(engine.hasBid());
}

TEST(FOKBidTest, VolumeAvailableButPriceOutOfRangeKilled)
{
    // Ask 10 at 100, ask 10 at 110. FOK bid qty 20 at 105 — second level beyond price, killed.
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 110);
    engine.submitLimitOrder(OrderSide::Bid, 20, nextID(), 105, LimitType::FOK);

    EXPECT_EQ(engine.getLogSize(), 0u);
    EXPECT_TRUE(engine.hasAsk());
    EXPECT_FALSE(engine.hasBid());
}

TEST(FOKBidTest, DoesNotRestAfterKill)
{
    // Ensuring a killed FOK order never ends up resting in the book
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), 100, LimitType::FOK); // no asks

    EXPECT_FALSE(engine.hasBid());
    EXPECT_FALSE(engine.bestBid().has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// FOK (Fill-Or-Kill) Ask Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(FOKAskTest, ExactVolumeAvailableExecutes)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 100, LimitType::FOK);

    EXPECT_EQ(engine.getLogSize(), 1u);
    EXPECT_FALSE(engine.hasBid());
    EXPECT_FALSE(engine.hasAsk());
}

TEST(FOKAskTest, InsufficientVolumeKilledNoTrade)
{
    // Bid qty 5 at 100; FOK ask qty 10 — not enough bids, killed
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Bid, 5, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 100, LimitType::FOK);

    EXPECT_EQ(engine.getLogSize(), 0u);
    EXPECT_TRUE(engine.hasBid()); // resting bid untouched
    EXPECT_FALSE(engine.hasAsk());
}

TEST(FOKAskTest, NoLiquidityKilledImmediately)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 100, LimitType::FOK);

    EXPECT_EQ(engine.getLogSize(), 0u);
    EXPECT_FALSE(engine.hasAsk());
}

TEST(FOKAskTest, PriceAboveBestBidKilledNoTrade)
{
    // Bid at 95; FOK ask at 100 — doesn't cross, killed
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Bid, 10, nextID(), 95);
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 100, LimitType::FOK);

    EXPECT_EQ(engine.getLogSize(), 0u);
    EXPECT_FALSE(engine.hasAsk());
    EXPECT_TRUE(engine.hasBid());
    EXPECT_EQ(engine.bestBid().value(), 95);
}

TEST(FOKAskTest, SpansMultipleBidLevelsSufficientVolumeExecutes)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Bid, 5, nextID(), 101);
    engine.submitLimitOrder(OrderSide::Bid, 5, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 99, LimitType::FOK);

    EXPECT_EQ(engine.getLogSize(), 2u);
    EXPECT_FALSE(engine.hasBid());
    EXPECT_FALSE(engine.hasAsk());
}

TEST(FOKAskTest, SpansMultipleBidLevelsInsufficientVolumeKilled)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Bid, 5, nextID(), 101);
    engine.submitLimitOrder(OrderSide::Bid, 5, nextID(), 100);
    engine.submitLimitOrder(OrderSide::Ask, 15, nextID(), 99, LimitType::FOK);

    EXPECT_EQ(engine.getLogSize(), 0u);
    EXPECT_TRUE(engine.hasBid()); // bids untouched
    EXPECT_FALSE(engine.hasAsk());
}

TEST(FOKAskTest, DoesNotRestAfterKill)
{
    MatchingEngine engine;
    engine.submitLimitOrder(OrderSide::Ask, 10, nextID(), 100, LimitType::FOK); // no bids

    EXPECT_FALSE(engine.hasAsk());
    EXPECT_FALSE(engine.bestAsk().has_value());
}
