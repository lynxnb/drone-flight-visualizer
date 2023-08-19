cmake_minimum_required(VERSION 3.26)
project(drone_flight_visualizer)

set(CMAKE_CXX_STANDARD 20)

set(source_DIR ${CMAKE_SOURCE_DIR}/src)

add_executable(drone_flight_visualizer ${source_DIR}/main.cpp)
