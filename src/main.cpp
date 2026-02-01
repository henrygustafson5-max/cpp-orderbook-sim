#include <iostream>


// Forward declarations
void benchmarkLimitInsert(std::size_t numOrders);
void benchmarkMarketFill(std::size_t depth);
void benchmarkMixed(std::size_t ops);

int main()
{
    std::cout << "=== Matching Engine Benchmarks ===\n\n";

    // Warm-up (optional but good practice)
    benchmarkLimitInsert(static_cast<std::size_t>(1000));

    // Core benchmarks
    benchmarkLimitInsert(static_cast<std::size_t>(100000));
    benchmarkMarketFill(static_cast<std::size_t>(10000));
    benchmarkMixed(static_cast<std::size_t>(200000));

    std::cout << "=== Benchmarks Complete ===\n";

    return 0;
}
