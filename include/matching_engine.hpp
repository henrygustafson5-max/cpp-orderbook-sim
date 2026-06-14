#pragma once
#include "order.hpp"
#include "order_book.hpp"
#include "trade.hpp"
#include <unordered_map> 
#include <string>

using SymbolID = size_t; 
struct MatchingEngine
{
    std::vector<OrderBook> book; 
    TradeLog tradelog;  
    TradeID id {0}; 
    std::unordered_map<SymbolID, std::string> symbolLookup; 
    std::unordered_map<OrderID, SymbolID> idToSymbol;
  
    MatchingEngine(size_t numberofsymbols); 

    MatchingEngine();

    void fillAndRestLimitBid(SymbolID ticker, std::unique_ptr<LimitOrder> limitOrder);

    void fillAndRestLimitAsk(SymbolID ticker, std::unique_ptr<LimitOrder> limitOrder);

    void fillMarketOrder(SymbolID ticker, OrderSide marketSide, Quantity marketQty, OrderID marketID); 
    
    void submitLimitOrder(SymbolID ticker, OrderSide orderSide, Quantity quantity, OrderID orderID, Price price, LimitType type = LimitType::GTC);

    void submitMarketOrder(SymbolID ticker, OrderSide side, Quantity quantity, OrderID id);

    void printTrade(std::size_t index) const;

    std::size_t getLogSize() const ;

    std::optional<Price> bestBid(SymbolID ticker) const;

    std::optional<Price> bestAsk(SymbolID ticker) const;
       
    bool FOKVolumeCheck(SymbolID ticker, OrderSide side, Price price, Quantity volume);

    std::optional<SymbolID> requestModify(OrderID id);
     
    bool cancelOrder(OrderID id);
    
    bool reduceOrder(OrderID id, Quantity newQty);

    bool cancelReplace(OrderID id, Quantity newQTY, Price newPrice);    

    bool hasAsk(SymbolID ticker) const;

    bool hasBid(SymbolID ticker) const;
};


