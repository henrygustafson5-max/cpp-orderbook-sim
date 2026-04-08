#include "matching_engine.hpp"
#include "order.hpp"


    
    void MatchingEngine::submitLimitOrder(OrderSide orderSide, Quantity quantity, OrderID orderID, Price price)
    {
        if (quantity == 0 || price <= 0) return;
        auto limitOrder = std::make_unique<LimitOrder>(orderSide, quantity, orderID, price);
        fillAndRestLimitOrder(std::move(limitOrder));
    }


    void MatchingEngine::submitMarketOrder(OrderSide side, Quantity quantity, OrderID id)
    {
        if (quantity == 0) return;
        MatchingEngine::fillMarketOrder(side, quantity, id);
    }    

    void MatchingEngine::fillMarketOrder(OrderSide marketSide, Quantity marketQty, OrderID marketID)
    {
        if (marketSide == OrderSide::Bid)
        {
         while (marketQty > 0 && book.hasAsks())
        {
            auto tradeopt{book.consumeBestAsk(marketQty)};
            if(!tradeopt) break; 
            ExecutionReport executedtrade = *tradeopt; 
            marketQty -= executedtrade.executedQTY;
            Trade trade{MatchingEngine::id++,executedtrade.restingPrice, executedtrade.executedQTY, marketID, executedtrade.restingID, marketSide};  
            tradelog.record(trade); 
        } 
        } 
        else 
        {
         while(marketQty > 0 && book.hasBids())
            {
                auto tradeopt{book.consumeBestBid(marketQty)};
                if(!tradeopt) break; 
                ExecutionReport executedtrade = *tradeopt; 
                marketQty -= executedtrade.executedQTY;
                Trade trade{MatchingEngine::id++, executedtrade.restingPrice, executedtrade.executedQTY, marketID, executedtrade.restingID, marketSide};
                tradelog.record(trade);  
            }
        }
    }

    void MatchingEngine::fillAndRestLimitOrder(std::unique_ptr<LimitOrder> limitOrder)
    {
        OrderSide side {limitOrder->getOrderSide()};
        Price incomingLimitPrice {limitOrder->getPrice()};
        if(side == OrderSide::Bid)
        {
         while(limitOrder->getQuantity() > 0 )
         {
            auto bestPriceOpt = book.bestAsk();
            if(!bestPriceOpt) 
                break;
         
            Price restingAsk =*bestPriceOpt;

            if(incomingLimitPrice < restingAsk)
              break;

                auto executedTradeOpt{book.consumeBestAsk(limitOrder->getQuantity())};
                if(!executedTradeOpt) break;
                ExecutionReport executedTrade = *executedTradeOpt;
                if (executedTrade.executedQTY == 0) 
                    break;
                
                limitOrder->updateQuantity(executedTrade.executedQTY);
                Trade trade{
                    MatchingEngine::id++,
                    executedTrade.restingPrice,
                    executedTrade.executedQTY,
                    limitOrder->getOrderID(),
                    executedTrade.restingID,
                    OrderSide::Bid
                };
                tradelog.record(trade);
             }
        
        if (limitOrder->getQuantity() > 0)
        {
         book.addBid(std::move(limitOrder));
        }
       }
        else 
       {
        while(limitOrder->getQuantity() > 0 )
         {
            auto bestPriceOpt = book.bestBid();
            if(!bestPriceOpt) 
                break;
         
            Price restingBid =*bestPriceOpt;

            if(incomingLimitPrice > restingBid)
              break;

                auto executedTradeOpt{book.consumeBestBid(limitOrder->getQuantity())};
                if(!executedTradeOpt) break;
                ExecutionReport executedTrade = *executedTradeOpt;
                if (executedTrade.executedQTY == 0) 
                    break;
                
                limitOrder->updateQuantity(executedTrade.executedQTY);
                Trade trade{
                    MatchingEngine::id++,
                    executedTrade.restingPrice,
                    executedTrade.executedQTY,
                    limitOrder->getOrderID(),
                    executedTrade.restingID,
                    OrderSide::Ask
                };
                tradelog.record(trade);
             } 
            if(limitOrder->getQuantity() > 0)
            book.addAsk(std::move(limitOrder));
       }
    }


    bool MatchingEngine::requestModify(OrderID id)
    {
      return book.orderExists(id);
    }

    bool MatchingEngine::cancelOrder(OrderID id)
    {
        if(!requestModify(id)) return false;
        book.cancelOrder(id);
        return true;
    }
    
    bool MatchingEngine::reduceOrder(OrderID id, Quantity newQty) 
    {
      if(!requestModify(id)) return false; 
      auto info = book.infoFromID(id);
      const auto& order = *info.orderIT;
      Quantity restingQTY = order->getQuantity();
      if(newQty <= 0 || newQty >= restingQTY) return false;
      book.reduceQuantity(id,newQty);
      return true;
    }
    bool MatchingEngine::cancelReplace(OrderID id, Quantity newQTY, Price newPrice)
    {
      if(!requestModify(id)) return false;
      auto info = book.infoFromID(id);
      OrderSide side = info.side;
      if(!cancelOrder(id)) return false;
      MatchingEngine::submitLimitOrder(side, newQTY, OrderIDGenerator::next() , newPrice);
      return true;
    }
   
    void MatchingEngine::printTrade(std::size_t index) const 
    {
        tradelog.printTrade(index);
    }
    std::size_t MatchingEngine::getLogSize() const
    {
        return tradelog.getTradeLogSize();
    }


    std::optional<Price> MatchingEngine::bestBid() const
    {
        return book.bestBid();
    }

    std::optional<Price> MatchingEngine::bestAsk() const
    {
        return book.bestAsk();
    }
   
    bool MatchingEngine::hasAsk() const
    {
        return book.hasAsks();
    }
    
    bool MatchingEngine::hasBid() const
    {
        return book.hasBids();
    }
