cmake_minimum_required(VERSION 3.26)
project(drone_flight_visualizer)

set(CMAKE_CXX_STANDARD 20)

set(source_DIR ${CMAKE_SOURCE_DIR}/src)

if (WIN32) # Install dlls in the same directory as the executable on Windows
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif ()

set(CPM_SOURCE_CACHE ${CMAKE_SOURCE_DIR}/libraries)

include(cmake/CPM.cmake)

# Glaze JSON/CSV library
CPMAddPackage("gh:stephenberry/glaze@1.3.5")
include_directories(${glaze_SOURCE_DIR}/include)

#[[# VK bootstrap vulkan starter library
CPMAddPackage("gh:charles-lunarg/vk-bootstrap@0.7")
include_directories(${vk-bootstrap_SOURCE_DIR}/src)]]

# CPR C++ requests
#TODO: update to the latest tag once PR 948 is merged and released
CPMAddPackage("gh:libcpr/cpr#d7cec83e9480253bacf11ff6441e9d00c08c71bf")
include_directories(${cpr_SOURCE_DIR}/include)

# glfw library include and configuration
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
CPMAddPackage("gh:glfw/glfw#3.3.8")
include_directories(${glfw_SOURCE_DIR}/include)

# glm library
CPMAddPackage("gh:g-truc/glm#0.9.9.8")
include_directories(${glm_SOURCE_DIR}/glm)


# Find vulkan headers
find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})

add_executable(drone_flight_visualizer
        ${source_DIR}/main.cpp
        ${source_DIR}/vulkan/vulkan.cpp
)

include(FetchContent)
FetchContent_Declare(
        fetch_vk_bootstrap
        GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
        GIT_TAG        v0.7 #suggest using a tag so the library doesn't update whenever new commits are pushed to a branch
)
FetchContent_MakeAvailable(fetch_vk_bootstrap)

target_link_libraries(${PROJECT_NAME} PRIVATE
        glaze::glaze
        cpr::cpr
        glfw
        Vulkan::Vulkan
        #vk-bootstrap::vk-bootstrap
        glm
)



