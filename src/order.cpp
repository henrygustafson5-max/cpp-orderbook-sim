#include "order.hpp"
#include <cstdint>
#include <chrono>

using Clock = std::chrono::steady_clock;
using Timestamp = Clock::time_point; 

using Price = int32_t;
using Quantity = uint32_t;
using OrderID = int32_t;


    OrderID OrderIDGenerator::next()
    {
        static OrderID id{1};
        return id++;
    }


    Price LimitOrder::getPrice() const
    {
        return m_Price; 
    }

    Quantity LimitOrder::getQuantity() const 
    {
        return m_Quantity;
    }

    OrderSide LimitOrder::getOrderSide() const 
    {
        return m_OrderSide;
    }

    OrderID LimitOrder::getOrderID() const{
        return m_OrderID;
    }
    void LimitOrder::updateQuantity(Quantity filledQuantity) 
    {
        m_Quantity -= filledQuantity; 
    }

    void LimitOrder::setQuantity(Quantity newQuantity)
    {
        m_Quantity = newQuantity;
    }
