#pragma once 
#include "order.hpp"

using TradeID = std::uint64_t; 

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

class TradeLog 
{
    private:
    std::vector<Trade> tradelog;

    public:
    void record(const Trade& trade); 

    void printTrade(std::size_t index) const;
        
    std::size_t getTradeLogSize() const;
};