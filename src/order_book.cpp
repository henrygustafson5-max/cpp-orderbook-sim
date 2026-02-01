#include "order_book.hpp"
#include "order.hpp"
#include <cassert>
#include <optional>


    

    void OrderBook::addBid(std::unique_ptr<LimitOrder> order)
    {
       const Price price = order->getPrice();
        m_BidSide[price].push_back(std::move(order));
    }

    void OrderBook::addAsk(std::unique_ptr<LimitOrder> order)
    {
        const Price price = order->getPrice();
        m_AskSide[price].push_back(std::move(order));
    }
   
    bool OrderBook::hasAsks() const
    {
        return !m_AskSide.empty();
    }

    bool OrderBook::hasBids() const
    {
        return !m_BidSide.empty();
    }

  

    std::optional<Price> OrderBook::bestBid() const 
    {
        if (m_BidSide.empty()) return std::nullopt; 
        assert(!m_BidSide.begin()->second.empty()); 
        return m_BidSide.begin()->first; 
    }

    std::optional<Price> OrderBook::bestAsk() const
    {
        if (m_AskSide.empty()) return std::nullopt; 
        assert(!m_AskSide.begin()->second.empty());
        return m_AskSide.begin()->first;
    }

    std::optional<ExecutionReport> OrderBook::consumeBestAsk(Quantity quantity)
    {   
        if (m_AskSide.empty()) return std::nullopt;
        auto priceIt = m_AskSide.begin();
        auto& orderQueue = priceIt->second; 
        if (orderQueue.empty()) return std::nullopt;
        LimitOrder& restingOrder = *orderQueue.front(); 
        Quantity executed = std::min(quantity, restingOrder.getQuantity());
        if(executed == 0) return std::nullopt;
        restingOrder.updateQuantity(executed);
        OrderID rID{restingOrder.getOrderID()};
        Price rPrice{restingOrder.getPrice()};
        if (restingOrder.getQuantity() == 0)
        {
            orderQueue.pop_front(); 
        }
        if (orderQueue.empty())
        {
            m_AskSide.erase(priceIt);
        }
        return ExecutionReport{rPrice, rID, executed}; 

    }
    std::optional<ExecutionReport> OrderBook::consumeBestBid(Quantity quantity)
    {
        if (m_BidSide.empty()) return std::nullopt;
        auto priceIt = m_BidSide.begin();
        auto& orderQueue = priceIt->second; 
        if (orderQueue.empty()) return std::nullopt;
        LimitOrder& restingOrder = *orderQueue.front();
        Quantity executed = std::min(quantity, restingOrder.getQuantity());
        if (executed == 0) return std::nullopt;
        restingOrder.updateQuantity(executed);
        OrderID rID{restingOrder.getOrderID()};
        Price rPrice{restingOrder.getPrice()};
        if (restingOrder.getQuantity() == 0)
        {
            orderQueue.pop_front();
        }
        if (orderQueue.empty())
        {
            m_BidSide.erase(priceIt);
        }
        return ExecutionReport{rPrice, rID, executed}; 
    }

