cmake_minimum_required(VERSION 3.16)

project(cpp_orderbook_sim)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(cpp_orderbook_sim
    src/main.cpp
    )

target_compile_options(cpp_orderbook_sim PRIVATE
-Wall 
-Wextra 
-Wpedantic
-Wconversion 
-Wsign-conversion
)