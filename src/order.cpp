#include "order.hpp"
#include <atomic>


    OrderID OrderIDGenerator::next()
    {
        static std::atomic<OrderID> id{1};
        return id.fetch_add(1, std::memory_order_relaxed);
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
