#include <cstdint> 
#include <vector> 

enum class orderSide
{
    Bid,
    Ask,
};

using Price = int32_t;
using Quantity = uint32_t;
using OrderID = int32_t; 

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
};

class OrderBook 
{
    private:
    std::vector<LimitOrder> m_BidVector;
    std::vector<LimitOrder> m_AskVector; 

    public: 

    //InsertBid inserts a bid limit order into descending order 
    void insertBid(const LimitOrder& order)
    {
        auto it = m_BidVector.begin();

        while (it != m_BidVector.end() && it->getPrice() >= order.getPrice())
        {
            ++it;
        }

        m_BidVector.insert(it, order);
    }
    //Insertask inserts an ask limit order into ascending order  
    void insertAsk(const LimitOrder& order)
    {
        auto it = m_AskVector.begin();

        while(it!= m_AskVector.end() && it->getPrice() <= order.getPrice()) 
        {
            ++it; 
        }

        m_AskVector.insert(it, order);
    }
    //returns best ask from orderbook
    const LimitOrder& getBestAsk()
    {
        return m_AskVector.front();
    }
    //returns best bid from orderbook
    const LimitOrder& getBestBid() 
    {
        return m_BidVector.front();
    }
};


class MatchingEngine
{
    private: 
    OrderBook book; 
    
    public:
    

};