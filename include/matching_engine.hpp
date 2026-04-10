#pragma once
#include "order.hpp"
#include "order_book.hpp"
#include "trade.hpp"



class MatchingEngine
{
    private: 
    OrderBook book; 
    TradeLog tradelog;  
    TradeID id {0}; 

    void fillAndRestLimitBid(std::unique_ptr<LimitOrder> limitOrder);

    void fillAndRestLimitAsk(std::unique_ptr<LimitOrder> limitOrder);

    void fillMarketOrder(OrderSide marketSide, Quantity marketQty, OrderID marketID); 
    
    public:

    void submitLimitOrder(OrderSide orderSide, Quantity quantity, OrderID orderID, Price price, LimitType type = LimitType::GTC);

    void submitMarketOrder( OrderSide side, Quantity quantity, OrderID id);

    void printTrade(std::size_t index) const;

    std::size_t getLogSize() const ;

    std::optional<Price> bestBid() const;

    std::optional<Price> bestAsk() const;
    
    bool FOKVolumeCheck(OrderSide side, Price price, Quantity volume);
    bool requestModify(OrderID id);

    bool cancelOrder(OrderID id);
    
    bool reduceOrder(OrderID id, Quantity newQty);

    bool cancelReplace(OrderID id, Quantity newQTY, Price newPrice);    

    bool hasAsk() const;

    bool hasBid() const;
};


