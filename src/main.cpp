#include <cstdint> 
#include <map>
#include <deque>
#include <chrono> 
#include <vector>
#include <algorithm>
#include <optional> 

using Price = int32_t;
using Quantity = uint32_t;
using OrderID = int32_t;
using TradeID = std::uint64_t; 
using Clock = std::chrono::steady_clock;
using Timestamp = Clock::time_point; 

enum class orderSide
{
    Bid,
    Ask,
};

class MarketOrder
{
    private:
    orderSide m_OrderSide {};
    Quantity m_Quantity {};
    OrderID m_OrderID{};

    public: 
    MarketOrder(orderSide side, Quantity quantity, OrderID orderid)
    : m_OrderSide{side}
    , m_Quantity{quantity}
    , m_OrderID{orderid}
    {}
    
    Quantity getQuantity() const 
    {
        return m_Quantity; 
    }
    orderSide getSide() const 
    {
        return m_OrderSide;
    } 
    OrderID getOrderID() const 
    {
        return m_OrderID; 
    }
};

class LimitOrder
{
    private: 
    orderSide m_OrderSide{};
    Quantity m_Quantity{};
    OrderID m_OrderID{};
    Price m_Price{}; 
    public: 
    LimitOrder(orderSide side, Quantity quantity, OrderID orderid, Price price)
    : m_OrderSide{side}
    , m_Quantity{quantity}
    , m_OrderID{orderid}
    , m_Price{price}
    {}
    Price getPrice() const
    {
        return m_Price; 
    }

    Quantity getQuantity() const 
    {
        return m_Quantity;
    }

    orderSide getOrderSide() const 
    {
        return m_OrderSide;
    }

    OrderID getOrderID() const{
        return m_OrderID;
    }
    void updateQuantity(Quantity filledQuantity) 
    {
        m_Quantity -= filledQuantity; 
    }

    void setQuantity(Quantity newQuantity)
    {
        m_Quantity = newQuantity;
    }
};

struct Trade
{
    static TradeID nextTradeID; 
    TradeID m_TradeID{};
    Price m_Price{};
    Quantity m_Qty{};
    OrderID m_AggressorOrderID{};
    OrderID m_RestingOrderID{};
    orderSide m_AggressorSide{};
    Timestamp m_Time{};

    Trade(Price price, Quantity quantity, OrderID aggressorOrderID, OrderID restingOrderID, orderSide aggressorside)
    : m_TradeID{nextTradeID++}
    , m_Price{price}
    , m_Qty{quantity}
    , m_AggressorOrderID{aggressorOrderID}
    , m_RestingOrderID{restingOrderID}
    , m_AggressorSide{aggressorside}
    , m_Time{Clock::now()}
    {}
};
TradeID Trade::nextTradeID = 1;

class TradeLog 
{
    private:
    std::vector<Trade> tradelog;

    public:
    void record(Trade trade) 
    {
        tradelog.push_back(std::move(trade));
    }
};

struct currentRestingSnapshot
    {
        OrderID orderid;
        Price price;
    };
class OrderBook 
{
    private:
    std::map<Price, std::deque<LimitOrder>> m_BidSide;
    std::map<Price, std::deque<LimitOrder>> m_AskSide; 
    
    public:

    void addBid(const LimitOrder& order)
    {
        Price price = order.getPrice();
        m_BidSide[price].push_back(order);
    }

    void addAsk(const LimitOrder& order)
    {
        Price price = order.getPrice();
        m_AskSide[price].push_back(order);
    }
   
    bool hasAsks() const
    {
        return !m_AskSide.empty();
    }

    bool hasBids() const
    {
        return !m_BidSide.empty();
    }

    std::optional<currentRestingSnapshot> peekBestAsk() const
    {
        if (m_AskSide.empty())
            return std::nullopt;

        auto priceIt = m_AskSide.begin();
        const auto& orderQueue = priceIt->second;

        if (orderQueue.empty())
            return std::nullopt;
        const LimitOrder& restingOrder = orderQueue.front();

        return currentRestingSnapshot{restingOrder.getOrderID(), restingOrder.getPrice()};
    }

    std::optional<currentRestingSnapshot> peekBestBid() const
    {
        if (m_BidSide.empty()) 
            return std::nullopt;
        auto priceIt = std::prev(m_BidSide.end());
        const auto& orderQueue = priceIt->second; 

        if (orderQueue.empty())
            return std::nullopt; 
        const LimitOrder& restingOrder = orderQueue.front();

        return currentRestingSnapshot{restingOrder.getOrderID(), restingOrder.getPrice()};
    }
    Quantity consumeBestAsk(Quantity quantity)
    {   
        if (m_AskSide.empty()) return 0;
        auto priceIt = m_AskSide.begin();
        auto& orderQueue = priceIt->second; 
        if (orderQueue.empty()) return 0;
        LimitOrder& restingOrder = orderQueue.front(); 
        Quantity executed = std::min(quantity, restingOrder.getQuantity());
        restingOrder.updateQuantity(executed);
        if (restingOrder.getQuantity() == 0)
        {
            orderQueue.pop_front(); 
        }
        if (orderQueue.empty())
        {
            m_AskSide.erase(priceIt);
        }
        return executed; 

    }
    Quantity consumeBestBid(Quantity quantity)
    {
        if (m_BidSide.empty()) return 0;
        auto priceIt = std::prev(m_BidSide.end());
        auto& orderQueue = priceIt->second; 
        if (orderQueue.empty()) return 0;
        LimitOrder& restingOrder = orderQueue.front();
        Quantity executed = std::min(quantity, restingOrder.getQuantity());
        restingOrder.updateQuantity(executed);
        if (restingOrder.getQuantity() == 0)
        {
            orderQueue.pop_front();
        }
        if (orderQueue.empty())
        {
            m_BidSide.erase(priceIt);
        }
        return executed; 
    }

};



class MatchingEngine
{
    private: 
    OrderBook book; 
    TradeLog tradelog;  

    public:
    
    void fillMarketOrder(MarketOrder& order) 
    {
        Quantity marketQty {order.getQuantity()};
        orderSide side {order.getSide()};
        if (side == orderSide::Bid)
        {
         while (marketQty > 0 && book.hasAsks())
        {
            auto restAskOpt = book.peekBestAsk();
            if(!restAskOpt) break;
            currentRestingSnapshot restingAsk =*restAskOpt;
            Quantity executed{book.consumeBestAsk(marketQty)};
            if(executed == 0)
               break;
            marketQty -= executed;
            Trade trade{restingAsk.price, executed, order.getOrderID(), restingAsk.orderid, order.getSide()};
            tradelog.record(trade); 
        } 
        } 
        else 
        {
         while(marketQty > 0 && book.hasBids())
            {
                auto restBidOpt = book.peekBestBid();
                if(!restBidOpt) break;
                currentRestingSnapshot restingBid = *restBidOpt;
                Quantity executed(book.consumeBestBid(marketQty));
                if(executed == 0)
                    break; 
                marketQty -= executed; 
                Trade trade{restingBid.price, executed, order.getOrderID(), restingBid.orderid, order.getSide()};
                tradelog.record(trade);
            }
        }
    }

    void fillAndRestLimitOrder(LimitOrder& order)
    {
        Quantity orderQty {order.getQuantity()};
        orderSide side {order.getOrderSide()};
        Price incomingLimitPrice {order.getPrice()};
        if(side == orderSide::Bid)
        {
         while(orderQty > 0 )
         {
            auto restAskOpt = book.peekBestAsk();
            if(!restAskOpt) 
                break;
         
            currentRestingSnapshot restingAsk =*restAskOpt;

            if(incomingLimitPrice < restingAsk.price)
              break;

                Quantity executed{book.consumeBestAsk(orderQty)};
                if (executed == 0) 
                    break;
                
                
                orderQty -= executed;
                Trade trade{
                    restingAsk.price, 
                    executed, 
                    order.getOrderID(), 
                    restingAsk.orderid,  
                    order.getOrderSide()
                };
                tradelog.record(trade);
             }
        
        if (orderQty > 0)
        {
         order.setQuantity(orderQty);
         book.addBid(order);
        }
       }
        else 
       {
            while(orderQty > 0 )
          {
            auto bestBidOpt = book.peekBestBid();
            if(!bestBidOpt) 
                break;
         
            currentRestingSnapshot restingBid =*bestBidOpt;

            if(incomingLimitPrice > restingBid.price)
              break;

                Quantity executed{book.consumeBestBid(orderQty)};
                if (executed == 0) 
                    break;
                
                
                orderQty -= executed;
                Trade trade{
                    restingBid.price, 
                    executed, 
                    order.getOrderID(), 
                    restingBid.orderid,  
                    order.getOrderSide()
                };
                tradelog.record(trade);
             }
        
            if (orderQty > 0)
            {
                order.setQuantity(orderQty);
                book.addAsk(order);
            } 
       }
    }
    
        

};