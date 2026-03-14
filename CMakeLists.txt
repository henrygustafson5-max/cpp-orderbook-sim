cmake_minimum_required(VERSION 3.20)
project(cpp_orderbook_sim)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Dependencies via FetchContent ─────────────────────────────
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
    GIT_SHALLOW    TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.9.1
    GIT_SHALLOW    TRUE
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest googlebenchmark)

# ── Core library (shared by sim and tests) ────────────────────
add_library(orderbook_lib STATIC
    src/matching_engine.cpp
    src/order_book.cpp
    src/order.cpp
    src/trade.cpp
)
target_include_directories(orderbook_lib PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_compile_options(orderbook_lib PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)

# ── Main executable ───────────────────────────────────────────
add_executable(orderbook_sim
    src/main.cpp
)
target_link_libraries(orderbook_sim PRIVATE orderbook_lib)
target_compile_options(orderbook_sim PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)

# ── Benchmarks ────────────────────────────────────────────────
add_executable(orderbook_bench
    src/benchmark.cpp
)
target_link_libraries(orderbook_bench PRIVATE orderbook_lib benchmark::benchmark)
target_compile_options(orderbook_bench PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)

# ── Unit tests ────────────────────────────────────────────────
enable_testing()
add_executable(orderbook_tests
    tests/orderbook_test.cpp
)
target_link_libraries(orderbook_tests PRIVATE orderbook_lib GTest::gtest_main)
target_compile_options(orderbook_tests PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
include(GoogleTest)
gtest_discover_tests(orderbook_tests)
