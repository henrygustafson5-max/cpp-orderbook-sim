#include <cstdint> 

namespace trading  
{
enum class orderSide
{
    Bid,
    Ask
};

enum class orderType
{
    Market,
    Limit 
};
}
using Price = int32_t;
using Quantity = int32_t;
using OrderID = int32_t; 

class Order
{
    private:
    trading::orderSide m_OrderSide {};
    trading::orderType m_OrderType {};
    Price m_Price {};
    Quantity m_Quantity {};
    OrderID m_OrderID{};

    public: 
     Order (trading::orderSide side, trading::orderType type, Price price, Quantity quantity, OrderID orderid)
    : m_OrderSide{side}
    , m_OrderType{type}
    , m_Price{price}
    , m_Quantity{quantity}
    , m_OrderID{orderid}
    {
    }    
    

};

class OrderBook
{
    private:

    public: 


};




class MatchingEngine
{
    private: 

    public:


};