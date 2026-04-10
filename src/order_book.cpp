#include "order_book.hpp"
#include "order.hpp"
#include <optional>


    

    void OrderBook::addBid(std::unique_ptr<LimitOrder> order)
    {
       const Price price = order->getPrice();
       const OrderID id = order->getOrderID();
       const Quantity qty = order->getQuantity(); 
       auto [levelIT, inserted] = m_BidSide.try_emplace(price);
       auto& level = levelIT->second; 
       auto orderIT = level.orders.insert(level.orders.end(), std::move(order));
       level.levelQTY += qty;
       m_lookup[id] =  LookUp{OrderSide::Bid, orderIT, price,qty };
    }
  
    void OrderBook::addAsk(std::unique_ptr<LimitOrder> order)
    { 
       const Price price = order->getPrice();
       const OrderID id = order->getOrderID();
       const Quantity qty = order->getQuantity(); 
       auto [levelIT, inserted] = m_AskSide.try_emplace(price);
       auto& level = levelIT->second; 
       auto orderIT = level.orders.insert(level.orders.end(), std::move(order));
       level.levelQTY += qty;
       m_lookup[id] =  LookUp{OrderSide::Ask, orderIT, price,qty}; 
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
        if (m_BidSide.begin()->second.orders.empty()) return std::nullopt;
        return m_BidSide.begin()->first;
    }

    std::optional<Price> OrderBook::bestAsk() const
    {
        if (m_AskSide.empty()) return std::nullopt;
        if (m_AskSide.begin()->second.orders.empty()) return std::nullopt;
        return m_AskSide.begin()->first;
    }

    std::optional<ExecutionReport> OrderBook::consumeBestAsk(Quantity quantity)
    {   
        if (m_AskSide.empty()) return std::nullopt;
        auto priceIt = m_AskSide.begin();
        auto& orderQueue = priceIt->second.orders; 
        if (orderQueue.empty()) return std::nullopt;
        LimitOrder& restingOrder = *orderQueue.front(); 
        Quantity executed = std::min(quantity, restingOrder.getQuantity());
        if(executed == 0) return std::nullopt;
        restingOrder.updateQuantity(executed);
        OrderID rID{restingOrder.getOrderID()};
        Price rPrice{restingOrder.getPrice()};
        if (restingOrder.getQuantity() == 0)
        {
            m_lookup.erase(rID);
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
        auto& orderQueue = priceIt->second.orders;
        if (orderQueue.empty()) return std::nullopt;
        LimitOrder& restingOrder = *orderQueue.front();
        Quantity executed = std::min(quantity, restingOrder.getQuantity());
        if (executed == 0) return std::nullopt;
        restingOrder.updateQuantity(executed);
        OrderID rID{restingOrder.getOrderID()};
        Price rPrice{restingOrder.getPrice()};
        if (restingOrder.getQuantity() == 0)
        {
            m_lookup.erase(rID);
            orderQueue.pop_front();
        }
        if (orderQueue.empty())
        {
            m_BidSide.erase(priceIt);
        }
        return ExecutionReport{rPrice, rID, executed};
    }


    bool OrderBook::orderExists(OrderID id)
    {
        if(m_lookup.contains(id))
        {
          return true;
        }
        return false;
    }
    
    LookUp OrderBook::infoFromID(OrderID id)
    {
      return m_lookup[id];
    }
  
    bool OrderBook::FOKVolumeCheck(OrderSide side, Price price, Quantity volume)
    {
        if(side == OrderSide::Bid) {
         Quantity restingQTY{};
        for(const auto& pair : m_AskSide) 
        {
            if(pair.first > price) return false;
            restingQTY += pair.second.levelQTY;
            if(restingQTY >= volume) return true;
        }
        return false;
    }
        else { 
         Quantity restingQTY{};
        for(const auto& pair : m_BidSide) 
        {
            if(pair.first < price) return false;
            restingQTY += pair.second.levelQTY;
            if(restingQTY >= volume) return true;
        }
        return false;
     }        
    }
    void OrderBook::cancelOrder(OrderID id)
    {    
       LookUp info = m_lookup[id]; 
       Price price = info.price;
       const auto& order = *info.orderIT;
       Quantity removingQty = order->getQuantity();
       if(info.side == OrderSide::Bid)
       {   
        auto mapIt = m_BidSide.find(price);
        mapIt->second.levelQTY -= removingQty;
        mapIt->second.orders.erase(info.orderIT);
        if(mapIt->second.levelQTY == 0 ) m_BidSide.erase(price); 
       }
      else 
      {   
        auto mapIt = m_AskSide.find(price);
        mapIt->second.levelQTY -= removingQty;
        mapIt->second.orders.erase(info.orderIT);
        if(mapIt->second.levelQTY == 0 ) m_AskSide.erase(price); 
      }
    }
     
    void OrderBook::reduceQuantity(OrderID id, Quantity newQTY)
    {
        LookUp info = m_lookup[id];
        auto const& order = *info.orderIT;
        order->setQuantity(newQTY); 
      
    }

           
