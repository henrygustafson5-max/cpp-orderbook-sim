#include <cstddef>
#include <cstdint> 
#include <map>
#include <deque>
#include <chrono> 
#include <memory>
#include <vector>
#include <algorithm>
#include <optional> 
#include <iostream>

using Price = int32_t;
using Quantity = uint32_t;
using OrderID = int32_t;
using TradeID = std::uint64_t; 
using Clock = std::chrono::steady_clock;
using Timestamp = Clock::time_point; 

enum class OrderSide
{
    Bid,
    Ask,
};

struct OrderIDGenerator
{
    static OrderID next()
    {
        static OrderID id{1};
        return id++;
    }
};


class MarketOrder
{
    private:
    OrderSide m_OrderSide {};
    Quantity m_Quantity {};
    OrderID m_OrderID{};

    public: 
    MarketOrder(OrderSide side, Quantity quantity, OrderID orderid)
    : m_OrderSide{side}
    , m_Quantity{quantity}
    , m_OrderID{orderid}
    {}
    
    Quantity getQuantity() const 
    {
        return m_Quantity; 
    }
    OrderSide getSide() const 
    {
        return m_OrderSide;
    } 
    OrderID getOrderID() const 
    {
        return m_OrderID; 
    }

    void updateQuantity(Quantity executed) 
    {
        m_Quantity -= executed;
    }
};

class LimitOrder
{
    private: 
    OrderSide m_OrderSide{};
    Quantity m_Quantity{};
    OrderID m_OrderID{};
    Price m_Price{}; 
    Timestamp m_Time{Clock::now()};
    public: 
    LimitOrder(OrderSide side, Quantity quantity, OrderID orderid, Price price)
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

    OrderSide getOrderSide() const 
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
    OrderSide m_AggressorSide{};
    Timestamp m_Time{};

    Trade(Price price, Quantity quantity, OrderID aggressorOrderID, OrderID restingOrderID, OrderSide aggressorside)
    : m_TradeID{nextTradeID++}
    , m_Price{price}
    , m_Qty{quantity}
    , m_AggressorOrderID{aggressorOrderID}
    , m_RestingOrderID{restingOrderID}
    , m_AggressorSide{aggressorside}
    , m_Time{Clock::now()}
    {}
};
TradeID Trade::nextTradeID = 0;

class TradeLog 
{
    private:
    std::vector<Trade> tradelog;

    public:
    void record(Trade trade) 
    {
        tradelog.push_back(std::move(trade));
    }

    void printTrade(std::size_t index) const
    {
        if (index >= tradelog.size())
        {
            std::cout<<"Invalid Index \n";
            return;
        }
        const Trade& t {tradelog[index]};
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        t.m_Time.time_since_epoch()).count();
        std::cout
        << "Trade ID: "<<t.m_TradeID
        << " Price: "<<t.m_Price
        << " Traded Quantity: "<<t.m_Qty
        << " Aggressor ID: "<<t.m_AggressorOrderID
        << " Resting ID: "<< t.m_RestingOrderID
        << " Aggressor Side: "<< (t.m_AggressorSide == OrderSide::Bid ? "Bid" : "Ask")
        << " Trade Time (ns): " << ns <<"\n" ;
    }

    std::size_t getTradeLogSize() const 
    {
        return tradelog.size(); 
    }
};


struct currentRestingSnapshot
    {
        OrderID orderid;
        Price price;
    };

using Queue = std::deque<std::unique_ptr<LimitOrder>>; 
class OrderBook 
{
    private:
    std::map<Price, Queue, std::greater<Price>> m_BidSide;
    std::map<Price, Queue, std::less<Price>> m_AskSide; 
    
    public:

    void addBid(std::unique_ptr<LimitOrder> order)
    {
       const Price price = order->getPrice();
        m_BidSide[price].push_back(std::move(order));
    }

    void addAsk(std::unique_ptr<LimitOrder> order)
    {
        const Price price = order->getPrice();
        m_AskSide[price].push_back(std::move(order));
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
        const LimitOrder& restingOrder = *orderQueue.front();

        return currentRestingSnapshot{restingOrder.getOrderID(), restingOrder.getPrice()};
    }

    std::optional<currentRestingSnapshot> peekBestBid() const
    {
        if (m_BidSide.empty()) 
            return std::nullopt;
        auto priceIt = m_BidSide.begin();
        const auto& orderQueue = priceIt->second; 

        if (orderQueue.empty())
            return std::nullopt; 
        const LimitOrder& restingOrder = *orderQueue.front();

        return currentRestingSnapshot{restingOrder.getOrderID(), restingOrder.getPrice()};
    }
    Quantity consumeBestAsk(Quantity quantity)
    {   
        if (m_AskSide.empty()) return 0;
        auto priceIt = m_AskSide.begin();
        auto& orderQueue = priceIt->second; 
        if (orderQueue.empty()) return 0;
        LimitOrder& restingOrder = *orderQueue.front(); 
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
        auto priceIt = m_BidSide.begin();
        auto& orderQueue = priceIt->second; 
        if (orderQueue.empty()) return 0;
        LimitOrder& restingOrder = *orderQueue.front();
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

    void submitLimitOrder(OrderSide orderSide, Quantity quantity, OrderID orderID, Price price)
    {
    auto limitOrder = std::make_unique<LimitOrder>(orderSide, quantity, orderID, price); 
    fillAndRestLimitOrder(std::move(limitOrder)); 
    }

    void submitMarketOrder( OrderSide side, Quantity quantity, OrderID id)
    {
    MarketOrder order{side, quantity, id};
    fillMarketOrder(order);
    }    

    void fillMarketOrder(MarketOrder order) 
    {
        Quantity marketQty {order.getQuantity()};
        OrderSide side {order.getSide()};
        if (side == OrderSide::Bid)
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
            order.updateQuantity(executed);
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
                order.updateQuantity(executed); 
                Trade trade{restingBid.price, executed, order.getOrderID(), restingBid.orderid, order.getSide()};
                tradelog.record(trade);
            }
        }
    }

    void fillAndRestLimitOrder(std::unique_ptr<LimitOrder> limitOrder)
    {
        Quantity orderQty {limitOrder->getQuantity()};
        OrderSide side {limitOrder->getOrderSide()};
        Price incomingLimitPrice {limitOrder->getPrice()};
        if(side == OrderSide::Bid)
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
                    limitOrder->getOrderID(), 
                    restingAsk.orderid,  
                    limitOrder->getOrderSide()
                };
                tradelog.record(trade);
             }
        
        if (orderQty > 0)
        {
         limitOrder->setQuantity(orderQty);
         book.addBid(std::move(limitOrder));
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
                    limitOrder->getOrderID(), 
                    restingBid.orderid,  
                    limitOrder->getOrderSide()
                };
                tradelog.record(trade);
             }
        
            if (orderQty > 0)
            {
                limitOrder->setQuantity(orderQty);
                book.addAsk(std::move(limitOrder));
            } 
       }
    }
    
    void printTrade(auto index) const 
    {
        tradelog.printTrade(index);
    }
    std::size_t getLogSize() const
    {
        return tradelog.getTradeLogSize();
    }

};


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