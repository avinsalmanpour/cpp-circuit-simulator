cmake_minimum_required(VERSION 3.29)

project(cpp-circuit-simulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(cpp-circuit-simulator
        src/circuit_simulator.cpp)
