cmake_minimum_required(VERSION 3.20)
project(cpp_orderbook_sim)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(orderbook_sim
    src/main.cpp
    src/matching_engine.cpp
    src/order_book.cpp
    src/order.cpp
    src/trade.cpp
    src/benchmark.cpp
)

target_include_directories(orderbook_sim
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)


target_compile_options(orderbook_sim PRIVATE
-Wall 
-Wextra 
-Wpedantic
-Wconversion 
-Wsign-conversion
)