cmake_minimum_required(VERSION 3.26)
project(drone_flight_visualizer)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(OUTPUT_BINARY_DIR "${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# CPM.cmake: package management utility for CMake
set(CPM_SOURCE_CACHE ${CMAKE_SOURCE_DIR}/libraries)
include(cmake/CPM.cmake)

# rapidjson: JSON parsing
set(RAPIDJSON_BUILD_DOC OFF)
set(RAPIDJSON_BUILD_EXAMPLES OFF)
set(RAPIDJSON_BUILD_TESTS ON)
CPMAddPackage("gh:Tencent/rapidjson@1.1.0")
include_directories(SYSTEM ${rapidjson_SOURCE_DIR}/include)

# csv-parser: CSV parsing
CPMAddPackage("gh:vincentlaucsb/csv-parser#2.1.3")
include_directories(SYSTEM ${csv-parser_SOURCE_DIR}/single_include)

# CPR: C++ Requests
# TODO: update to the latest tag once PR 948 is merged and released
# PR 948 fixes a bug where the library would try to link with stdc++fs on Windows
CPMAddPackage("gh:libcpr/cpr#d7cec83e9480253bacf11ff6441e9d00c08c71bf")
include_directories(SYSTEM ${cpr_SOURCE_DIR}/include)

# vk-bootstrap
set(VK_BOOTSTRAP_VULKAN_HEADER_DIR ${Vulkan_INCLUDE_DIRS})
CPMAddPackage("gh:charles-lunarg/vk-bootstrap#61f7761")
include_directories(SYSTEM ${vk-bootstrap_SOURCE_DIR}/src)

# GLFW: Vulkan window and wurface
set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_INSTALL OFF)
CPMAddPackage("gh:glfw/glfw#3.3.8")
include_directories(SYSTEM ${glfw_SOURCE_DIR}/include)

# GLM: Vector and matrix math
CPMAddPackage("gh:g-truc/glm#0.9.9.8")
include_directories(SYSTEM ${glm_SOURCE_DIR}/glm)

# Find installed Vulkan
find_package(Vulkan REQUIRED)
include_directories(SYSTEM ${Vulkan_INCLUDE_DIRS})

# Vulkan Memory Allocator
CPMAddPackage("gh:GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator@3.0.1")
include_directories(SYSTEM ${VulkanMemoryAllocator_SOURCE_DIR}/include)

add_subdirectory(src)

add_subdirectory(shaders)
