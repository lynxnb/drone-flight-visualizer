cmake_minimum_required(VERSION 3.20)
project(drone_flight_visualizer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(OUTPUT_BINARY_DIR "${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")
# Disallow multi-configuration generators append a per-configuration subdirectory with $<0:>
# TODO: get rid of per-configuration folders and let the generator handle it
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_BINARY_DIR}/$<0:>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# Disable MSVC deprecation warnings
if (WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
endif ()

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
add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_LEFT_HANDED)
CPMAddPackage("gh:g-truc/glm#0.9.9.8")
include_directories(SYSTEM ${glm_SOURCE_DIR}/glm)

# Find installed Vulkan
find_package(Vulkan REQUIRED)
include_directories(SYSTEM ${Vulkan_INCLUDE_DIRS})

# Vulkan Memory Allocator
CPMAddPackage("gh:GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator@2.3.0")
include_directories(SYSTEM ${VulkanMemoryAllocator_SOURCE_DIR}/src)

# tinyobjloader: OBJ file loading
CPMAddPackage("gh:tinyobjloader/tinyobjloader@1.0.6")
include_directories(SYSTEM ${tinyobjloader_SOURCE_DIR})

# stb_image: image loading for texture
CPMAddPackage(
        NAME stb_image
        DOWNLOAD_NO_EXTRACT TRUE
        URL "https://raw.githubusercontent.com/nothings/stb/f4a71b13373436a2866c5d68f8f80ac6f0bc1ffe/stb_image.h"
        EXCLUDE_FROM_ALL
        SYSTEM
)
include_directories(SYSTEM ${stb_image_SOURCE_DIR})

# ImGUI: UI
CPMAddPackage("gh:ocornut/imgui@1.90.1")
include_directories(SYSTEM ${imgui_SOURCE_DIR})
add_library(imgui
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})

add_subdirectory(src)

add_subdirectory(shaders)

add_custom_target(Assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        ${OUTPUT_BINARY_DIR}/assets
)
message(STATUS "Registering assets copy to ${OUTPUT_BINARY_DIR}/assets")
