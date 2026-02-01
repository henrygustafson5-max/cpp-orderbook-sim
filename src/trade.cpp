#include "trade.hpp"
#include "order.hpp"
#include <iostream>
using TradeID = std::uint64_t; 



    void TradeLog::record(const Trade& trade) 
    {
        tradelog.push_back(trade);
    }

    void TradeLog::printTrade(std::size_t index) const
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

    std::size_t TradeLog::getTradeLogSize() const 
    {
        return tradelog.size(); 
    }
    
