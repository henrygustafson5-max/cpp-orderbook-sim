#include "matching_engine.hpp"
#include "order.hpp"




int main()
{
    MatchingEngine engine;

    engine.submitLimitOrder(OrderSide::Ask, 100, OrderIDGenerator::next(), 101);
    engine.submitLimitOrder(OrderSide::Ask, 100, OrderIDGenerator::next(), 102);

    engine.submitLimitOrder(OrderSide::Bid, 100, OrderIDGenerator::next(), 99);
    engine.submitLimitOrder(OrderSide::Bid, 100, OrderIDGenerator::next(), 98);

    // --- Aggress ---
    engine.submitMarketOrder(OrderSide::Bid, 150, OrderIDGenerator::next());
    engine.submitMarketOrder(OrderSide::Ask, 50, OrderIDGenerator::next());

    for (std::size_t index {0} ; index< engine.getLogSize() ; index++ )
    {
        engine.printTrade(index);
    }
    return 0;

}