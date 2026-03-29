#pragma once
#include "order_book.hpp"
#include "trade.hpp"

class MatchingEngine
{
    private: 
    OrderBook book; 
    TradeLog tradelog;  
    TradeID id {0}; 

    void fillAndRestLimitOrder(std::unique_ptr<LimitOrder> limitOrder);

    void fillMarketOrder(OrderSide marketSide, Quantity marketQty, OrderID marketID); 

    bool cancelOrder(OrderID id, const OrderInfo& info);
    
    public:

    void submitLimitOrder(OrderSide orderSide, Quantity quantity, OrderID orderID, Price price);

    void submitMarketOrder( OrderSide side, Quantity quantity, OrderID id);

    void printTrade(std::size_t index) const;

    std::size_t getLogSize() const ;

    std::optional<Price> bestBid() const;

    std::optional<Price> bestAsk() const;

    bool requestCancel(OrderID id);

    bool hasAsk() const;

    bool hasBid() const;
};