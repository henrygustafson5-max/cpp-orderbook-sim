#pragma once 
#include "order.hpp"
#include <map>
#include <deque> 
#include <memory>
#include <optional>

using Queue = std::deque<std::unique_ptr<LimitOrder>>; 
struct ExecutionReport
{
    Price restingPrice;
    OrderID restingID;
    Quantity executedQTY; 
};

class OrderBook 
{
    private:
    std::map<Price, Queue, std::greater<Price>> m_BidSide;
    std::map<Price, Queue, std::less<Price>> m_AskSide; 
    
    public:

    void addBid(std::unique_ptr<LimitOrder> order) ;

    void addAsk(std::unique_ptr<LimitOrder> order) ; 

    bool hasAsks() const ;

    bool hasBids() const ;

    std::optional<Price> bestBid() const;

    std::optional<Price> bestAsk() const;

    std::optional<ExecutionReport> consumeBestAsk(Quantity quantity);

    std::optional<ExecutionReport> consumeBestBid(Quantity quantity);

};