cmake_minimum_required(VERSION 3.26)
project(drone_flight_visualizer)

set(CMAKE_CXX_STANDARD 20)

set(source_DIR ${CMAKE_SOURCE_DIR}/src)

if(WIN32) # Install dlls in the same directory as the executable on Windows
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

# CPM.cmake: package management utility for CMake
set(CPM_SOURCE_CACHE ${CMAKE_SOURCE_DIR}/libraries)
include(cmake/CPM.cmake)

# Glaze: JSON/CSV parsing
CPMAddPackage("gh:stephenberry/glaze@1.3.5")
include_directories(SYSTEM ${glaze_SOURCE_DIR}/include)

# CPR: C++ Requests
# TODO: update to the latest tag once PR 948 is merged and released
# PR 948 fixes a bug where the library would try to link with stdc++fs on Windows
CPMAddPackage("gh:libcpr/cpr#d7cec83e9480253bacf11ff6441e9d00c08c71bf")
include_directories(SYSTEM ${cpr_SOURCE_DIR}/include)

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
CPMAddPackage("gh:glfw/glfw#3.3.8")
include_directories(SYSTEM ${glfw_SOURCE_DIR}/include)

# GLM
CPMAddPackage("gh:g-truc/glm#0.9.9.8")
include_directories(SYSTEM ${glm_SOURCE_DIR}/glm)

# Find installed Vulkan
find_package(Vulkan REQUIRED)
include_directories(SYSTEM ${Vulkan_INCLUDE_DIRS})

# Build the executable
add_executable(drone_flight_visualizer
        ${source_DIR}/main.cpp
        ${source_DIR}/vulkan/vulkan.cpp
)

# Link the executable to the libraries
target_link_libraries(drone_flight_visualizer PRIVATE
        glaze::glaze
        cpr::cpr
        Vulkan::Vulkan
        glfw
        glm
)
