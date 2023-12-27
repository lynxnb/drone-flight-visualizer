set(DFV_SOURCE_COMMON
        visualizer.cpp
        vulkan/vk_engine.cpp
        vulkan/vk_initializers.cpp
        vulkan/vk_pipeline.cpp
        vulkan/deletion_queue.cpp
        vulkan/vk_mesh.cpp
        vulkan/vk_engine_utils.cpp
        vulkan/vk_engine_init.cpp
        utils/stb_image_loader.cpp
)

set(DFV_SOURCE_MAP
        map/data_fetcher.cpp
        map/map_manager.cpp
        map/chunk_loader.cpp
)


# drone_flight_visualizer executable
add_executable(drone_flight_visualizer
        ${DFV_SOURCE_COMMON}
        ${DFV_SOURCE_MAP}
        glfw/glfw_surface.cpp
        flight_data/drone_flight_data.cpp
        drone_entrypoint.cpp
)

target_include_directories(drone_flight_visualizer PRIVATE ".")

# Link the executable to the libraries
target_link_libraries(drone_flight_visualizer PRIVATE
        cpr::cpr
        Vulkan::Vulkan
        vk-bootstrap::vk-bootstrap
        glfw
        glm
        tinyobjloader
)

add_dependencies(drone_flight_visualizer Shaders)
add_dependencies(drone_flight_visualizer Assets)


# mock visualizer
add_executable(mock_visualizer
        ${DFV_SOURCE_COMMON}
        glfw/glfw_surface.cpp
        flight_data/mock_flight_data.cpp
        mock_entrypoint.cpp
)
target_include_directories(mock_visualizer PRIVATE ".")

target_link_libraries(mock_visualizer PRIVATE
        Vulkan::Vulkan
        vk-bootstrap::vk-bootstrap
        glfw
        glm
        tinyobjloader
)

add_dependencies(mock_visualizer Shaders)
add_dependencies(mock_visualizer Assets)

# mesh visualizer
add_executable(mesh_visualizer
        ${DFV_SOURCE_COMMON}
        ${DFV_SOURCE_MAP}
        glfw/glfw_surface.cpp
        flight_data/mock_flight_data.cpp
        mesh_entrypoint.cpp
)
target_include_directories(mesh_visualizer PRIVATE ".")

target_link_libraries(mesh_visualizer PRIVATE
        cpr::cpr
        Vulkan::Vulkan
        vk-bootstrap::vk-bootstrap
        glfw
        glm
        tinyobjloader
)

add_dependencies(mesh_visualizer Shaders)
add_dependencies(mesh_visualizer Assets)

