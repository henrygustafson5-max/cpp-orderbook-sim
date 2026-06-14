#pragma once 
#include <cstdint> 

using Price = int32_t;
using Quantity = uint32_t;
using OrderID = int32_t;

enum class OrderSide
{
    Bid,
    Ask,
};

struct OrderIDGenerator
{
    static OrderID next();
    static int max;
};

enum class LimitType
{
    GTC,
    FOK,
    IOC,
};

class LimitOrder
{
    private: 
    OrderSide m_OrderSide{};
    Quantity m_Quantity{};
    OrderID m_OrderID{};
    Price m_Price{}; 
    LimitType m_LimitType{};

    public: 
    LimitOrder(OrderSide side, Quantity quantity, OrderID orderid, Price price, LimitType type)
    : m_OrderSide{side}
    , m_Quantity{quantity}
    , m_OrderID{orderid}
    , m_Price{price}
    , m_LimitType{type}
    {}

    Price getPrice() const;

    LimitType getType() const;

    Quantity getQuantity() const;

    OrderSide getOrderSide() const;

    OrderID getOrderID() const;

    void updateQuantity(Quantity filledQuantity);

    void setQuantity(Quantity newQuantity);
};
