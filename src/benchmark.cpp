#include "matching_engine.hpp"
#include "timersetup.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>

constexpr size_t kWorkloadSize = 500'000;
constexpr size_t kRuns = 10;

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
    return {modtype, 0, 0, OrderSide::Ask, generatequantity(), id};
  }
}


std::vector<Operation> buildworkload(){

  std::vector<Operation> workload;
  workload.reserve(kWorkloadSize);
  std::uniform_int_distribution<int> op (0, 100) ;

  for(size_t i {} ; i < kWorkloadSize ; ++i){

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

struct LatencyMetrics{
  double p90Ns{};
  double p99Ns{};
  double p999Ns{};
  double cyclesPerOp{};
};

// nth_element partitions around the index so the three percentile lookups
// don't need a full sort of the 500k samples.
uint64_t percentile(std::vector<uint64_t>& cycles, double pct){
  const auto idx = static_cast<std::ptrdiff_t>(
      pct * static_cast<double>(cycles.size() - 1));
  std::nth_element(cycles.begin(), cycles.begin() + idx, cycles.end());
  return cycles[static_cast<size_t>(idx)];
}

LatencyMetrics benchmark(const std::vector<Operation>& workload,
                         std::vector<uint64_t>& cycles){
  cycles.clear();

  for(const auto& op: workload){

    const uint64_t start = startClock();
    switch(op.type){
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
    case Operation::Type::Cancel:
        engine.cancelOrder(op.oid);
        break;
    case Operation::Type::ReduceQTY:
        engine.reduceOrder(op.oid, op.qty);
        break;
    }
    const uint64_t stop = stopClock();

    cycles.push_back(calculateCycles(start, stop));
  }

  uint64_t totalCycles {};
  for(uint64_t c: cycles){
    totalCycles += c;
  }

  LatencyMetrics metrics;
  metrics.p90Ns  = cyclesToNs(percentile(cycles, 0.90));
  metrics.p99Ns  = cyclesToNs(percentile(cycles, 0.99));
  metrics.p999Ns = cyclesToNs(percentile(cycles, 0.999));
  metrics.cyclesPerOp =
      static_cast<double>(totalCycles) / static_cast<double>(cycles.size());
  return metrics;
}


int main(){

  // stdout is block-buffered when not a TTY (e.g. over ssh); unbuffer so
  // results print live and survive an early kill.
  std::setvbuf(stdout, nullptr, _IONBF, 0);

  std::printf("timer overhead: %llu ticks | counter rate: %.3f ticks/ns\n",
              static_cast<unsigned long long>(timerOverhead), ticksPerNs());

  LatencyMetrics avg{};
  std::vector<uint64_t> cycles;
  cycles.reserve(kWorkloadSize);

  for(size_t run {}; run < kRuns; ++run){
    engine = MatchingEngine(200);
    std::vector<Operation> workload {buildworkload()};

    const LatencyMetrics metrics = benchmark(workload, cycles);
    avg.p90Ns       += metrics.p90Ns;
    avg.p99Ns       += metrics.p99Ns;
    avg.p999Ns      += metrics.p999Ns;
    avg.cyclesPerOp += metrics.cyclesPerOp;

    if((run + 1) % 100 == 0){
      std::fprintf(stderr, "run %zu/%zu\r", run + 1, kRuns);
    }
  }
  std::fprintf(stderr, "\n");

  const auto runs = static_cast<double>(kRuns);
  std::printf("averaged over %zu runs of %zu ops:\n", kRuns, kWorkloadSize);
  std::printf("  P90    latency: %10.1f ns\n", avg.p90Ns  / runs);
  std::printf("  P99    latency: %10.1f ns\n", avg.p99Ns  / runs);
  std::printf("  P99.9  latency: %10.1f ns\n", avg.p999Ns / runs);
  std::printf("  cycles per op : %10.1f\n",    avg.cyclesPerOp / runs);

}
