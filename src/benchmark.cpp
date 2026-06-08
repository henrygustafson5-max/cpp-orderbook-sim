#include "matching_engine.hpp"
#include <cstdint>
#include <random>
#include <vector>
#include <x86intrin.h>

 MatchingEngine engine(200);  
  
 struct Operation{ 
 
   enum class Type{
     LimitFOK,
     LimitIOC,
     LimitGTC,
     Market,
     Cancel,
     ReduceQTY,
        
   };

   Type type{};
   SymbolID ticker{};
   Price price{};         
   OrderSide side{}; 
   Quantity qty{};
   OrderID oid{}; 
  
};


  
std::mt19937_64 gen(std::random_device{}());


//50 symbols generate 80% of market traffic - remaining 150 are 20% 
SymbolID generatesymbol(){
  std::uniform_int_distribution<uint32_t> dist(0,100);
  std::uniform_int_distribution<SymbolID> hot(0,  50);
  std::uniform_int_distribution<SymbolID> cold(51, 199);
  
  uint32_t path {dist(gen)};
  if(path < 80){
    return hot(gen);
  }
  return cold(gen);
}



Price generateprice(){
  std::uniform_int_distribution<Price> mid(150, 250);
  std::uniform_int_distribution<Price> low(1, 149);
  std::uniform_int_distribution<Price> high(251, 300);
  std::uniform_int_distribution<int> dist(0, 100);
  int roll {dist(gen)};
  if(roll < 80){
    return mid(gen);
  }
  else if (roll < 90){
    return low(gen);
  }
  return high(gen);
}


Quantity generatequantity(){
  std::uniform_int_distribution<Quantity> mid(200, 300);
  std::uniform_int_distribution<Quantity> low(1, 199);
  std::uniform_int_distribution<Quantity> high(201, 300);
  std::uniform_int_distribution<int> dist(0, 100); 
  int roll = dist(gen);
  if(roll < 80){
    return mid(gen);
  }
  else if (roll < 90){
    return low(gen);
  }
  return high(gen);
}

Operation generatelimitorder(){
std::uniform_int_distribution<int> type (0, 2);
std::uniform_int_distribution<int> side (0, 1);
Operation::Type ordertype {type(gen)};
OrderSide orderside {side(gen)};
return {ordertype, generatesymbol(), generateprice(), orderside, generatequantity(),OrderIDGenerator::next()};
}
 
Operation generatemarketorder(){
std::uniform_int_distribution<int> type (0, 2);
std::uniform_int_distribution<int> side (0, 1);
Operation::Type ordertype {Operation::Type::Market};
OrderSide orderside {side(gen)};
return {ordertype, generatesymbol(), -1, orderside, generatequantity(), OrderIDGenerator::next()};
}

Operation generateordermod(){
  std::uniform_int_distribution<int> dist{0, 100};  
  int roll = dist(gen);
  std::uniform_int_distribution<OrderID> randid {0, OrderIDGenerator::max};
  if(roll < 80){
    Operation::Type modtype {Operation::Type::Cancel}; 
    OrderID id {randid(gen)};
    return {modtype, 0, 0, OrderSide::Ask, 0, id };
  }  
  else{
    Operation::Type modtype {Operation::Type::ReduceQTY};
    OrderID id {randid(gen)};
    return {modtype, 0, 0, OrderSide::Ask, 0, id};
  }
}


std::vector<Operation> buildworkload(){
    
  std::vector<Operation> workload(500'000); 
  std::uniform_int_distribution<int> op (0, 100) ;
  
  for(size_t i {} ; i < 500'000 ; ++i){
 
  int roll {op(gen)};
  
  if(roll < 60){
    workload.push_back(generatelimitorder());
  }
  else if(roll < 90){
    workload.push_back(generateordermod());
  }
  else{
    workload.push_back(generatemarketorder());
  }  
}
 return workload; 
}  

benchmark(std::vector<Operation>& workload){
     
  for(auto& op: workload){

    auto type{op.type}; 
    switch(type){ 
    case Operation::Type::LimitFOK: 
       engine.submitLimitOrder(op.ticker, op.side, op.qty, op.oid, op.price, LimitType::FOK);
       break;
    case Operation::Type::LimitGTC: 
       engine.submitLimitOrder(op.ticker, op.side, op.qty, op.oid, op.price, LimitType::GTC);
       break;
    case Operation::Type::LimitIOC:
       engine.submitLimitOrder(op.ticker, op.side, op.qty, op.oid, op.price, LimitType::IOC);
       break;
    case Operation::Type::Market:
       engine.submitMarketOrder(op.ticker, op.side, op.qty, op.oid);
       break;
    case Operation::Type::ReduceQTY:
        engine.cancelOrder(op.oid);
        break;
    case Operation::Type::Cancel: 
        engine.reduceOrder(op.oid, op.qty);
        break;
    }
    


  }


}

struct LatencyMetrics{
 uint64_t P99{};
 uint64_t P
}


int main(){
  
 std::vector<LatencyMetrics>(100000);
 for(size_t iterations{} ; i  < 100000 ; ++i){
 std::vector<Operation> workload {buildworkload()};
  

 }
 //Start timer
  
   


}
