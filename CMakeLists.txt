cmake_minimum_required(VERSION 3.26)
project(drone_flight_visualizer)

set(CMAKE_CXX_STANDARD 20)

set(source_DIR ${CMAKE_SOURCE_DIR}/src)

set(CPM_SOURCE_CACHE ${CMAKE_SOURCE_DIR}/libraries)

include(cmake/CPM.cmake)

# Glaze JSON/CSV library
CPMAddPackage("gh:stephenberry/glaze@1.3.5")
include_directories(${glaze_SOURCE_DIR}/include/glaze)

add_executable(drone_flight_visualizer ${source_DIR}/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE glaze::glaze)

