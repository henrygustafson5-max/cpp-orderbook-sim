#pragma once 
#include "order.hpp"
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include <list>

struct ExecutionReport
{
    Price restingPrice;
    OrderID restingID;
    Quantity executedQTY; 
};

struct PriceLevel 
{
  std::list<std::unique_ptr<LimitOrder>> orders;
  Quantity levelQTY;
};
struct LookUp
{
    OrderSide side;
    std::list<std::unique_ptr<LimitOrder>>::iterator orderIT;
    Price price;
    Quantity qty;
};

class OrderBook 
{
  
    private:
    std::map<Price, PriceLevel, std::greater<Price>> m_BidSide;
    std::map<Price, PriceLevel, std::less<Price>> m_AskSide; 
    std::unordered_map<OrderID, LookUp> m_lookup;

    public:

    void addBid(std::unique_ptr<LimitOrder> order) ;

    void addAsk(std::unique_ptr<LimitOrder> order) ; 

    bool hasAsks() const ;

    bool hasBids() const ;

    std::optional<Price> bestBid() const;

    std::optional<Price> bestAsk() const;

    std::optional<ExecutionReport> consumeBestAsk(Quantity quantity);

    std::optional<ExecutionReport> consumeBestBid(Quantity quantity);
    
    LookUp infoFromID(OrderID id);

    bool orderExists(OrderID id);

    void cancelOrder(OrderID id);
          
    void reduceQuantity(OrderID id, Quantity newQTY);
};
