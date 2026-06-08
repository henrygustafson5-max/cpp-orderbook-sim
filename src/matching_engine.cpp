#include "matching_engine.hpp"
#include "order.hpp"
  
    MatchingEngine::MatchingEngine(size_t numberofsymbols)
    {book.reserve(numberofsymbols);}
    
    MatchingEngine::MatchingEngine()
    {
    book.reserve(1);
    }
    void MatchingEngine::submitLimitOrder(SymbolID ticker, OrderSide orderSide, Quantity quantity, OrderID orderID, Price price, LimitType type )
    {
        if (quantity == 0 || price <= 0) return;
        auto limitOrder = std::make_unique<LimitOrder>(orderSide, quantity, orderID, price, type);
        if(orderSide == OrderSide::Ask) fillAndRestLimitAsk(ticker, std::move(limitOrder));
        else fillAndRestLimitBid(ticker, std::move(limitOrder));
    }
     
    void MatchingEngine::submitMarketOrder(SymbolID ticker, OrderSide side, Quantity quantity, OrderID id)
    {
       if (quantity == 0) return;
        MatchingEngine::fillMarketOrder(ticker, side, quantity, id);
    }    
      
    void MatchingEngine::fillMarketOrder(SymbolID ticker, OrderSide marketSide, Quantity marketQty, OrderID marketID)
    {
        if (marketSide == OrderSide::Bid)
        {
         while (marketQty > 0 && book[ticker].hasAsks())
        {
            auto tradeopt{book[ticker].consumeBestAsk(marketQty)};
            if(!tradeopt) break; 
            ExecutionReport executedtrade = *tradeopt; 
            marketQty -= executedtrade.executedQTY;
            Trade trade{ticker, MatchingEngine::id++,executedtrade.restingPrice, executedtrade.executedQTY, marketID, executedtrade.restingID, marketSide};  
            tradelog.record(trade); 
        } 
        } 
        else 
        {
         while(marketQty > 0 && book[ticker].hasBids())
            {
                auto tradeopt{book[ticker].consumeBestBid(marketQty)};
                if(!tradeopt) break; 
                ExecutionReport executedtrade = *tradeopt; 
                marketQty -= executedtrade.executedQTY;
                Trade trade{ticker, MatchingEngine::id++, executedtrade.restingPrice, executedtrade.executedQTY, marketID, executedtrade.restingID, marketSide};
                tradelog.record(trade);  
            }
        }
    }
    void MatchingEngine::fillAndRestLimitBid(SymbolID ticker, std::unique_ptr<LimitOrder> incomingOrder)
    {
        Price incomingPrice {incomingOrder->getPrice()};
        LimitType type {incomingOrder->getType()};
        OrderID oid {incomingOrder->getOrderID()};
        while(incomingOrder->getQuantity() > 0 )
         {
            auto bestPriceOpt = book[ticker].bestAsk();
            if(!bestPriceOpt) break; 
            Price restingAsk =*bestPriceOpt;
            if(incomingPrice < restingAsk) break;
            if(type == LimitType::FOK){
                if(!book[ticker].FOKVolumeCheck(OrderSide::Bid, incomingPrice, incomingOrder->getQuantity())) return;
            }
            auto executedTradeOpt{book[ticker].consumeBestAsk(incomingOrder->getQuantity())};
            if(!executedTradeOpt) break;
            ExecutionReport executedTrade = *executedTradeOpt;
            if (executedTrade.executedQTY == 0) break; 
            incomingOrder->updateQuantity(executedTrade.executedQTY);
                Trade trade{
                    ticker, 
                    MatchingEngine::id++,
                    executedTrade.restingPrice,
                    executedTrade.executedQTY,
                    oid,
                    executedTrade.restingID,
                    OrderSide::Bid
                };
             tradelog.record(trade);
        } 
        if(incomingOrder->getQuantity() > 0 && type == LimitType::GTC) {
        idToSymbol[oid] = ticker;
        book[ticker].addBid(std::move(incomingOrder));
          
    }
    }


    void MatchingEngine::fillAndRestLimitAsk(SymbolID ticker, std::unique_ptr<LimitOrder> incomingOrder)
    {
        Price incomingPrice {incomingOrder->getPrice()};
        LimitType type {incomingOrder->getType()};
        OrderID oid {incomingOrder->getOrderID()};
        while(incomingOrder->getQuantity() > 0 )
         {
            auto bestPriceOpt = book[ticker].bestBid();
            if(!bestPriceOpt) break; 
            Price restingBid =*bestPriceOpt;
            if(incomingPrice > restingBid) break;
            if(type == LimitType::FOK){
                if(!book[ticker].FOKVolumeCheck(OrderSide::Ask, incomingPrice, incomingOrder->getQuantity())) return;
            }
            auto executedTradeOpt{book[ticker].consumeBestBid(incomingOrder->getQuantity())};
            if(!executedTradeOpt) break;
            ExecutionReport executedTrade = *executedTradeOpt;
            if (executedTrade.executedQTY == 0) break; 
            incomingOrder->updateQuantity(executedTrade.executedQTY);
                Trade trade{
                    ticker, 
                    MatchingEngine::id++,
                    executedTrade.restingPrice,
                    executedTrade.executedQTY,
                    oid,
                    executedTrade.restingID,
                    OrderSide::Ask
                };
             tradelog.record(trade);
        } 
        if(incomingOrder->getQuantity() > 0 && type == LimitType::GTC)
        {
          idToSymbol[oid] = ticker;
          book[ticker].addAsk(std::move(incomingOrder));
        }
      }

    std::optional<SymbolID> MatchingEngine::requestModify(OrderID id)
    {
      auto ticker{idToSymbol[id]};
      if(book[ticker].orderExists(id)) return ticker;
      return std::nullopt;
    }
    
    bool MatchingEngine::cancelOrder(OrderID id)
    {
        auto orderexists = requestModify(id);
        if(orderexists == std::nullopt) return false;
        auto ticker {*orderexists};
        book[ticker].cancelOrder(id);
        return true;
    }

    bool MatchingEngine::reduceOrder(OrderID id, Quantity newQty) 
    {
      auto orderexists = requestModify(id);
      if(orderexists == std::nullopt) return false;
      auto ticker{*orderexists};
      auto info = book[ticker].infoFromID(id);
      const auto& order = *info.orderIT;
      Quantity restingQTY = order->getQuantity();
      if(newQty <= 0 || newQty >= restingQTY) return false;
      book[ticker].reduceQuantity(id,newQty);
      return true;
    }

    bool MatchingEngine::cancelReplace(OrderID id, Quantity newQTY, Price newPrice)
    {
      auto orderexists {requestModify(id)};
      if(orderexists == std::nullopt) return false;
      auto ticker{*orderexists};
      auto info = book[ticker].infoFromID(id);
      OrderSide side = info.side;
      if(!cancelOrder(id)) return false;
      MatchingEngine::submitLimitOrder(ticker,side, newQTY, OrderIDGenerator::next() , newPrice, LimitType::GTC);
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


